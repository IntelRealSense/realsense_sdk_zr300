// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "viewer.h"
#include "rs_sdk_version.h"

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
            auto image = rs::utils::get_shared_ptr_with_releaser(core::image_interface::create_instance_from_librealsense_frame(frame, rs::core::image_interface::flag::any));
            update_buffer(image);
        }

        void viewer::show_image(const rs::core::image_interface * image)
        {
            if(!image) return;
            auto smart_img = rs::utils::get_shared_ptr_with_releaser(const_cast<rs::core::image_interface*>(image));
            update_buffer(smart_img);
        }

        void viewer::show_image(std::shared_ptr<rs::core::image_interface> image)
        {
            if(!image) return;
            update_buffer(image);
        }

        bool viewer::add_window(rs::core::stream_type stream)
        {
            if(m_windows_positions.find(stream) == m_windows_positions.end())
            {
                m_windows_positions[stream] = 0;
                if(m_windows_positions.size() < m_stream_count) return false;
                int pos = 0;
                for(auto & stream : m_windows_positions)
                {
                    stream.second = pos++;
                }
            }
            return true;
        }

        void viewer::render_image(std::shared_ptr<rs::core::image_interface> image)
        {
            auto stream = image->query_stream_type();

            std::lock_guard<std::mutex> guard(m_render_mutex);

            if(!add_window(stream)) return;

            int gl_format, gl_pixel_size;
            const core::image_interface * converted_image = nullptr;
            const core::image_interface * image_to_show = image.get();
            switch(image->query_info().format)
            {
                case rs::core::pixel_format::rgb8:
                    gl_format = GL_RGB;
                    gl_pixel_size = GL_UNSIGNED_BYTE;
                    break;
                case rs::core::pixel_format::bgr8:
                    gl_format = GL_BGR_EXT;
                    gl_pixel_size = GL_UNSIGNED_BYTE;
                    break;
                case rs::core::pixel_format::yuyv:
                    if(image->convert_to(core::pixel_format::rgba8, &converted_image) != core::status_no_error) return;
                    image_to_show = converted_image;
                    gl_format = GL_RGBA;
                    gl_pixel_size = GL_UNSIGNED_BYTE;
                    break;
                case rs::core::pixel_format::rgba8:
                    gl_format = GL_RGBA;
                    gl_pixel_size = GL_UNSIGNED_BYTE;
                    break;
                case rs::core::pixel_format::bgra8:
                    gl_format = GL_BGRA_EXT;
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
                    if(image->convert_to(core::pixel_format::rgba8, &converted_image) != core::status_no_error) return;
                    image_to_show = converted_image;
                    gl_format = GL_RGBA;
                    gl_pixel_size = GL_UNSIGNED_BYTE;
                    break;
                default:
                    throw "format is not supported";
            }
            auto converted_image_releaser = rs::utils::get_unique_ptr_with_releaser(converted_image);

            gl_draw(image_to_show, gl_format, gl_pixel_size);
        }

        void viewer::gl_draw(const rs::core::image_interface * image, int gl_format, int gl_pixel_size)
        {
            auto size = update_window_size(image);
            auto width = size.first;
            auto height = size.second;

            glLoadIdentity ();
            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glOrtho(0, width, height, 0, -1, +1);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image->query_info().width, image->query_info().height, 0, gl_format, gl_pixel_size, image->query_data());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glBindTexture(GL_TEXTURE_2D, 0);
            glEnable(GL_TEXTURE_2D);
            glBegin(GL_QUADS);
            glTexCoord2f(0, 0); glVertex2f(0, 0);
            glTexCoord2f(0, 1); glVertex2f(0, (float)height);
            glTexCoord2f(1, 1); glVertex2f((float)width, (float)height);
            glTexCoord2f(1, 0); glVertex2f((float)width, 0);
            glEnd();
            glDisable(GL_TEXTURE_2D);
            glPopMatrix();
            glfwSwapBuffers(m_window);
        }

        std::pair<int,int> viewer::update_window_size(const rs::core::image_interface * image)
        {
            auto info = image->query_info();
            auto position = m_windows_positions.at(image->query_stream_type());
            int window_width, window_height;
            glfwGetWindowSize(m_window, &window_width, &window_height);

            auto window_grid = calc_grid(window_width, window_height, m_stream_count);
            auto image_window_width = window_width / window_grid.first;
            auto image_window_height = window_height / window_grid.second;

//            m_window_size = std::min(image_window_width, image_window_height);

            double scale_width = (double)image_window_width / (double)info.width;
            double scale_height = (double)image_window_height / (double)info.height;
            auto width = scale_width < scale_height ? image_window_width : (int)(info.width * scale_height);
            auto height = scale_height < scale_width ? image_window_height : (int)(info.height * scale_width);


            glfwMakeContextCurrent(m_window);
            auto x_entry = GLint((position % window_grid.first) * image_window_width + (int)((image_window_width - width) / 2.0));
            auto y_entry = GLint((window_grid.second - 1 - position / window_grid.first) * image_window_height + (int)((image_window_height - height) / 2.0));
            glViewport (x_entry, y_entry, width, height);

            return std::pair<int,int>(width, height);
        }

        std::pair<int,int> viewer::calc_grid(size_t width, size_t height, size_t streams)
        {
            float ratio = (float)width / (float)height;
            auto x = sqrt(ratio * (float)streams);
            auto y = (float)streams / x;
            x = round(x);
            y = round(y);
            if(x*y > streams)
                y > x ? y-- : x--;
            if(x*y < streams)
                y > x ? x++ : y++;
            return x == 0 || y == 0 ? std::pair<int,int>(1,1) : std::pair<int,int>(x,y);
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
            m_window = glfwCreateWindow(int(m_window_size), (int)(0.75 * m_window_size), title.c_str(), NULL, NULL);
            glfwMakeContextCurrent(m_window);

            glViewport (0, 0, GLsizei(m_window_size * m_stream_count), m_window_size);
            glClear(GL_COLOR_BUFFER_BIT);
            glfwSwapBuffers(m_window);
        }
    }
}
