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
#include <functional>
#include <atomic>

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
            viewer(size_t stream_count, uint32_t window_size, std::function<void()> on_close_callback, std::string window_title = "");

            ~viewer();

            void show_frame(rs::frame frame);
            void show_image(rs::utils::smart_ptr<const rs::core::image_interface> image);
            void show_image(std::shared_ptr<rs::core::image_interface> image);

        private:
            void setup_windows();
            void render_image(std::shared_ptr<rs::core::image_interface> image);
            void ui_refresh();
            void update_buffer(std::shared_ptr<rs::core::image_interface>& image);

            std::map<rs::core::stream_type, std::shared_ptr<rs::core::image_interface>> m_render_buffer;
            std::function<void()> m_user_on_close_callback;
            std::condition_variable m_render_thread_cv;
            std::thread m_ui_thread;
            std::mutex m_render_mutex;
            int m_window_size;
            std::string m_window_title;
            GLFWwindow * m_window;
            size_t m_stream_count;
            std::map<rs::core::stream_type, int> m_windows_positions;
            std::atomic<bool> m_is_running;
        };
    }
}
