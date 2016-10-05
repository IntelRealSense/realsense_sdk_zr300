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
            if(!cv_module)
            {
                return status_data_not_initialized;
            }

            std::lock_guard<std::mutex> state_guard(m_state_lock);
            switch(m_current_state)
            {
                case state::streaming:
                case state::configured:
                    return status_invalid_state;
                case state::unconfigured:
                default:
                    break;
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

        status pipeline_async_impl::query_available_config(uint32_t index, video_module_interface::supported_module_config & available_config) const
        {
            std::lock_guard<std::mutex> state_guard(m_state_lock);
            return locked_state_query_available_config(index, available_config);
        }

        status pipeline_async_impl::set_config(const video_module_interface::supported_module_config &config)
        {
            std::lock_guard<std::mutex> state_guard(m_state_lock);
            return locked_state_set_config(config);
        }

        status pipeline_async_impl::query_current_config(video_module_interface::actual_module_config & current_config) const
        {
            std::lock_guard<std::mutex> state_guard(m_state_lock);
            switch(m_current_state)
            {
                case state::unconfigured:
                    return status_invalid_state;
                case state::configured:
                case state::streaming:
                default:
                    break;
            }
            current_config = convert_to_actual_config(m_pipeline_config);
            return status_no_error;
        }

        status pipeline_async_impl::start(callback_handler * app_callbacks_handler)
        {   
            std::lock_guard<std::mutex> state_guard(m_state_lock);
            switch(m_current_state)
            {
                case state::streaming:
                    return status_invalid_state;
                case state::unconfigured:
                {
                    status set_minimal_supported_config_status = set_minimal_supported_configuration();
                    if(set_minimal_supported_config_status < status_no_error)
                    {
                        return set_minimal_supported_config_status;
                    }
                    break;
                }
                case state::configured:
                default:
                    break;
            }

            assert(m_current_state == state::configured && "the pipeline must be in configured state to start");

            std::vector<std::shared_ptr<samples_consumer_base>> samples_consumers;
            if(app_callbacks_handler)
            {
                //application samples consumer creation :
                samples_consumers.push_back(std::unique_ptr<samples_consumer_base>(
                    new sync_samples_consumer(
                            [app_callbacks_handler](std::shared_ptr<correlated_sample_set> sample_set)
                                {
                                  correlated_sample_set copied_sample_set = *sample_set;
                                  copied_sample_set.add_ref();
                                  app_callbacks_handler->on_new_sample_set(&copied_sample_set);
                                },
                            m_pipeline_config)));
            }
            // create a samples consumer for each cv module
            for(auto cv_module : m_cv_modules)
            {
                video_module_interface::supported_module_config & extended_module_config = m_modules_configs[cv_module];

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
                                if(app_callbacks_handler)
                                {
                                    app_callbacks_handler->on_cv_module_process_complete(cv_module->query_module_uid());
                                }
                            },
                            extended_module_config)));
                }
                else //cv_module is async
                {
                    samples_consumers.push_back(std::unique_ptr<samples_consumer_base>(new async_samples_consumer(
                                                                                               app_callbacks_handler,
                                                                                               cv_module,
                                                                                               extended_module_config)));
                }
            }

            std::unique_ptr<rs::core::streaming_device_manager> streaming_device_manager;
            try
            {
                streaming_device_manager.reset(new rs::core::streaming_device_manager(
                                                           m_pipeline_config,
                                                           [this](std::shared_ptr<correlated_sample_set> sample_set) { non_blocking_set_sample_callback(sample_set); },
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
                    return status_invalid_state;
            }

            resources_reset();
            m_current_state = state::configured;
            return status_no_error;
        }

        status pipeline_async_impl::reset()
        {
            std::lock_guard<std::mutex> state_guard(m_state_lock);
            resources_reset();

            // configuration reset
            m_cv_modules.clear();
            m_modules_configs.clear();
            m_pipeline_config = {};
            return status_no_error;
        }

        rs::device * pipeline_async_impl::get_device(const video_module_interface::supported_module_config & config) const
        {
            if(m_context->get_device_count() < 1)
            {
                return nullptr;
            }

            //TODO : check device name againt config and support different devices
            return m_context->get_device(0);
        }

        bool pipeline_async_impl::is_there_a_satisfying_module_config(video_module_interface * cv_module,
                                                                      const video_module_interface::supported_module_config & given_config,
                                                                      video_module_interface::supported_module_config & satisfying_config) const
        {
            for(auto config_index = 0;; ++config_index)
            {
                video_module_interface::supported_module_config supported_config = {};
                if(cv_module->query_supported_module_config(config_index, supported_config) < status_no_error)
                {
                    //finished looping through the supported configs
                    return false;
                }

                auto is_the_device_in_the_current_config_valid = std::strlen(given_config.device_name) == 0 ||
                                                                 (std::strcmp(given_config.device_name, supported_config.device_name) == 0);
                if (!is_the_device_in_the_current_config_valid)
                {
                    //skip config due to miss-matching the given config device
                    continue;
                }

                bool are_all_streams_in_the_current_config_satisfied = true;
                for(uint32_t stream_index = 0; stream_index < static_cast<uint32_t>(stream_type::max); ++stream_index)
                {
                    video_module_interface::supported_image_stream_config & stream_config = supported_config.image_streams_configs[stream_index];
                    if(stream_config.is_enabled)
                    {
                        const video_module_interface::supported_image_stream_config & given_stream_config = given_config.image_streams_configs[stream_index];
                        //compare the stream with the given config
                        //TODO : compares to the ideal resulotion and fps, need to check the min resulotion and f
                        bool is_satisfying_stream_config = given_stream_config.is_enabled &&
                                                           stream_config.ideal_size.width == given_stream_config.ideal_size.width &&
                                                           stream_config.ideal_size.height == given_stream_config.ideal_size.height &&
                                                           stream_config.ideal_frame_rate == given_stream_config.ideal_frame_rate;

                        if(!is_satisfying_stream_config)
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
                        const video_module_interface::supported_motion_sensor_config & given_motion_config = given_config.motion_sensors_configs[motion_index];

                        if(!given_motion_config.is_enabled)
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
                satisfying_config = supported_config;
                return true;
            }

            return false;
        }

        bool pipeline_async_impl::is_there_a_satisfying_device_mode(rs::device * device,
                                                                    const video_module_interface::supported_module_config& given_config) const
        {
            for(auto stream_index = 0; stream_index < static_cast<uint32_t>(stream_type::max); stream_index++)
            {
                auto stream = static_cast<stream_type>(stream_index);
                if(!given_config.image_streams_configs[stream_index].is_enabled)
                {
                    continue;
                }

                bool is_there_satisfying_device_stream_mode = false;
                auto librealsense_stream = convert_stream_type(stream);
                for (auto mode_index = 0; mode_index < device->get_stream_mode_count(librealsense_stream); mode_index++)
                {
                    int width, height, frame_rate;
                    format librealsense_format;
                    device->get_stream_mode(librealsense_stream, mode_index, width, height, librealsense_format, frame_rate);
                    if(given_config.image_streams_configs[stream_index].ideal_size.width == width &&
                       given_config.image_streams_configs[stream_index].ideal_size.height == height &&
                       given_config.image_streams_configs[stream_index].ideal_frame_rate == frame_rate)
                    {
                        is_there_satisfying_device_stream_mode = true;
                        break;
                    }
                }
                if(!is_there_satisfying_device_stream_mode)
                {
                    return false;
                }
            }
            for(auto motion_index = 0; motion_index < static_cast<uint32_t>(motion_type::max); motion_index++)
            {
                auto motion = static_cast<motion_type>(motion_index);
                if(!given_config.motion_sensors_configs[motion_index].is_enabled)
                {
                    continue;
                }

                if(!device->supports(capabilities::motion_events))
                {
                    return false;
                }
            }
            return true;
        }

        void pipeline_async_impl::non_blocking_set_sample_callback(std::shared_ptr<correlated_sample_set> sample_set)
        {
            std::lock_guard<std::mutex> samples_consumers_guard(m_samples_consumers_lock);
            for(size_t i = 0; i < m_samples_consumers.size(); ++i)
            {
                if(m_samples_consumers[i]->is_sample_set_contains_a_single_required_sample(sample_set))
                {
                    m_samples_consumers[i]->non_blocking_set_sample_set(sample_set);
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

        const video_module_interface::supported_module_config pipeline_async_impl::get_hardcoded_superset_config() const
        {
            video_module_interface::supported_module_config hardcoded_config = {};

            std::string supported_camera = "Intel RealSense ZR300";
            supported_camera.copy(hardcoded_config.device_name, supported_camera.size());
            hardcoded_config.device_name[supported_camera.size()] = '\0';

            hardcoded_config.samples_time_sync_mode = video_module_interface::supported_module_config::time_sync_mode::sync_not_required;

            video_module_interface::supported_image_stream_config & depth_desc = hardcoded_config[stream_type::depth];
            depth_desc.min_size.width = 320;
            depth_desc.min_size.height = 240;
            depth_desc.ideal_size.width = 640;
            depth_desc.ideal_size.height = 480;
            depth_desc.ideal_frame_rate = 30;
            depth_desc.minimal_frame_rate = 30;
            depth_desc.flags = sample_flags::none;
            depth_desc.preset = preset_type::default_config;
            depth_desc.is_enabled = true;

            video_module_interface::supported_image_stream_config & color_desc = hardcoded_config[stream_type::color];
            color_desc.min_size.width = 640;
            color_desc.min_size.height = 480;
            color_desc.ideal_size.width = 640;
            color_desc.ideal_size.height = 480;
            color_desc.ideal_frame_rate = 30;
            color_desc.minimal_frame_rate = 30;
            color_desc.flags = sample_flags::none;
            color_desc.preset = preset_type::default_config;
            color_desc.is_enabled = true;

            video_module_interface::supported_image_stream_config & ir_desc = hardcoded_config[stream_type::infrared];
            ir_desc.min_size.width = 640;
            ir_desc.min_size.height = 480;
            ir_desc.ideal_size.width = 640;
            ir_desc.ideal_size.height = 480;
            ir_desc.ideal_frame_rate = 30;
            ir_desc.minimal_frame_rate = 30;
            ir_desc.flags = sample_flags::none;
            ir_desc.preset = preset_type::default_config;
            ir_desc.is_enabled = true;

            video_module_interface::supported_image_stream_config & ir2_desc = hardcoded_config[stream_type::infrared2];
            ir2_desc.min_size.width = 640;
            ir2_desc.min_size.height = 480;
            ir2_desc.ideal_size.width = 640;
            ir2_desc.ideal_size.height = 480;
            ir2_desc.ideal_frame_rate = 30;
            ir2_desc.minimal_frame_rate = 30;
            ir2_desc.flags = sample_flags::none;
            ir2_desc.preset = preset_type::default_config;
            ir2_desc.is_enabled = true;

            video_module_interface::supported_image_stream_config & fisheye_desc = hardcoded_config[stream_type::fisheye];
            fisheye_desc.min_size.width = 640;
            fisheye_desc.min_size.height = 480;
            fisheye_desc.ideal_size.width = 640;
            fisheye_desc.ideal_size.height = 480;
            fisheye_desc.ideal_frame_rate = 30;
            fisheye_desc.minimal_frame_rate = 30;
            fisheye_desc.flags = sample_flags::none;
            fisheye_desc.preset = preset_type::default_config;
            fisheye_desc.is_enabled = true;

            video_module_interface::supported_motion_sensor_config & accel_desc = hardcoded_config[motion_type::accel];
            accel_desc.flags = sample_flags::none;
            accel_desc.minimal_frame_rate = 250;
            accel_desc.ideal_frame_rate = 250;
            accel_desc.is_enabled = true;

            video_module_interface::supported_motion_sensor_config & gyro_desc = hardcoded_config[motion_type::gyro];
            gyro_desc.flags = sample_flags::none;
            gyro_desc.minimal_frame_rate = 200;
            gyro_desc.ideal_frame_rate = 200;
            gyro_desc.is_enabled = true;

            return hardcoded_config;
        }

        status pipeline_async_impl::locked_state_query_available_config(uint32_t index, video_module_interface::supported_module_config & available_config) const
        {
            switch(m_current_state)
            {
                case state::streaming:
                    return status_invalid_state;
                case state::configured:
                case state::unconfigured:
                default:
                    break;
            }

            //currently supporting a single hardcoded config
            if(index != 0)
            {
                return status_value_out_of_range;
            }

            auto hardcoded_supported_config = get_hardcoded_superset_config();

            //check cv modules are satisfied with the config
            bool are_all_cv_modules_satisfied = true;
            for(auto module : m_cv_modules)
            {
                video_module_interface::supported_module_config satisfying_supported_config = {};
                if(!is_there_a_satisfying_module_config(module, hardcoded_supported_config, satisfying_supported_config))
                {
                    LOG_ERROR("module id " << module->query_module_uid() << "failed to match the available config");
                    are_all_cv_modules_satisfied = false;
                }
            }
            if(!are_all_cv_modules_satisfied)
            {
                return status_match_not_found;
            }

            rs::device * device = get_device(hardcoded_supported_config);
            if(!device)
            {
                return status_item_unavailable;
            }

            if(!is_there_a_satisfying_device_mode(device, hardcoded_supported_config))
            {
                return status_match_not_found;
            }

            std::vector<video_module_interface::supported_module_config> available_configs;
            available_configs.push_back(hardcoded_supported_config);
            available_config = available_configs.at(index);
            return status_no_error;
        }

        status pipeline_async_impl::locked_state_set_config(const video_module_interface::supported_module_config & config)
        {
            switch(m_current_state)
            {
                case state::streaming:
                    return status_invalid_state;
                case state::configured:
                case state::unconfigured:
                default:
                    break;
            }

            rs::device * device = get_device(config);
            if(!device)
            {
                return status_item_unavailable;
            }

            //validate that the config is valid by librealsense
            if(!is_there_a_satisfying_device_mode(device, config))
            {
                return status_match_not_found;
            }

            std::map<video_module_interface *, video_module_interface::supported_module_config> modules_configs;

            //set the configuration on the cv modules
            status module_config_status = status_no_error;
            for (auto cv_module : m_cv_modules)
            {
                video_module_interface::supported_module_config satisfying_config = {};
                if(is_there_a_satisfying_module_config(cv_module, config, satisfying_config))
                {
                    //TODO: add intrinsics and extrinsics
                    auto actual_module_config = convert_to_actual_config(satisfying_config);
                    auto status = cv_module->set_module_config(actual_module_config);
                    if(status < status_no_error)
                    {
                        LOG_ERROR("failed to set configuration on module id : " << cv_module->query_module_uid());
                        module_config_status = status;
                        break;
                    }
                    //save the module configuration internaly
                    modules_configs[cv_module] = satisfying_config;
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

                m_current_state = state::unconfigured;
                return module_config_status;
            }

            //commit updated config
            m_modules_configs.swap(modules_configs);
            m_pipeline_config = config;
            m_current_state = state::configured;
            return module_config_status;
        }

        const video_module_interface::actual_module_config pipeline_async_impl::convert_to_actual_config(
            const video_module_interface::supported_module_config & supported_config) const
        {
            rs::device * device = get_device(supported_config);

            video_module_interface::actual_module_config actual_config = {};
            for(uint32_t stream_index = 0; stream_index < static_cast<uint32_t>(stream_type::max); ++stream_index)
            {
                if(supported_config.image_streams_configs[stream_index].is_enabled)
                {
                    rs::stream librealsense_stream = convert_stream_type(static_cast<stream_type>(stream_index));
                    actual_config.image_streams_configs[stream_index].size = supported_config.image_streams_configs[stream_index].ideal_size;
                    actual_config.image_streams_configs[stream_index].frame_rate = supported_config.image_streams_configs[stream_index].ideal_frame_rate;
                    actual_config.image_streams_configs[stream_index].flags = supported_config.image_streams_configs[stream_index].flags;
                    //actual_config.image_streams_configs[stream_index].intrinsics = convert_intrinsics(device->get_stream_intrinsics(librealsense_stream));
                    //actual_config.image_streams_configs[stream_index].extrinsics = convert_extrinsics(device->get_extrinsics(rs::stream::depth, librealsense_stream));
                    actual_config.image_streams_configs[stream_index].is_enabled = true;
                }
            }
            for(uint32_t i = 0; i < static_cast<uint32_t>(motion_type::max); ++i)
            {
                if(supported_config.motion_sensors_configs[i].is_enabled)
                {
                    actual_config.motion_sensors_configs[i].frame_rate = supported_config.motion_sensors_configs[i].ideal_frame_rate;
                    actual_config.motion_sensors_configs[i].flags = supported_config.motion_sensors_configs[i].flags;
                    /*if(device->is_motion_tracking_active())
                    {
                        actual_config.motion_sensors_configs[i].intrinsics = convert_motion_intrinsics(device->get_motion_intrinsics());
                    }
                    actual_config.motion_sensors_configs[i].extrinsics = convert_extrinsics(device->get_motion_extrinsics_from(rs::stream::depth));
                    */
                    actual_config.motion_sensors_configs[i].is_enabled = true;
                }
            }

            std::memcpy(actual_config.device_info.name, supported_config.device_name, std::strlen(supported_config.device_name));



            return actual_config;
        }

        status pipeline_async_impl::set_minimal_supported_configuration()
        {
            const int config_index = 0;
            video_module_interface::supported_module_config available_config = {};
            auto query_available_status = locked_state_query_available_config(config_index, available_config);
            if(query_available_status < status_no_error)
            {
                LOG_ERROR("failed to query the available configuration, error code" << query_available_status);
                return query_available_status;
            }

            auto reduced_available_config = available_config;

            //reduce the available config only if cv modules were added
            if(m_cv_modules.size() > 0)
            {
                // disable all streams and motions
                for(uint32_t stream_index = 0; stream_index < static_cast<uint32_t>(stream_type::max); ++stream_index)
                {
                    reduced_available_config.image_streams_configs[stream_index].is_enabled = false;
                }
                for(uint32_t motion_index = 0; motion_index < static_cast<uint32_t>(motion_type::max); ++motion_index)
                {
                    reduced_available_config.motion_sensors_configs[motion_index].is_enabled = false;
                }

                //enable the minimum config required by the cv modules
                for(auto module : m_cv_modules)
                {
                    video_module_interface::supported_module_config satisfying_supported_config = {};
                    if(is_there_a_satisfying_module_config(module, available_config, satisfying_supported_config))
                    {
                        for(uint32_t stream_index = 0; stream_index < static_cast<uint32_t>(stream_type::max); ++stream_index)
                        {
                            if(satisfying_supported_config.image_streams_configs[stream_index].is_enabled)
                            {
                                reduced_available_config.image_streams_configs[stream_index].is_enabled = true;
                            }
                        }

                        for(uint32_t motion_index = 0; motion_index < static_cast<uint32_t>(motion_type::max); ++motion_index)
                        {
                            if(satisfying_supported_config.motion_sensors_configs[motion_index].is_enabled)
                            {
                                reduced_available_config.motion_sensors_configs[motion_index].is_enabled = true;
                            }
                        }
                    }
                    else
                    {
                        LOG_ERROR("the default configuration is not supported by a cv module");
                        return status_exec_aborted;
                    }
                }

                //clear disabled configs
                for(uint32_t stream_index = 0; stream_index < static_cast<uint32_t>(stream_type::max); ++stream_index)
                {
                    if(!reduced_available_config.image_streams_configs[stream_index].is_enabled)
                    {
                        reduced_available_config.image_streams_configs[stream_index] = {};
                    }
                }
                for(uint32_t motion_index = 0; motion_index < static_cast<uint32_t>(motion_type::max); ++motion_index)
                {
                    if(!reduced_available_config.motion_sensors_configs[motion_index].is_enabled)
                    {
                        reduced_available_config.motion_sensors_configs[motion_index] = {};
                    }
                }

            }

            //set the updated config
            auto query_set_config_status = locked_state_set_config(reduced_available_config);
            if(query_set_config_status < status_no_error)
            {
                LOG_ERROR("failed to set configuration, error code" << query_set_config_status);
                return query_set_config_status;
            }

            return status_no_error;
        }

        pipeline_async_impl::~pipeline_async_impl()
        {
            resources_reset();
        }
    }
}

