// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "viewer.h"
#include "rs/core/lrs_image.h"

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
        viewer::viewer(rs::device* device, uint32_t window_size) :
            m_window_size(window_size),
            m_window(nullptr),
            m_device(device)
        {
            for(int i = 0; i <= (int)rs::core::stream_type::fisheye; i++)
            {
                if(device->is_stream_enabled((rs::stream)i))
                    m_streams.push_back((rs::core::stream_type)i);
            }
        }

        viewer::~viewer()
        {
            if(m_window)
            {
                glfwDestroyWindow(m_window);
                glfwTerminate();
            }
        }

        void viewer::show_frame(rs::frame frame)
        {
            auto image = std::shared_ptr<core::image_interface>(new rs::core::lrs_image(frame, rs::core::image_interface::flag::any, nullptr));
            render_image(std::move(image));
        }

        void viewer::show_image(std::shared_ptr<rs::core::image_interface> image)
        {
            render_image(image);
        }

        void viewer::render_image(std::shared_ptr<rs::core::image_interface> image)
        {
            auto stream = image->query_stream_type();

            std::lock_guard<std::mutex> guard(m_render_mutex);

            if(m_window == nullptr)
            {
                setup_windows();
            }

            auto position = m_windows_positions.at(stream);

            utils::smart_ptr<const core::image_interface> converted_image;
            if(image->convert_to(core::pixel_format::rgba8, converted_image) != core::status_no_error) return;

            auto info = converted_image->query_info();

            double scale = (double)m_window_size / (double)info.width;
            auto width = m_window_size;
            auto height = info.height * scale;


            glfwMakeContextCurrent(m_window);

            glViewport (position * width, 0, width, width);

            glLoadIdentity ();
            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glOrtho(0, m_window_size, m_window_size, 0, -1, +1);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, info.width, info.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, converted_image->query_data());
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
            glfwPollEvents();
        }

        void viewer::setup_windows()
        {
            int position = 0;
            for(auto it = m_streams.begin(); it != m_streams.end(); ++it)
            {
                m_windows_positions[*it] = position++;
            }
            if(m_window)
            {
                glfwDestroyWindow(m_window);
                glfwTerminate();
            }
            glfwInit();
            m_window = glfwCreateWindow(m_window_size * m_windows_positions.size(), m_window_size, "RS SDK Viewer", NULL, NULL);
            glfwMakeContextCurrent(m_window);
            glViewport (0, 0, m_window_size * m_windows_positions.size(), m_window_size);
            glClear(GL_COLOR_BUFFER_BIT);
        }
    }
}
