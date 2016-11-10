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
        viewer::viewer(size_t stream_count, uint32_t window_width, uint32_t window_height, std::function<void()> on_close_callback, std::string window_title) :
            m_window(nullptr),
            m_stream_count(stream_count),
            m_user_on_close_callback(on_close_callback),
            m_is_running(true)
        {

            setup_window(window_width, window_height, window_title);
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

        void viewer::render_image(std::shared_ptr<rs::core::image_interface> image)
        {
            auto stream = image->query_stream_type();

            if(!add_window(stream)) return;

            int gl_format, gl_channel_type;
            const core::image_interface * converted_image = nullptr;
            const core::image_interface * image_to_show = image.get();

            switch(image->query_info().format)
            {
                case rs::core::pixel_format::rgb8:
                    gl_format = GL_RGB;
                    gl_channel_type = GL_UNSIGNED_BYTE;
                    break;
                case rs::core::pixel_format::bgr8:
                    gl_format = GL_BGR_EXT;
                    gl_channel_type = GL_UNSIGNED_BYTE;
                    break;
                case rs::core::pixel_format::yuyv:
                    if(image->convert_to(core::pixel_format::rgba8, &converted_image) != core::status_no_error) return;
                    gl_format = GL_RGBA;
                    gl_channel_type = GL_UNSIGNED_BYTE;
                    break;
                case rs::core::pixel_format::rgba8:
                    gl_format = GL_RGBA;
                    gl_channel_type = GL_UNSIGNED_BYTE;
                    break;
                case rs::core::pixel_format::bgra8:
                    gl_format = GL_BGRA_EXT;
                    gl_channel_type = GL_UNSIGNED_BYTE;
                    break;
                case rs::core::pixel_format::raw8:
                case rs::core::pixel_format::y8:
                    gl_format = GL_LUMINANCE;
                    gl_channel_type = GL_UNSIGNED_BYTE;
                    break;
                case rs::core::pixel_format::y16:
                    gl_format = GL_LUMINANCE;
                    gl_channel_type = GL_SHORT;
                    break;
                case rs::core::pixel_format::z16:
                    if(image->convert_to(core::pixel_format::rgba8, &converted_image) != core::status_no_error) return;
                    gl_format = GL_RGBA;
                    gl_channel_type = GL_UNSIGNED_BYTE;
                    break;
                default:
                    throw "format is not supported";
            }

            auto converted_image_releaser = rs::utils::get_unique_ptr_with_releaser(converted_image);

            if(converted_image != nullptr)
            {
                image_to_show = converted_image;
            }

            draw(image_to_show, gl_format, gl_channel_type);
        }

        bool viewer::add_window(rs::core::stream_type stream)
        {
            std::lock_guard<std::mutex> guard(m_render_mutex);

            if(m_windows_positions.find(stream) == m_windows_positions.end())
            {
                m_windows_positions[stream] = 0;
                if(m_windows_positions.size() < m_stream_count) return false;
                int pos = 0;
                //map is sorted by key values therefor the streams position is difind by the stream's enum value
                for(auto & stream : m_windows_positions)
                {
                    stream.second = pos++;
                }
            }
            return true;
        }

        void viewer::draw(const rs::core::image_interface * image, int gl_format, int gl_channel_type)
        {
            auto rect = calc_window_size(image);

            auto x_entry = rect.first.first;
            auto y_entry = rect.first.second;
            auto width = rect.second.first;
            auto height = rect.second.second;

            std::lock_guard<std::mutex> guard(m_render_mutex);

            glfwMakeContextCurrent(m_window);
            glViewport (x_entry, y_entry, width, height);

            glLoadIdentity ();
            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glOrtho(0, width, height, 0, -1, +1);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image->query_info().width, image->query_info().height, 0, gl_format, gl_channel_type, image->query_data());
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

        std::pair<viewer::int_pair, viewer::int_pair> viewer::calc_window_size(const rs::core::image_interface * image)
        {
            auto info = image->query_info();
            size_t position = m_windows_positions.at(image->query_stream_type());

            int window_width, window_height;
            glfwGetWindowSize(m_window, &window_width, &window_height);

            int_pair window_grid = calc_grid(window_width, window_height, m_stream_count);

            double grid_cell_width = window_width / (double)window_grid.first;
            double grid_cell_height = window_height / (double)window_grid.second;

            double scale_width = grid_cell_width / (double)info.width;
            double scale_height = grid_cell_height / (double)info.height;

            double width = scale_width < scale_height ? grid_cell_width : info.width * scale_height;
            double height = scale_height < scale_width ? grid_cell_height : info.height * scale_width;

            int cell_x_postion = (int)((double)(position % window_grid.first)  * grid_cell_width);
            int cell_y_position = (int)((double)(window_grid.second - 1 - (position / window_grid.first)) * grid_cell_height);

            int in_cell_x_offset = (int)((grid_cell_width - width) / 2.0);
            int in_cell_y_offset = (int)((grid_cell_height - height) / 2.0);

            int x_entry = cell_x_postion + in_cell_x_offset;
            int y_entry = cell_y_position + in_cell_y_offset;

            return std::pair<int_pair,int_pair>(int_pair(x_entry, y_entry),int_pair(width, height));
        }

        viewer::int_pair viewer::calc_grid(size_t width, size_t height, size_t streams)
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
            return x == 0 || y == 0 ? int_pair(1,1) : int_pair(x,y);
        }

        void viewer::setup_window(uint32_t width, uint32_t height, std::string window_title)
        {
            if(m_stream_count == 0)
                return;
            if(m_window)
            {
                glfwDestroyWindow(m_window);
                glfwTerminate();
            }
            glfwInit();
            m_window = glfwCreateWindow(width, height, window_title.c_str(), nullptr, nullptr);
            glfwMakeContextCurrent(m_window);
        }
    }
}
