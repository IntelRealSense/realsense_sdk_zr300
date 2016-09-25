// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include "rs/core/pipeline_async_interface.h"
#include "samples_consumer_base.h"

namespace rs
{
    namespace core
    {
        class async_samples_consumer : public samples_consumer_base,
                                       public video_module_interface::processing_event_handler
        {
        public:
            async_samples_consumer(pipeline_async_interface::callback_handler * app_callbacks_handler,
                                            video_module_interface * cv_module,
                                   const video_module_interface::actual_module_config &module_config,
                                   video_module_interface::supported_module_config::time_sync_mode time_sync_mode);

            // processing_event_handler interface
            void process_sample_complete(video_module_interface *sender, correlated_sample_set *sample) override;

            ~async_samples_consumer();
        private:
            pipeline_async_interface::callback_handler * m_app_callbacks_handler;
            video_module_interface * m_cv_module;

            std::thread m_worker_thread;
            bool m_is_closing;
            std::mutex m_lock;
            std::condition_variable m_conditional_variable;
            bool m_is_new_output_ready;

            void notify_sample_set_proccessing_completed();
            void set_ready_sample_set(std::shared_ptr<correlated_sample_set> ready_sample_set) override;
            void notifier_loop();
        };
    }
}
