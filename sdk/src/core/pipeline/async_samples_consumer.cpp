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
                                                       const video_module_interface::actual_module_config &module_config,
                                                       const video_module_interface::supported_module_config::time_sync_mode time_sync_mode):
            samples_consumer_base(module_config, time_sync_mode),
            m_app_callbacks_handler(app_callbacks_handler),
            m_cv_module(cv_module)
        {
            m_cv_module->register_event_hander(this);
        }

        void async_samples_consumer::on_complete_sample_set(std::shared_ptr<correlated_sample_set> ready_sample_set)
        {
            correlated_sample_set copied_sample_set = *ready_sample_set;
            status process_sample_set_status = m_cv_module->process_sample_set_async(&copied_sample_set);
            if(process_sample_set_status < status_no_error)
            {
                LOG_ERROR("failed async sample process");
                if(m_app_callbacks_handler)
                {
                    m_app_callbacks_handler->on_status(process_sample_set_status);
                }
                return;
            }
        }

        void async_samples_consumer::process_sample_complete(video_module_interface *sender, correlated_sample_set *sample)
        {
            if(m_app_callbacks_handler)
            {
                try
                {
                    m_app_callbacks_handler->on_cv_module_process_complete(m_cv_module);
                }
                catch(const std::exception & ex)
                {
                    LOG_ERROR("app callbacks handler throw ex" << ex.what());
                    throw ex;
                }
            }
        }

        async_samples_consumer::~async_samples_consumer()
        {
            m_cv_module->unregister_event_hander(this);
        }
    }
}
