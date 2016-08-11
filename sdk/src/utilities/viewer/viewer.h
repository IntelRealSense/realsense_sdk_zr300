// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <map>
#include <mutex>
#include <chrono>
#include <memory>
#include <queue>
#include <thread>
#include <condition_variable>
#include <librealsense/rs.hpp>
#include <rs_core.h>
#include <tuple>

//opengl api
#define GLFW_INCLUDE_GLU

#include <GLFW/glfw3.h>

namespace rs
{
    namespace utils
    {
        class viewer
        {
        public:
            viewer(device *device, uint32_t window_size, std::string window_title = "");
            ~viewer();
            void show_frame(rs::frame frame);
            void show_image(rs::utils::smart_ptr<const rs::core::image_interface> image);
            void show_image(std::shared_ptr<rs::core::image_interface> image);

        private:
            //rendering
            void setup_windows();
            void render_image(std::shared_ptr<rs::core::image_interface> image);
            void render_color2depth();

            std::mutex m_render_mutex;
            int m_window_size;
            std::string m_window_title;
            GLFWwindow * m_window;
            rs::device * m_device;
            std::vector<core::stream_type> m_streams;
            std::map<rs::core::stream_type, int> m_windows_positions;
        };
    }
}
