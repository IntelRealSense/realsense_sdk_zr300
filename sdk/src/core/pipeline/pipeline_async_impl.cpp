// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <vector>
#include <algorithm>
#include <librealsense/rs.hpp>
#include "rs/core/context_interface.h"
#include "rs/utils/librealsense_conversion_utils.h"
#include "rs/utils/log_utils.h"
#include "pipeline_async_impl.h"
#include "sync_samples_consumer.h"
#include "async_samples_consumer.h"

using namespace std;
using namespace rs::utils;

namespace rs
{
    namespace core
    {
        pipeline_async_impl::pipeline_async_impl(const char * playback_file_path) :
            pipeline_common(playback_file_path),
            m_streaming_device_manager(nullptr)
        {

        }

        status pipeline_async_impl::start(callback_handler * app_callbacks_handler)
        {   
            if(!app_callbacks_handler)
            {
                return status_handle_invalid;
            }

            // get the current configuration
            pipeline_common_interface::pipeline_config current_pipeline_config = {};
            auto query_config_status = query_current_config(current_pipeline_config);
            if(query_config_status < status_no_error)
            {
                LOG_ERROR("cv module failed to query_current_config, error code : " << query_config_status);
                return query_config_status;
            }

            auto user_actual_config = current_pipeline_config.module_config;
            std::vector<std::shared_ptr<samples_consumer_base>> samples_consumers;
            //application samples consumer creation :
            samples_consumers.push_back(std::unique_ptr<samples_consumer_base>(
                new sync_samples_consumer(
                        [app_callbacks_handler](std::shared_ptr<correlated_sample_set> sample_set)
                            {
                              correlated_sample_set copied_sample_set = *sample_set;
                              copied_sample_set.add_ref();
                              app_callbacks_handler->on_new_sample_set(&copied_sample_set);
                            },
                        user_actual_config,
                        video_module_interface::supported_module_config::time_sync_mode::time_synced_input_only)));

            // create a samples consumer for each cv module
            auto cv_modules_configs = get_cv_module_configurations();
            for(auto module_config : cv_modules_configs)
            {
                auto cv_module = std::get<0>(module_config);
                video_module_interface::actual_module_config & actual_module_config = std::get<1>(module_config);
                video_module_interface::supported_module_config & extended_module_config = std::get<2>(module_config);

                auto is_cv_module_sync = true;
                auto config_flags = extended_module_config.config_flags;
                if(config_flags & video_module_interface::supported_module_config::flags::async_processing_supported)
                {
                    is_cv_module_sync = false;
                }

                if(is_cv_module_sync)
                {
                    samples_consumers.push_back(std::unique_ptr<samples_consumer_base>(new sync_samples_consumer(
                            [cv_module, app_callbacks_handler](std::shared_ptr<correlated_sample_set> sample_set)
                            {
                                //push to sample_set to the cv module proccess sync
                                correlated_sample_set copied_sample_set = *sample_set;
                                copied_sample_set.add_ref();

                                auto status = cv_module->process_sample_set_sync(&copied_sample_set);
                                if(status < status_no_error)
                                {
                                    LOG_ERROR("cv module failed to sync process sample set, error code" << status);
                                    return;
                                }

                                app_callbacks_handler->on_cv_module_process_complete(cv_module->query_module_uid());
                            },
                            actual_module_config,
                            extended_module_config.samples_time_sync_mode)));
                }
                else //cv_module is async
                {
                    samples_consumers.push_back(std::unique_ptr<samples_consumer_base>(new async_samples_consumer(
                                                                                               app_callbacks_handler,
                                                                                               cv_module,
                                                                                               actual_module_config,
                                                                                               extended_module_config.samples_time_sync_mode)));
                }
            }

            std::unique_ptr<rs::core::streaming_device_manager> streaming_device_manager;
            try
            {
                streaming_device_manager.reset(new rs::core::streaming_device_manager(
                                                           user_actual_config,
                                                           [this](std::shared_ptr<correlated_sample_set> sample_set) { non_blocking_set_sample(sample_set); },
                                                            m_context));
            }
            catch(const std::exception & ex)
            {
                LOG_ERROR("failed to start device, error message : " << ex.what());
                return status_device_failed;
            }
            catch(...)
            {
                LOG_ERROR("failed to start device");
                return status_device_failed;
            }

            //commit to update the pipeline state
            {
                std::lock_guard<std::mutex> samples_consumers_guard(samples_consumers_lock);
                m_samples_consumers = std::move(samples_consumers);
            }

            m_streaming_device_manager = std::move(streaming_device_manager);

            return status_no_error;
        }

        status pipeline_async_impl::stop()
        {
            resources_reset();
            return status_no_error;
        }

        status pipeline_async_impl::reset()
        {
            resources_reset();
            return pipeline_common::reset();
        }

        void pipeline_async_impl::non_blocking_set_sample(std::shared_ptr<correlated_sample_set> sample_set)
        {
            std::lock_guard<std::mutex> samples_consumers_guard(samples_consumers_lock);
            for(size_t i = 0; i < m_samples_consumers.size(); ++i)
            {
                auto current_sample_consumer = m_samples_consumers[i];
                if(current_sample_consumer)
                {
                    if(current_sample_consumer->is_sample_set_contains_a_single_required_sample(sample_set))
                    {
                        current_sample_consumer->non_blocking_set_sample_set(sample_set);
                    }
                }
            }
        }

        void pipeline_async_impl::resources_reset()
        {
            {
                std::lock_guard<std::mutex> samples_consumers_guard(samples_consumers_lock);
                m_samples_consumers.clear();
            }
            m_streaming_device_manager.reset();
        }

        pipeline_async_impl::~pipeline_async_impl()
        {
            resources_reset();
        }
    }
}

