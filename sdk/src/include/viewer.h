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
            using int2 = std::pair<int, int>;
        public:
            viewer(size_t stream_count, uint32_t window_width, uint32_t window_height, std::function<void()> on_close_callback, std::string window_title = "");

            ~viewer();

            void show_frame(rs::frame frame);
            void show_image(const rs::core::image_interface * image);
            void show_image(std::shared_ptr<rs::core::image_interface> image);

        private:
            void setup_windows(uint32_t width, uint32_t height, std::string window_title);
            void render_image(std::shared_ptr<rs::core::image_interface> image);
            void draw(const rs::core::image_interface * image, int gl_format, int gl_pixel_size);
            void ui_refresh();
            void update_buffer(std::shared_ptr<rs::core::image_interface>& image);
            bool add_window(rs::core::stream_type stream);

            int2 calc_grid(size_t width, size_t height, size_t streams);
            std::pair<int2, int2> calc_window_size(const core::image_interface *image);
            std::map<rs::core::stream_type, std::shared_ptr<rs::core::image_interface>> m_render_buffer;
            std::function<void()> m_user_on_close_callback;
            std::condition_variable m_render_thread_cv;
            std::thread m_ui_thread;
            std::mutex m_render_mutex;
            GLFWwindow * m_window;
            size_t m_stream_count;
            std::map<rs::core::stream_type, size_t> m_windows_positions;
            std::atomic<bool> m_is_running;
        };
    }
}
