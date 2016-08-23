// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "viewer.h"

namespace
{
    static std::map<rs::core::stream_type, std::string> create_streams_names_map()
    {
        std::map<rs::core::stream_type, std::string> rv;
        rv[rs::core::stream_type::depth] = "depth";
        rv[rs::core::stream_type::color] = "color";
        rv[rs::core::stream_type::infrared] = "infrared";
        rv[rs::core::stream_type::infrared2] = "infrared2";
        rv[rs::core::stream_type::fisheye] = "fisheye";
        return rv;
    }

}

namespace rs
{
    namespace utils
    {
        viewer::viewer(size_t stream_count, uint32_t window_size, std::function<void()> on_close_callback, std::string window_title) :
            m_window_size(window_size),
            m_window(nullptr),
            m_window_title(window_title),
            m_stream_count(stream_count),
            m_user_on_close_callback(on_close_callback),
            m_is_running(true)
        {
            setup_windows();
            m_ui_thread = std::thread(&viewer::ui_refresh, this);
        }

        viewer::~viewer()
        {
            m_is_running = false;

            m_user_on_close_callback = nullptr;

            if (std::this_thread::get_id() == m_ui_thread.get_id())
            {
                m_ui_thread.detach();
            }
            else
            {
                m_render_thread_cv.notify_one();

                if (m_ui_thread.joinable() == true)
                    m_ui_thread.join();

            }

            glfwDestroyWindow(m_window);
            glfwTerminate();
        }

        void viewer::ui_refresh()
        {
            std::vector<std::shared_ptr<rs::core::image_interface>> images;
            images.reserve(5); // MAX_STREAM_TYPES_COUNT

            auto pred = [this]() -> bool
            {
                    return ((m_is_running == false) || (m_render_buffer.size() > 0));
            };
                    ;
            while(m_is_running)
            {
                std::unique_lock<std::mutex> locker(m_render_mutex);
                bool render =  m_render_thread_cv.wait_for(locker, std::chrono::milliseconds(10), pred);

                if (render == true)
                {
                    if (m_is_running == false)
                        return;

                    for (auto& image_pair : m_render_buffer)
                    {
                        images.emplace_back(image_pair.second);
                    }

                    m_render_buffer.clear();
                }

                locker.unlock();

                if (render)
                {
                    // TODO: make images clear scope guard

                    for (auto& image : images)
                    {
                        render_image(image);
                    }

                    images.clear();
                }

                glfwPollEvents();
                if (glfwWindowShouldClose(m_window) > 0)
                {
                    m_is_running = false;
                }
            }


            std::unique_lock<std::mutex> locker(m_render_mutex);
            m_render_buffer.clear();
            locker.unlock();

            if(m_user_on_close_callback != nullptr)
            {
                m_user_on_close_callback();
            }
        }

        void viewer::update_buffer(std::shared_ptr<rs::core::image_interface>& image)
        {
            std::unique_lock<std::mutex> locker(m_render_mutex);

            if (m_is_running)
            {
                rs::core::stream_type stream_type = image->query_stream_type();

                bool notify = false;
                if (m_render_buffer.find(stream_type) == m_render_buffer.end())
                    notify = true;

                m_render_buffer[image->query_stream_type()] =image;

                locker.unlock();

                if (notify)
                    m_render_thread_cv.notify_one();
            }

        }

        void viewer::show_frame(rs::frame frame)
        {
            auto image = std::shared_ptr<core::image_interface>(core::image_interface::create_instance_from_librealsense_frame(frame, rs::core::image_interface::flag::any, nullptr));
            update_buffer(image);
        }

        void viewer::show_image(rs::utils::smart_ptr<const rs::core::image_interface> image)
        {
            if(!image) return;
            auto smart_img = std::shared_ptr<rs::core::image_interface>(const_cast<rs::core::image_interface*>(image.get()), [](rs::core::image_interface*){});
            update_buffer(smart_img);
        }

        void viewer::show_image(std::shared_ptr<rs::core::image_interface> image)
        {
            if(!image) return;
            update_buffer(image);
        }

        void viewer::render_image(std::shared_ptr<rs::core::image_interface> image)
        {
            auto stream = image->query_stream_type();

            std::lock_guard<std::mutex> guard(m_render_mutex);

            if(m_windows_positions.find(stream) == m_windows_positions.end())
            {
                m_windows_positions[stream] = m_windows_positions.size() - 1;
            }

            size_t position = m_windows_positions.at(stream);

            if(position >= m_stream_count) return;

            int gl_format, gl_pixel_size;
            utils::smart_ptr<const core::image_interface> converted_image;
            const core::image_interface * image_to_show = image.get();
            switch(image->query_info().format)
            {
                case rs::core::pixel_format::rgb8:
                    gl_format = GL_RGB;
                    gl_pixel_size = GL_UNSIGNED_BYTE;
                    break;
                case rs::core::pixel_format::bgr8:
                    gl_format = GL_BGR;
                    gl_pixel_size = GL_UNSIGNED_BYTE;
                    break;
                case rs::core::pixel_format::yuyv:
                    gl_format = GL_LUMINANCE_ALPHA;
                    gl_pixel_size = GL_UNSIGNED_BYTE;
                    break;
                case rs::core::pixel_format::rgba8:
                    gl_format = GL_RGBA;
                    gl_pixel_size = GL_UNSIGNED_BYTE;
                    break;
                case rs::core::pixel_format::bgra8:
                    gl_format = GL_BGRA;
                    gl_pixel_size = GL_UNSIGNED_BYTE;
                    break;
                case rs::core::pixel_format::raw8:
                case rs::core::pixel_format::y8:
                    gl_format = GL_LUMINANCE;
                    gl_pixel_size = GL_UNSIGNED_BYTE;
                    break;
                case rs::core::pixel_format::y16:
                    gl_format = GL_LUMINANCE;
                    gl_pixel_size = GL_SHORT;
                    break;
                case rs::core::pixel_format::z16:
                    if(image->convert_to(core::pixel_format::rgba8, converted_image) != core::status_no_error) return;
                    image_to_show = converted_image.get();
                    gl_format = GL_RGBA;
                    gl_pixel_size = GL_UNSIGNED_BYTE;
                    break;
                default:
                    throw "format is not supported";
            }

            auto info = image_to_show->query_info();

            double scale = (double)m_window_size / (double)info.width;
            auto width = m_window_size;
            auto height = info.height * scale;


            glfwMakeContextCurrent(m_window);

            glViewport (position * width, 0, width, width);

            glLoadIdentity ();
            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glOrtho(0, m_window_size, m_window_size, 0, -1, +1);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, info.width, info.height, 0, gl_format, gl_pixel_size, image_to_show->query_data());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glBindTexture(GL_TEXTURE_2D, 0);
            glEnable(GL_TEXTURE_2D);
            glBegin(GL_QUADS);
            glTexCoord2f(0, 0); glVertex2f(0, (width - height) / 2.0);
            glTexCoord2f(0, 1); glVertex2f(0, width - (width - height) / 2.0);
            glTexCoord2f(1, 1); glVertex2f(width, width - (width - height) / 2.0);
            glTexCoord2f(1, 0); glVertex2f(width, (width - height) / 2.0);
            glEnd();
            glDisable(GL_TEXTURE_2D);
            glPopMatrix();
            glfwSwapBuffers(m_window);
        }

        void viewer::setup_windows()
        {
            if(m_stream_count == 0)
                return;
            if(m_window)
            {
                glfwDestroyWindow(m_window);
                glfwTerminate();
            }
            glfwInit();
            auto title = m_window_title == "" ? "RS SDK Viewer" : m_window_title;
            m_window = glfwCreateWindow(m_window_size * m_stream_count, m_window_size, title.c_str(), NULL, NULL);
            glfwMakeContextCurrent(m_window);

            glViewport (0, 0, m_window_size * m_stream_count, m_window_size);
            glClear(GL_COLOR_BUFFER_BIT);
            glfwSwapBuffers(m_window);
        }
    }
}
