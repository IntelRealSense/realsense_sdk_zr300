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
#include <rs_core.h>
#include <tuple>
#include <functional>
#include <atomic>

//opengl api
#define GLFW_INCLUDE_GLU

#include <GLFW/glfw3.h>

#ifdef WIN32 
#ifdef realsense_viewer_EXPORTS
#define  DLL_EXPORT __declspec(dllexport)
#else
#define  DLL_EXPORT __declspec(dllimport)
#endif /* realsense_viewer_EXPORTS */
#else /* defined (WIN32) */
#define DLL_EXPORT
#endif

namespace rs
{
    namespace utils
    {
        class DLL_EXPORT viewer
        {
            using int_pair = std::pair<int, int>;
        public:
            viewer(size_t stream_count, uint32_t width, uint32_t height, std::function<void()> on_close_callback, std::string title = "");

            ~viewer();

            void show_image(const rs::core::image_interface * image);
            void show_image(std::shared_ptr<rs::core::image_interface> image);

        private:
            void setup_window(uint32_t width, uint32_t height, std::string window_title);
            void render_image(std::shared_ptr<rs::core::image_interface> image);
            void draw(const rs::core::image_interface * image, int gl_format, int gl_channel_type);
            void ui_refresh();
            void update_buffer(std::shared_ptr<rs::core::image_interface>& image);
            bool add_window(rs::core::stream_type stream);

            int_pair calc_grid(size_t width, size_t height, size_t streams);
            std::pair<int_pair, int_pair> calc_window_size(const core::image_interface *image);
            std::map<rs::core::stream_type, std::shared_ptr<rs::core::image_interface>> m_render_buffer;
            uint32_t m_width;
            uint32_t m_height;
            std::string m_title;
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
