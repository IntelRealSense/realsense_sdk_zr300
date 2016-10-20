// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "rs/utils/log_utils.h"
#include "sync_samples_consumer.h"

#include "samples_consumer_base.h"
using namespace rs::utils;

namespace rs
{
    namespace core
    {
        sync_samples_consumer::sync_samples_consumer(std::function<void(std::shared_ptr<correlated_sample_set>)> sample_set_ready_handler,
                                                     const video_module_interface::actual_module_config &module_config,
                                                     const video_module_interface::supported_module_config::time_sync_mode time_sync_mode):
            samples_consumer_base(module_config, time_sync_mode),
            m_is_closing(false),
            m_current_sample_set(nullptr),
            m_sample_set_ready_handler(sample_set_ready_handler)
        {
            m_samples_consumer_thread = std::thread(&sync_samples_consumer::consumer_loop, this);
        }

        void sync_samples_consumer::on_complete_sample_set(std::shared_ptr<correlated_sample_set> ready_sample_set)
        {
            //update the current object even if no one took it
            {
                std::unique_lock<std::mutex> lock(m_lock);
                m_current_sample_set = std::move(ready_sample_set);
            }
            m_conditional_variable.notify_one();
        }

        void sync_samples_consumer::consumer_loop()
        {
            while(!m_is_closing)
            {
                std::shared_ptr<correlated_sample_set> samples_set;
                {
                  std::unique_lock<std::mutex> lock(m_lock);
                  m_conditional_variable.wait(lock, [this]() { return m_is_closing || ( !m_is_closing && m_current_sample_set);});
                  samples_set.swap(m_current_sample_set);
                }

                if(!samples_set)
                {
                    continue;
                }

                try
                {
                    m_sample_set_ready_handler(samples_set);
                }
                catch(const std::exception & ex)
                {
                    LOG_ERROR("m_sample_set_ready_handler callback throw ex" << ex.what());
                    throw ex;
                }
            }
        }

        sync_samples_consumer::~sync_samples_consumer()
        {
            m_is_closing = true;
            m_conditional_variable.notify_one();
            if(m_samples_consumer_thread.joinable())
            {
                m_samples_consumer_thread.join();
            }
        }
    }
}
