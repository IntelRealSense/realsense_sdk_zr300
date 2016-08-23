// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <mutex>
#include <condition_variable>
#include <thread>

#include "rs/cv_modules/max_depth_value_module/max_depth_value_module_interface.h"
#include "rs_core.h"

namespace rs
{
    namespace cv_modules
    {
        /**
         * @brief The max_depth_value_module class
         * an example computer vision module that calculates the max depth value.
         */
        class max_depth_value_module_impl : public rs::core::video_module_interface, public max_depth_value_module_interface
        {
        public:
            max_depth_value_module_impl(const max_depth_value_module_impl & other) = delete;
            max_depth_value_module_impl & operator=(const max_depth_value_module_impl & other) = delete;
            max_depth_value_module_impl(const max_depth_value_module_impl && other) = delete;
            max_depth_value_module_impl & operator=(const max_depth_value_module_impl && other) = delete;

            max_depth_value_module_impl(uint64_t milisenconds_added_to_simulate_larger_computation_time = 0);

            // video_module_interface impl
            int32_t query_module_uid() override;
            rs::core::status query_supported_module_config(int32_t idx,
                    rs::core::video_module_interface::supported_module_config &supported_config) override;
            rs::core::status query_current_module_config(rs::core::video_module_interface::actual_module_config &module_config) override;
            rs::core::status set_module_config(const rs::core::video_module_interface::actual_module_config &module_config) override;
            rs::core::status process_sample_set_sync(rs::core::correlated_sample_set *sample_set) override;
            rs::core::status process_sample_set_async(rs::core::correlated_sample_set *sample_set) override;
            rs::core::status register_event_hander(rs::core::video_module_interface::processing_event_handler *handler) override;
            rs::core::status unregister_event_hander(rs::core::video_module_interface::processing_event_handler *handler) override;
            rs::core::video_module_control_interface *query_video_module_control() override;

            // max_depth_value_module_interface impl
            rs::utils::smart_ptr<max_depth_value_output_data> get_max_depth_value_data() override;

            ~max_depth_value_module_impl();

        private:
            const uint64_t m_milliseconds_added_to_simulate_larger_computation_time;
            rs::core::video_module_interface::actual_module_config m_current_module_config;
            rs::core::video_module_interface::processing_event_handler * m_processing_handler;

            rs::utils::smart_ptr<max_depth_value_output_data> process_depth_max_value(rs::utils::smart_ptr<rs::core::image_interface> depth_image);

            //thread for handling inputs throughput in async flow
            std::thread m_processing_thread;
            void async_processing_loop();
            bool m_is_closing;

            //internal class to handle a single object non blocking set and blocked get.
            template <typename T>
            class thread_safe_object
            {
            public:
                thread_safe_object(const thread_safe_object<T>& other) = delete;
                thread_safe_object & operator=(const T & other) = delete;
                thread_safe_object(const thread_safe_object<T>&& other) = delete;
                thread_safe_object & operator=(const T && other) = delete;

                explicit thread_safe_object(const T & object) : m_object(object) {}

                void set(const T & updated_object)
                {
                    //update the current object even if no one took it
                    {
                        std::unique_lock<std::mutex> lock(m_object_lock);
                        m_object = std::move(updated_object);
                        m_is_object_ready = true;
                    }
                    std::cout<< "new object is ready"<<std::endl;
                    m_object_conditional_variable.notify_one();
                }

                T blocking_get()
                {
                    //take the current output data
                    T output_object;
                    {
                        std::unique_lock<std::mutex> lock(m_object_lock);
                        m_object_conditional_variable.wait(lock, [this]() {return m_is_object_ready;});
                        output_object = std::move(m_object);
                        m_is_object_ready = false;
                    }
                    std::cout<<"object taken" <<std::endl;

                    return std::move(output_object);
                }

            private:
                T m_object;
                std::mutex m_object_lock;
                std::condition_variable m_object_conditional_variable;
                bool m_is_object_ready;
            };

            thread_safe_object<rs::utils::smart_ptr<rs::core::image_interface>> m_input_depth_image;
            thread_safe_object<rs::utils::smart_ptr<max_depth_value_output_data>> m_output_data;
        };
    }
}
