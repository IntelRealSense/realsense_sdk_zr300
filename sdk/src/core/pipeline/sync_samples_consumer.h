// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include "rs/core/pipeline_async_interface.h"
#include "samples_consumer_base.h"

namespace rs
{
    namespace core
    {
        /**
         * @brief The samples_consumer class
         */
        class sync_samples_consumer : public samples_consumer_base
        {
        public:
            sync_samples_consumer(std::function<void(std::shared_ptr<correlated_sample_set>)> sample_set_ready_handler,
                                  const video_module_interface::actual_module_config & module_config,
                                  video_module_interface::supported_module_config::time_sync_mode time_sync_mode);

            virtual ~sync_samples_consumer();
        protected:

        private:
            std::thread m_samples_consumer_thread;
            bool m_is_closing;
            std::shared_ptr<correlated_sample_set> m_current_sample_set;
            std::mutex m_lock;
            std::condition_variable m_conditional_variable;

            std::function<void(std::shared_ptr<correlated_sample_set>)> m_sample_set_ready_handler;
            void set_ready_sample_set(std::shared_ptr<correlated_sample_set> ready_sample_set) override;

            void consumer_loop();

        };
    }
}
