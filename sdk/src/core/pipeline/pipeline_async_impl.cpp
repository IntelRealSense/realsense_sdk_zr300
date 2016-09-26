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
            m_current_state(state::unconfigured),
            m_pipeline_config({}),
            m_streaming_device_manager(nullptr)
        {
            try
            {
                if(!playback_file_path)
                {
                    // initiate context of a real device
                    m_context.reset(new context());
                }
                else
                {
                    // initiate context from a playback file
                    m_context.reset(new rs::playback::context(playback_file_path));
                }
            }
            catch(std::exception& ex)
            {
                LOG_ERROR("failed to create context : " << ex.what());
                throw ex;
            }
        }


        status pipeline_async_impl::add_cv_module(video_module_interface * cv_module)
        {
            std::lock_guard<std::mutex> state_guard(m_state_lock);
            switch(m_current_state)
            {
                case state::streaming:
                case state::configured:
                    return status_wrong_state;
                case state::unconfigured:
                default:
                    break;
            }

            if(!cv_module)
            {
                return status_data_not_initialized;
            }

            if(std::find_if(m_cv_modules.begin(), m_cv_modules.end(),
                            [cv_module] (video_module_interface * checked_module)-> bool
                                {
                                    return cv_module->query_module_uid() == checked_module->query_module_uid();
                                }
                            ) != m_cv_modules.end())
            {
                return status_param_inplace;
            }

            m_cv_modules.push_back(cv_module);
            return status_no_error;
        }

        status pipeline_async_impl::query_cv_module(uint32_t index, video_module_interface ** cv_module) const
        {
            std::lock_guard<std::mutex> state_guard(m_state_lock);

            if(m_cv_modules.size() <= index || index < 0)
            {
                return status_value_out_of_range;
            }

            if(cv_module == nullptr)
            {
                return status_handle_invalid;
            }


            *cv_module = m_cv_modules.at(index);
            return status_no_error;
        }

        status pipeline_async_impl::query_available_config(uint32_t index, pipeline_async_interface::pipeline_config & available_config) const
        {
            std::lock_guard<std::mutex> state_guard(m_state_lock);
            switch(m_current_state)
            {
                case state::streaming:
                    return status_wrong_state;
                case state::configured:
                case state::unconfigured:
                default:
                    break;
            }

            //TODO : optimize by caching the configs?
            std::vector<pipeline_async_interface::pipeline_config> available_configs;
            auto configs_intersection_status = get_intersecting_modules_config(available_configs);
            if(configs_intersection_status < status_no_error)
            {
                LOG_ERROR("failed getting intersecting cv modules configuration");
                return configs_intersection_status;
            }

            /*auto filterout_configs_status = filterout_configs_unsupported_the_context(available_configs);
            if(filterout_configs_status  < status_no_error)
            {
                LOG_ERROR("failed to filter out configuration againt the actual context");
                return filterout_configs_status ;
            }*/

            //validate given index
            if(available_configs.size() <= index || index < 0)
            {
                return status_value_out_of_range;
            }

            available_config = available_configs.at(index);
            return status_no_error;
        }

        status pipeline_async_impl::set_config(const pipeline_async_interface::pipeline_config & pipeline_config)
        {
            std::lock_guard<std::mutex> state_guard(m_state_lock);
            switch(m_current_state)
            {
                case state::streaming:
                    return status_wrong_state;
                case state::configured:
                case state::unconfigured:
                default:
                    break;
            }

            //TODO : validate that the config is one of the available configs
            //TODO : validate that the config is valid by librealsense
            //TODO : commit the pipeline changes only if all the modules successfully set.

            m_modules_configs.clear();
            m_pipeline_config = {};

            //set the configuration on the cv modules
            status module_config_status = status_no_error;
            for (auto cv_module : m_cv_modules)
            {
                video_module_interface::actual_module_config satisfying_config = {};
                video_module_interface::supported_module_config satisfying_config_extended_info = {};
                if(is_theres_satisfying_module_config(cv_module, pipeline_config.module_config, satisfying_config, satisfying_config_extended_info))
                {
                    auto status = cv_module->set_module_config(satisfying_config);
                    if(status < status_no_error)
                    {
                        LOG_ERROR("failed to set configuration on module id : " << cv_module->query_module_uid());
                        module_config_status = status;
                        break;
                    }
                    //save the module configuration internaly
                    m_modules_configs[cv_module] = { satisfying_config, satisfying_config_extended_info };
                }
                else
                {
                    LOG_ERROR("no available configuration for module id : " << cv_module->query_module_uid());
                    module_config_status = status_match_not_found;
                    break;
                }
            }

            //if there was a failure to set one of modules, fallback by resetting all modules
            if(module_config_status < status_no_error)
            {
                for (auto cv_module : m_cv_modules)
                {
                    if(cv_module->query_video_module_control())
                    {
                        cv_module->query_video_module_control()->reset();
                    }
                }

                m_modules_configs.clear();
                m_current_state = state::unconfigured;
                return module_config_status;
            }

            m_pipeline_config = pipeline_config;
            m_current_state = state::configured;
            return module_config_status;
        }

        status pipeline_async_impl::query_current_config(pipeline_async_interface::pipeline_config & current_pipeline_config) const
        {
            std::lock_guard<std::mutex> state_guard(m_state_lock);
            //TODO : take directly from the device instead of aggregating from the modules?
            //aggregate all modules configurations to a single pipeline config
            status query_current_module_config_status = status_no_error;

            current_pipeline_config = {};
            video_module_interface::actual_module_config & aggregated_module_config = current_pipeline_config.module_config;
            for (auto cv_module : m_cv_modules)
            {
                video_module_interface::actual_module_config current_module_config = {};
                auto status = cv_module->query_current_module_config(current_module_config);
                if(status < status_no_error)
                {
                    LOG_ERROR("failed to set configuration on module id : " << cv_module->query_module_uid());
                    current_pipeline_config = {};
                    query_current_module_config_status = status;
                    break;
                }

                //add any enabled configurations to the aggregated configuration (TODO : extract to general util?)
                for(uint32_t i = 0; i < static_cast<uint32_t>(stream_type::max); ++i)
                {
                    if(current_module_config.image_streams_configs[i].is_enabled)
                    {
                        aggregated_module_config.image_streams_configs[i] = current_module_config.image_streams_configs[i];
                    }
                }
                for(uint32_t i = 0; i < static_cast<uint32_t>(motion_type::max); ++i)
                {
                    if(current_module_config.motion_sensors_configs[i].is_enabled)
                    {
                        aggregated_module_config.motion_sensors_configs[i] = current_module_config.motion_sensors_configs[i];
                    }
                }
                aggregated_module_config.projection = current_module_config.projection; //ref counting the projection??
                aggregated_module_config.device_info = current_module_config.device_info;
            }

            return query_current_module_config_status;
        }

        //TODO: currently assumes a single cv module!
        //TODO: extract librealsense configuration validation to filterout_configs_unsupported_the_context
        status pipeline_async_impl::get_intersecting_modules_config(std::vector<pipeline_async_interface::pipeline_config> & intersecting_modules_configs) const
        {
            if(m_cv_modules.size() == 0)
            {
                return status_no_error;
            }
            //configure streams
            std::vector<stream_type> all_streams;
            for(auto stream_index = 0; stream_index < static_cast<int32_t>(stream_type::max); stream_index++)
            {
                   all_streams.push_back(static_cast<stream_type>(stream_index));
            }

            std::vector<motion_type> all_motions;
            for(auto stream_index = 0; stream_index < static_cast<int32_t>(motion_type::max); stream_index++)
            {
                all_motions.push_back(static_cast<motion_type>(stream_index));
            }

            video_module_interface::supported_module_config supported_config = {};

            //working with the first module only for now
            int hardcoded_cv_module_index = 0;

            int cv_module_config_index = 0;
            while(m_cv_modules.at(hardcoded_cv_module_index)->query_supported_module_config(cv_module_config_index, supported_config) >= status_no_error)
            {
                //get the device defined in the supported configuration
                rs::device * device = nullptr;
                for (auto i = 0; i < m_context->get_device_count(); i++)
                {
                    auto checked_device = m_context->get_device(i);
                    if(std::strcmp(checked_device->get_name(), supported_config.device_name) == 0)
                    {
                        device = checked_device;
                        break;
                    }
                }

                if(device == nullptr)
                {
                    LOG_DEBUG("config for device " << supported_config.device_name << " is skipped since its not in the current context");
                    cv_module_config_index++;
                    continue;
                }

                //actual config that will be part of the pipeline config
                video_module_interface::actual_module_config actual_config = {};
                std::memcpy(actual_config.device_info.name, supported_config.device_name, std::strlen(supported_config.device_name));

                // build streams configs structs
                for(auto &stream : all_streams)
                {
                    video_module_interface::supported_image_stream_config & supported_stream_config = supported_config[stream];

                    if (!supported_stream_config.is_enabled)
                        continue;

                    rs::stream librealsense_stream = convert_stream_type(stream);

                    bool is_matching_stream_mode_found = false;
                    auto stream_mode_count = device->get_stream_mode_count(librealsense_stream);
                    for(int i = 0; i < stream_mode_count; i++)
                    {
                        int width, height, frame_rate;
                        rs::format format;
                        device->get_stream_mode(librealsense_stream, i, width, height, format, frame_rate);
                        bool is_acceptable_stream_mode = (width == supported_stream_config.ideal_size.width &&
                                                          height == supported_stream_config.ideal_size.height &&
                                                          frame_rate == supported_stream_config.ideal_frame_rate);
                        if(is_acceptable_stream_mode)
                        {
                            device->enable_stream(librealsense_stream, width, height, format, frame_rate);

                            video_module_interface::actual_image_stream_config &actual_stream_config = actual_config[stream];
                            actual_stream_config.size.width = width;
                            actual_stream_config.size.height= height;
                            actual_stream_config.frame_rate = frame_rate;
                            actual_stream_config.intrinsics = convert_intrinsics(device->get_stream_intrinsics(librealsense_stream));
                            actual_stream_config.extrinsics = convert_extrinsics(device->get_extrinsics(rs::stream::depth, librealsense_stream));
                            actual_stream_config.is_enabled = true;

                            is_matching_stream_mode_found = true;

                            device->disable_stream(librealsense_stream);
                            break;
                        }
                    }

                    if(!is_matching_stream_mode_found)
                    {
                        //log wranning : didnt find matching stream configuration";
                    }
                }

                // build motions configs structs
                for(auto motion: all_motions)
                {
                    auto supported_motion_config = supported_config[motion];

                    if (!supported_motion_config.is_enabled)
                        continue;

                    actual_config[motion].flags = sample_flags::none;
                    actual_config[motion].frame_rate = 0; // not implemented by librealsense
                    actual_config[motion].is_enabled = true;
                }

                pipeline_async_interface::pipeline_config pipeline_config = {};
                pipeline_config.module_config = actual_config;

                cv_module_config_index++;
                intersecting_modules_configs.push_back(pipeline_config);
                //clean struct before next iteration
                supported_config = {};
            }

            return status_no_error;
        }

        bool pipeline_async_impl::is_theres_satisfying_module_config(video_module_interface * cv_module,
                                                                 const video_module_interface::actual_module_config & given_config,
                                                                 video_module_interface::actual_module_config & satisfying_config,
                                                                 video_module_interface::supported_module_config & satisfying_config_extended_info)
        {
            for(auto config_index = 0;; ++config_index)
            {
                video_module_interface::supported_module_config supported_config = {};
                video_module_interface::actual_module_config constructed_actual_config = {};
                if(cv_module->query_supported_module_config(config_index, supported_config) < status_no_error)
                {
                    LOG_ERROR("error : failed to query supported module configuration for index : " << config_index);
                    return false;
                }



                auto is_the_device_in_the_current_config_valid = std::strlen(given_config.device_info.name) == 0 ||
                                                                 (std::strcmp(given_config.device_info.name, supported_config.device_name) == 0);
                if (!is_the_device_in_the_current_config_valid)
                {
                    //skip config due to miss-matching the given config device
                    continue;
                }

                std::memcpy(constructed_actual_config.device_info.name, supported_config.device_name, std::strlen(supported_config.device_name));

                bool are_all_streams_in_the_current_config_satisfied = true;
                for(uint32_t stream_index = 0; stream_index < static_cast<uint32_t>(stream_type::max); ++stream_index)
                {
                    video_module_interface::supported_image_stream_config & stream_config = supported_config.image_streams_configs[stream_index];
                    if(stream_config.is_enabled)
                    {
                        const video_module_interface::actual_image_stream_config & given_stream_config = given_config.image_streams_configs[stream_index];
                        video_module_interface::actual_image_stream_config & constructed_stream_config = constructed_actual_config.image_streams_configs[stream_index];
                        //compare the stream with the given config
                        //TODO : compares to the ideal resulotion and fps, need to check the min resulotion and f
                        bool is_satisfying_stream_config = given_stream_config.is_enabled &&
                                                           stream_config.ideal_size.width == given_stream_config.size.width &&
                                                           stream_config.ideal_size.height == given_stream_config.size.height &&
                                                           stream_config.ideal_frame_rate == given_stream_config.frame_rate;

                        if(is_satisfying_stream_config)
                        {
                            constructed_stream_config.is_enabled = true;
                            constructed_stream_config.size.width = given_stream_config.size.width;
                            constructed_stream_config.size.height = given_stream_config.size.height;
                            constructed_stream_config.frame_rate = given_stream_config.frame_rate;
                            constructed_stream_config.flags = given_stream_config.flags;
                        }
                        else
                        {
                            are_all_streams_in_the_current_config_satisfied = false;
                            break; //out of the streams loop
                        }
                    }
                }
                if(!are_all_streams_in_the_current_config_satisfied)
                {
                    continue; // the current supported config is not satisfying all streams, try the next one
                }

                bool are_all_motions_in_the_current_config_satisfied = true;
                for(uint32_t motion_index = 0; motion_index < static_cast<uint32_t>(motion_type::max); ++motion_index)
                {
                    video_module_interface::supported_motion_sensor_config & motion_config = supported_config.motion_sensors_configs[motion_index];

                    if(motion_config.is_enabled)
                    {
                        const video_module_interface::actual_motion_sensor_config & given_motion_config = given_config.motion_sensors_configs[motion_index];
                        video_module_interface::actual_motion_sensor_config & constructed_motion_config = constructed_actual_config.motion_sensors_configs[motion_index];

                        if(given_motion_config.is_enabled)
                        {
                            constructed_motion_config.is_enabled = true;
                            constructed_motion_config.frame_rate = given_motion_config.frame_rate;
                            constructed_motion_config.flags = given_motion_config.flags;
                        }
                        else
                        {
                            are_all_motions_in_the_current_config_satisfied = false;
                            break;
                        }
                    }
                }
                if(!are_all_motions_in_the_current_config_satisfied)
                {
                    continue; // the current supported config is not satisfying all motions, try the next one
                }

                // found satisfying config
                satisfying_config = constructed_actual_config;
                satisfying_config_extended_info = supported_config;
                return true;
            }

            return false;
        }

        std::vector<std::tuple<video_module_interface *,
                               video_module_interface::actual_module_config,
                               video_module_interface::supported_module_config>> pipeline_async_impl::get_cv_module_configurations()
        {
            std::vector<std::tuple<video_module_interface *,
                                   video_module_interface::actual_module_config,
                                   video_module_interface::supported_module_config>> cv_modules_configs;

            for(auto cv_module : m_cv_modules)
            {
                cv_modules_configs.push_back(std::make_tuple(cv_module, m_modules_configs.at(cv_module).first, m_modules_configs.at(cv_module).second));
            }

            return cv_modules_configs;
        }

        status pipeline_async_impl::start(callback_handler * app_callbacks_handler)
        {   
            std::lock_guard<std::mutex> state_guard(m_state_lock);
            switch(m_current_state)
            {
                case state::streaming:
                    return status_wrong_state;
                case state::configured:
                case state::unconfigured:
                default:
                    break;
            }

            if(!app_callbacks_handler)
            {
                return status_handle_invalid;
            }

            auto user_actual_config = m_pipeline_config.module_config;
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
                        m_pipeline_config.app_samples_time_sync_mode)));

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
                std::lock_guard<std::mutex> samples_consumers_guard(m_samples_consumers_lock);
                m_samples_consumers = std::move(samples_consumers);
            }

            m_streaming_device_manager = std::move(streaming_device_manager);
            m_current_state = state::streaming;
            return status_no_error;
        }

        status pipeline_async_impl::stop()
        {
            std::lock_guard<std::mutex> state_guard(m_state_lock);
            switch(m_current_state)
            {
                case state::streaming:
                    break;
                case state::configured:
                case state::unconfigured:
                default:
                    return status_wrong_state;
            }

            resources_reset();
            m_current_state = state::configured;
            return status_no_error;
        }

        status pipeline_async_impl::reset()
        {
            std::lock_guard<std::mutex> state_guard(m_state_lock);

            resources_reset();
            m_cv_modules.clear();
            m_modules_configs.clear();
            return status_no_error;
        }

        void pipeline_async_impl::non_blocking_set_sample(std::shared_ptr<correlated_sample_set> sample_set)
        {
            std::lock_guard<std::mutex> samples_consumers_guard(m_samples_consumers_lock);
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
                std::lock_guard<std::mutex> samples_consumers_guard(m_samples_consumers_lock);
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

