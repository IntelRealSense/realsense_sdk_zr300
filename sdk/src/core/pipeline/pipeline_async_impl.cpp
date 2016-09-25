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
            m_device(nullptr),
            m_active_sources(static_cast<rs::source>(0))
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

            //create and add sample consumers
            {
                std::lock_guard<std::mutex> state_guard(samples_consumers_lock);
                // create a samples consumer for the application
                m_samples_consumers.push_back(std::unique_ptr<samples_consumer_base>(
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
                        m_samples_consumers.push_back(std::unique_ptr<samples_consumer_base>(new sync_samples_consumer(
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
                        m_samples_consumers.push_back(std::unique_ptr<samples_consumer_base>(new async_samples_consumer(
                                                                                                   app_callbacks_handler,
                                                                                                   cv_module,
                                                                                                   actual_module_config,
                                                                                                   extended_module_config.samples_time_sync_mode)));
                    }
                }
            }
            //start librealsense device
            //TODO : get the device defined in modules_config.
            m_device = m_context->get_device(0); //device memory managed by the context

            //start with no active sources
            m_active_sources = static_cast<rs::source>(0);


            //configure streams
            vector<stream_type> all_streams;
            for(auto stream_index = 0; stream_index < static_cast<int32_t>(stream_type::max); stream_index++)
            {
                all_streams.push_back(static_cast<stream_type>(stream_index));
            }

            for(auto &stream : all_streams)
            {
                if (!user_actual_config[stream].is_enabled)
                {
                    continue;
                }

                bool is_matching_stream_mode_found = false;
                //enable the streams
                rs::stream librealsense_stream = convert_stream_type(stream);
                auto stream_mode_count = m_device->get_stream_mode_count(librealsense_stream);
                for(auto stream_mode_index = 0; stream_mode_index < stream_mode_count; stream_mode_index++)
                {
                    int width, height, frame_rate;
                    rs::format format;
                    m_device->get_stream_mode(librealsense_stream, stream_mode_index, width, height, format, frame_rate);
                    bool is_acceptable_stream_mode = (width == user_actual_config[stream].size.width &&
                                                      height == user_actual_config[stream].size.height &&
                                                      frame_rate == user_actual_config[stream].frame_rate);
                    if(is_acceptable_stream_mode)
                    {
                        m_device->enable_stream(librealsense_stream, width, height, format, frame_rate);
                        is_matching_stream_mode_found = true;
                        break;
                    }
                }

                if(!is_matching_stream_mode_found)
                {
                    LOG_WARN("stream index " << static_cast<int32_t>(stream) << "was requested, but no valid configuration mode was found");
                    continue;
                }

                //define callbacks to the actual streams and set them.
                m_stream_callback_per_stream[stream] = [stream, this](rs::frame frame)
                {
                    std::shared_ptr<correlated_sample_set> sample_set(new correlated_sample_set(), sample_set_releaser());
                    (*sample_set)[stream] = image_interface::create_instance_from_librealsense_frame(frame, image_interface::flag::any, nullptr);

                    non_blocking_set_sample(sample_set);
                    //non_blocking_set_sample(get_unique_ptr_with_releaser(image_interface::create_instance_from_librealsense_frame(frame, image_interface::flag::any, nullptr)));
                };

                m_device->set_frame_callback(librealsense_stream, m_stream_callback_per_stream[stream]);

                m_active_sources = rs::source::video;
            }

            //configure motions
            if (m_device->supports(rs::capabilities::motion_events))
            {
                vector<motion_type> all_motions;
                for(auto stream_index = 0; stream_index < static_cast<int32_t>(motion_type::max); stream_index++)
                {
                    all_motions.push_back(static_cast<motion_type>(stream_index));
                }

                vector<motion_type> actual_motions;
                for(auto &motion: all_motions)
                {
                    if (user_actual_config[motion].is_enabled)
                    {
                        actual_motions.push_back(motion);
                    }
                }

                //single callback registration to motion callbacks.
                if(actual_motions.size() > 0)
                {
                    //enable motion from the selected module configuration
                    m_motion_callback = [actual_motions, this](rs::motion_data entry)
                    {
                        std::shared_ptr<correlated_sample_set> sample_set(new correlated_sample_set(), sample_set_releaser());
                        for(auto actual_motion : actual_motions)
                        {
                            auto & actual_motion_sample = (*sample_set)[actual_motion];
                            actual_motion_sample.timestamp = entry.timestamp_data.timestamp;
                            actual_motion_sample.type = actual_motion;
                            actual_motion_sample.frame_number = entry.timestamp_data.frame_number;
                            actual_motion_sample.data[0] = entry.axes[0];
                            actual_motion_sample.data[1] = entry.axes[1];
                            actual_motion_sample.data[2] = entry.axes[2];
                        }

                        non_blocking_set_sample(sample_set);
                    };

                    m_device->enable_motion_tracking(m_motion_callback);

                    if(m_active_sources == rs::source::video)
                    {
                        m_active_sources = rs::source::all_sources;
                    }
                    else // none sources
                    {
                        m_active_sources = rs::source::motion_data;
                    }
                }
            }

            m_device->start(m_active_sources);

            return status_no_error;
        }

        status pipeline_async_impl::stop()
        {
            {
                std::lock_guard<std::mutex> state_guard(samples_consumers_lock);
                //pipeline resources release
                m_samples_consumers.clear();
            }

            //TODO: close lrs with raii object, reset it here
            //librealsense resources
            m_device->stop(m_active_sources);
            m_device = nullptr;
            m_active_sources = static_cast<rs::source>(0);
            m_stream_callback_per_stream.clear();
            m_motion_callback = nullptr;

            return status_no_error;
        }

        status pipeline_async_impl::reset()
        {
            //TODO : pipeline state dependent lrs resources reset

            m_samples_consumers.clear();

            return pipeline_common::reset();
        }

        void pipeline_async_impl::non_blocking_set_sample(std::shared_ptr<correlated_sample_set> sample_set)
        {
            std::lock_guard<std::mutex> state_guard(samples_consumers_lock);
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

        pipeline_async_impl::~pipeline_async_impl()
        {

        }
    }
}

