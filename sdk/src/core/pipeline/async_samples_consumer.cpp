// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "async_samples_consumer.h"

#include <iostream>

using namespace rs::utils;

namespace rs
{
    namespace core
    {
        async_samples_consumer::async_samples_consumer(pipeline_async_interface::callback_handler *app_callbacks_handler,
                                                       video_module_interface * cv_module,
                                                       const video_module_interface::supported_module_config &module_config):
            samples_consumer_base(module_config),
            m_app_callbacks_handler(app_callbacks_handler),
            m_cv_module(cv_module),
            m_is_closing(false),
            m_is_new_output_ready(false)
        {
            m_cv_module->register_event_hander(this);
            m_worker_thread = std::thread(&async_samples_consumer::notifier_loop, this);
        }

        void async_samples_consumer::on_complete_sample_set(std::shared_ptr<correlated_sample_set> ready_sample_set)
        {
            correlated_sample_set copied_sample_set = *ready_sample_set;
            copied_sample_set.add_ref();
            m_cv_module->process_sample_set_async(&copied_sample_set);
        }

        void async_samples_consumer::notify_sample_set_proccessing_completed()
        {
            {
                std::unique_lock<std::mutex> lock(m_lock);
                m_is_new_output_ready = true;
            }

            m_conditional_variable.notify_one();
        }

        void async_samples_consumer::notifier_loop()
        {
            while(!m_is_closing)
            {
                {
                    std::unique_lock<std::mutex> lock(m_lock);
                    m_conditional_variable.wait(lock, [this]() {return m_is_new_output_ready;});
                    m_is_new_output_ready = false;
                }

                if(!m_is_closing && m_app_callbacks_handler)
                {
                    try
                    {
                        m_app_callbacks_handler->on_cv_module_process_complete(m_cv_module->query_module_uid());
                    }
                    catch(const std::exception & ex)
                    {
                        LOG_ERROR("m_app_callbacks_handler callback throw ex" << ex.what());
                        throw ex;
                    }
                }
            }
        }

        void async_samples_consumer::process_sample_complete(video_module_interface *sender, correlated_sample_set *sample)
        {
            //make sure we release any samples that were add-refed
            auto scoped_sample_set = get_unique_ptr_with_releaser(sample);

            notify_sample_set_proccessing_completed();
        }

        async_samples_consumer::~async_samples_consumer()
        {
            m_is_closing = true;
            notify_sample_set_proccessing_completed();
            if(m_worker_thread.joinable())
            {
                m_worker_thread.join();
            }
            m_cv_module->unregister_event_hander(this);
        }
    }
}
