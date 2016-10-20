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
            m_device(nullptr),
            m_projection(nullptr),
            m_actual_pipeline_config({}),
            m_user_requested_time_sync_mode(video_module_interface::supported_module_config::time_sync_mode::sync_not_required),
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
                                    return cv_module == checked_module;
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

        status pipeline_async_impl::query_default_config(uint32_t index, video_module_interface::supported_module_config & default_config) const
        {
            //currently supporting a single hardcoded config
            if(index != 0)
            {
                return status_value_out_of_range;
            }

            default_config = get_hardcoded_superset_config();
            return status_no_error;
        }

        status pipeline_async_impl::set_config(const video_module_interface::supported_module_config &config)
        {
            std::lock_guard<std::mutex> state_guard(m_state_lock);
            switch(m_current_state)
            {
                case state::streaming:
                    return status_invalid_state;
                case state::configured:
                case state::unconfigured:
                default:
                    break;
            }
            auto set_config_status = set_config_unsafe(config);
            if(set_config_status == status_no_error)
            {
                m_current_state = state::configured;
            }
            return set_config_status;
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
            current_config = m_actual_pipeline_config;
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
                    else
                    {
                        m_current_state = state::configured;
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
                                  app_callbacks_handler->on_new_sample_set(*sample_set);
                                },
                            m_actual_pipeline_config,
                            m_user_requested_time_sync_mode)));
            }
            // create a samples consumer for each cv module
            for(auto cv_module : m_cv_modules)
            {
                video_module_interface::actual_module_config & actual_module_config = std::get<0>(m_modules_configs[cv_module]);
                bool is_cv_module_async = std::get<1>(m_modules_configs[cv_module]);
                video_module_interface::supported_module_config::time_sync_mode module_time_sync_mode = std::get<2>(m_modules_configs[cv_module]);
                if(!is_cv_module_async)
                {
                    samples_consumers.push_back(std::unique_ptr<samples_consumer_base>(new sync_samples_consumer(
                            [cv_module, app_callbacks_handler](std::shared_ptr<correlated_sample_set> sample_set)
                            {
                                //push to sample_set to the cv module
                                auto status = cv_module->process_sample_set(*sample_set);

                                if(status < status_no_error)
                                {
                                    LOG_ERROR("cv module failed to sync process sample set, error code" << status);
                                    if(app_callbacks_handler)
                                    {
                                        app_callbacks_handler->on_status(status);
                                    }
                                    return;
                                }
                                if(app_callbacks_handler)
                                {
                                    app_callbacks_handler->on_cv_module_process_complete(cv_module);
                                }
                            },
                            actual_module_config,
                            module_time_sync_mode)));
                }
                else //cv_module is async
                {
                    samples_consumers.push_back(std::unique_ptr<samples_consumer_base>(new async_samples_consumer(
                                                                                               app_callbacks_handler,
                                                                                               cv_module,
                                                                                               actual_module_config,
                                                                                               module_time_sync_mode)));
                }
            }

            std::unique_ptr<rs::core::streaming_device_manager> streaming_device_manager;
            try
            {
                streaming_device_manager.reset(new rs::core::streaming_device_manager(
                                                           m_actual_pipeline_config,
                                                           [this](std::shared_ptr<correlated_sample_set> sample_set) { non_blocking_sample_callback(sample_set); },
                                                           m_device));
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

            m_cv_modules.clear();
            m_modules_configs.clear();
            m_actual_pipeline_config = {};
            m_user_requested_time_sync_mode = video_module_interface::supported_module_config::time_sync_mode::sync_not_required;
            m_projection = nullptr;
            m_device = nullptr;
            m_current_state = state::unconfigured;
            return status_no_error;
        }

        rs::device * pipeline_async_impl::get_device()
        {
            return m_device;
        }

        rs::device * pipeline_async_impl::get_device_from_config(const video_module_interface::supported_module_config & config) const
        {
            auto device_count = m_context->get_device_count();
            auto is_any_device_valid = (std::strcmp(config.device_name, "") == 0);
            for(int i = 0; i < device_count; ++i)
            {
                auto device = m_context->get_device(i);
                if(is_any_device_valid || std::strcmp(config.device_name, device->get_name()) == 0)
                {
                    return device;
                }
            }
            return nullptr;
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
                    //finished looping through the supported configs and haven't found a satisfying config
                    return false;
                }

                auto is_the_device_in_the_current_config_valid = std::strlen(given_config.device_name) == 0 ||
                                                                 (std::strcmp(given_config.device_name, supported_config.device_name) == 0);
                        ;
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
                        bool is_satisfying_stream_resolution = stream_config.size.width == given_stream_config.size.width &&
                                                               stream_config.size.height == given_stream_config.size.height;
                        bool is_satisfying_stream_framerate =  stream_config.frame_rate == given_stream_config.frame_rate ||
                                                               stream_config.frame_rate == 0;
                        bool is_satisfying_stream_config = given_stream_config.is_enabled &&
                                                           is_satisfying_stream_resolution &&
                                                           is_satisfying_stream_framerate;

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
                    if(given_config.image_streams_configs[stream_index].size.width == width &&
                       given_config.image_streams_configs[stream_index].size.height == height &&
                       given_config.image_streams_configs[stream_index].frame_rate == frame_rate)
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
                //TODO : uncalibrate cameras will state that it unsupport motion_events, need to handle this...
                if(!device->supports(capabilities::motion_events))
                {
                    return false;
                }
            }
            return true;
        }

        status pipeline_async_impl::enable_device_streams(rs::device * device,
                                                       const video_module_interface::supported_module_config& given_config)
        {
            vector<rs::stream> streams_to_disable_on_failure;
            for(auto stream_index = 0; stream_index < static_cast<uint32_t>(stream_type::max); stream_index++)
            {
                auto stream = static_cast<stream_type>(stream_index);
                if(!given_config.image_streams_configs[stream_index].is_enabled)
                {
                    continue;
                }

                bool was_the_required_stream_enabled = false;
                auto librealsense_stream = convert_stream_type(stream);
                for (auto mode_index = 0; mode_index < device->get_stream_mode_count(librealsense_stream); mode_index++)
                {
                    int width, height, frame_rate;
                    format librealsense_format;
                    device->get_stream_mode(librealsense_stream, mode_index, width, height, librealsense_format, frame_rate);
                    if(given_config.image_streams_configs[stream_index].size.width == width &&
                       given_config.image_streams_configs[stream_index].size.height == height &&
                       given_config.image_streams_configs[stream_index].frame_rate == frame_rate)
                    {
                        //TODO : enable native output buffer for performance
                        device->enable_stream(librealsense_stream,width, height, librealsense_format,frame_rate/*, output_buffer_format::native*/);
                        was_the_required_stream_enabled = true;
                        streams_to_disable_on_failure.push_back(librealsense_stream);
                        break;
                    }
                }
                if(!was_the_required_stream_enabled)
                {
                    for (auto stream : streams_to_disable_on_failure)
                    {
                        device->disable_stream(stream);
                    }
                    return status_init_failed;
                }
            }
            return status_no_error;
        }

        void pipeline_async_impl::non_blocking_sample_callback(std::shared_ptr<correlated_sample_set> sample_set)
        {
            std::lock_guard<std::mutex> samples_consumers_guard(m_samples_consumers_lock);
            for(size_t i = 0; i < m_samples_consumers.size(); ++i)
            {
                m_samples_consumers[i]->notify_sample_set_non_blocking(sample_set);
            }
        }

        void pipeline_async_impl::resources_reset()
        {
            //the order of destruction is critical,
            //the consumers must release all resouces allocated by the device inorder to stop and release the device.
            {
                std::lock_guard<std::mutex> samples_consumers_guard(m_samples_consumers_lock);
                m_samples_consumers.clear();
            }

            // cv modules reset
            for (auto cv_module : m_cv_modules)
            {
                cv_module->flush_resources();
            }

            // must be done after the cv modules reset so that all images will be release prior to stopping the device streaming
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
            depth_desc.size.width = 640;
            depth_desc.size.height = 480;
            depth_desc.frame_rate = 30;
            depth_desc.flags = sample_flags::none;
            depth_desc.is_enabled = true;

            video_module_interface::supported_image_stream_config & color_desc = hardcoded_config[stream_type::color];
            color_desc.size.width = 640;
            color_desc.size.height = 480;
            color_desc.frame_rate = 30;
            color_desc.flags = sample_flags::none;
            color_desc.is_enabled = true;

            video_module_interface::supported_image_stream_config & ir_desc = hardcoded_config[stream_type::infrared];
            ir_desc.size.width = 640;
            ir_desc.size.height = 480;
            ir_desc.frame_rate = 30;
            ir_desc.flags = sample_flags::none;
            ir_desc.is_enabled = true;

            video_module_interface::supported_image_stream_config & ir2_desc = hardcoded_config[stream_type::infrared2];
            ir2_desc.size.width = 640;
            ir2_desc.size.height = 480;
            ir2_desc.frame_rate = 30;
            ir2_desc.flags = sample_flags::none;
            ir2_desc.is_enabled = true;

            video_module_interface::supported_image_stream_config & fisheye_desc = hardcoded_config[stream_type::fisheye];
            fisheye_desc.size.width = 640;
            fisheye_desc.size.height = 480;
            fisheye_desc.frame_rate = 30;
            fisheye_desc.flags = sample_flags::none;
            fisheye_desc.is_enabled = true;

            video_module_interface::supported_motion_sensor_config & accel_desc = hardcoded_config[motion_type::accel];
            accel_desc.flags = sample_flags::none;
            accel_desc.frame_rate = 250;
            accel_desc.is_enabled = true;

            video_module_interface::supported_motion_sensor_config & gyro_desc = hardcoded_config[motion_type::gyro];
            gyro_desc.flags = sample_flags::none;
            gyro_desc.frame_rate = 200;
            gyro_desc.is_enabled = true;

            return hardcoded_config;
        }

        status pipeline_async_impl::set_config_unsafe(const video_module_interface::supported_module_config & config)
        {
            rs::device * device = get_device_from_config(config);
            if(!device)
            {
                 LOG_ERROR("failed to get the device");
                return status_item_unavailable;
            }

            //validate that the config is valid by librealsense
            if(!is_there_a_satisfying_device_mode(device, config))
            {
                return status_match_not_found;
            }

            auto enable_device_streams_status = enable_device_streams(device, config);
            if(enable_device_streams_status < status_no_error)
            {
                return enable_device_streams_status;
            }

            rs::utils::unique_ptr<projection_interface> projection;
            if(device->is_stream_enabled(rs::stream::color) && device->is_stream_enabled(rs::stream::depth))
            {
                try
                {
                    rs::core::intrinsics color_intrin = rs::utils::convert_intrinsics(device->get_stream_intrinsics(rs::stream::color));
                    rs::core::intrinsics depth_intrin = rs::utils::convert_intrinsics(device->get_stream_intrinsics(rs::stream::depth));
                    rs::core::extrinsics extrinsics = rs::utils::convert_extrinsics(device->get_extrinsics(rs::stream::depth, rs::stream::color));
                    projection.reset(rs::core::projection_interface::create_instance(&color_intrin, &depth_intrin, &extrinsics));
                }
                catch(const std::exception & ex)
                {
                    LOG_ERROR("failed to create projection object, error : " << ex.what());
                }
            }

            std::map<video_module_interface *, std::tuple<video_module_interface::actual_module_config,
                                                          bool,
                                                          video_module_interface::supported_module_config::time_sync_mode>> modules_configs;

            //set the configuration on the cv modules
            status module_config_status = status_no_error;
            for (auto cv_module : m_cv_modules)
            {
                video_module_interface::supported_module_config satisfying_config = {};
                if(is_there_a_satisfying_module_config(cv_module, config, satisfying_config))
                {
                    auto actual_module_config = create_actual_config_from_supported_config(satisfying_config, device);

                    //add projection to the configuration
                    actual_module_config.projection = projection.get();

                    auto status = cv_module->set_module_config(actual_module_config);
                    if(status < status_no_error)
                    {
                        LOG_ERROR("failed to set configuration on module id : " << cv_module->query_module_uid());
                        module_config_status = status;
                        break;
                    }

                    //save the module configuration internaly
                    auto module_config = std::make_tuple(actual_module_config, satisfying_config.async_processing, satisfying_config.samples_time_sync_mode);
                    modules_configs[cv_module] = module_config;
                }
                else
                {
                    LOG_ERROR("no available configuration for module id : " << cv_module->query_module_uid());
                    module_config_status = status_match_not_found;
                    break;
                }
            }

            //if there was a failure to set one of modules, fallback by resetting all modules and disabling the device streams
            if(module_config_status < status_no_error)
            {
                //clear the configured modules
                for (auto cv_module : m_cv_modules)
                {
                    cv_module->reset_config();
                }

                auto last_native_stream_type = stream_type::fisheye;
                //disable the device streams
                for(auto stream_index = 0; stream_index <= static_cast<uint32_t>(last_native_stream_type); stream_index++)
                {
                    auto librealsense_stream = convert_stream_type(static_cast<stream_type>(stream_index));
                    if(device->is_stream_enabled(librealsense_stream))
                    {
                       device->disable_stream(librealsense_stream);
                    }
                }
                return module_config_status;
            }

            //commit updated config
            m_modules_configs.swap(modules_configs);
            m_device = device;
            m_actual_pipeline_config = create_actual_config_from_supported_config(config, device);
            m_user_requested_time_sync_mode = config.samples_time_sync_mode;
            m_projection = std::move(projection);
            return status_no_error;
        }

        const video_module_interface::actual_module_config pipeline_async_impl::create_actual_config_from_supported_config(
            const video_module_interface::supported_module_config & supported_config,
            rs::device * device) const
        {
            if(!device)
            {
                throw std::runtime_error("no device, cant create actual config");
            }

            video_module_interface::actual_module_config actual_config = {};
            for(uint32_t stream_index = 0; stream_index < static_cast<uint32_t>(stream_type::max); ++stream_index)
            {
                if(supported_config.image_streams_configs[stream_index].is_enabled)
                {
                    rs::stream librealsense_stream = convert_stream_type(static_cast<stream_type>(stream_index));
                    actual_config.image_streams_configs[stream_index].size = supported_config.image_streams_configs[stream_index].size;
                    actual_config.image_streams_configs[stream_index].frame_rate = supported_config.image_streams_configs[stream_index].frame_rate;
                    actual_config.image_streams_configs[stream_index].flags = supported_config.image_streams_configs[stream_index].flags;
                    rs::intrinsics stream_intrinsics = {};
                    try
                    {
                        stream_intrinsics = device->get_stream_intrinsics(librealsense_stream);
                    }
                    catch(const std::exception & ex)
                    {
                        LOG_ERROR("failed to create intrinsics to stream : " << stream_index << ", error : " << ex.what());
                    }
                    actual_config.image_streams_configs[stream_index].intrinsics = convert_intrinsics(stream_intrinsics);
                    rs::extrinsics depth_to_stream_extrinsics = {};
                    try
                    {
                        depth_to_stream_extrinsics = device->get_extrinsics(rs::stream::depth, librealsense_stream);
                    }
                    catch(const std::exception & ex)
                    {
                        //TODO : FISHEYE extrinsics will throw exception on uncalibrated camera, need to handle...
                        LOG_ERROR("failed to create extrinsics from depth to stream : " << stream_index << ", error : " << ex.what());
                    }
                    actual_config.image_streams_configs[stream_index].extrinsics_depth = convert_extrinsics(depth_to_stream_extrinsics);

                    rs::extrinsics motion_extrinsics_from_stream = {};
                    try
                    {
                        motion_extrinsics_from_stream = device->get_motion_extrinsics_from(librealsense_stream);
                    }
                    catch(const std::exception & ex)
                    {
                        LOG_ERROR("failed to create motion extrinsics from stream : " << stream_index << ", error : " << ex.what());
                    }
                    actual_config.image_streams_configs[stream_index].extrinsics_motion = convert_extrinsics(motion_extrinsics_from_stream);

                    actual_config.image_streams_configs[stream_index].is_enabled = true;
                }
            }

            bool already_took_motion_intrinsics = false;
            rs::motion_intrinsics motion_intrinsics = {};
            bool already_took_motion_extrinsics = false;
            rs::extrinsics motion_extrinsics = {};
            try
            {
                motion_intrinsics = device->get_motion_intrinsics();
            }
            catch(const std::exception & ex)
            {
                LOG_ERROR("failed to create motion intrinsics, error : " << ex.what());
            }

            for(uint32_t motion_index = 0; motion_index < static_cast<uint32_t>(motion_type::max); ++motion_index)
            {
                motion_type motion = static_cast<motion_type>(motion_index);
                if(supported_config.motion_sensors_configs[motion_index].is_enabled)
                {
                    actual_config.motion_sensors_configs[motion_index].sample_rate = supported_config.motion_sensors_configs[motion_index].frame_rate;
                    actual_config.motion_sensors_configs[motion_index].flags = supported_config.motion_sensors_configs[motion_index].flags;
                    if(!already_took_motion_intrinsics)
                    {

                        already_took_motion_intrinsics = true;
                    }

                    switch (motion) {
                    case motion_type::accel:
                        actual_config.motion_sensors_configs[motion_index].intrinsics = convert_motion_device_intrinsic(motion_intrinsics.acc);
                        break;
                    case motion_type::gyro:
                        actual_config.motion_sensors_configs[motion_index].intrinsics = convert_motion_device_intrinsic(motion_intrinsics.gyro);
                        break;
                    default:
                        throw std::runtime_error("unknown motion type, can't translate intrinsics");
                    }

                    if(!already_took_motion_extrinsics)
                    {
                        try
                        {
                            motion_extrinsics = device->get_motion_extrinsics_from(rs::stream::depth);
                        }
                        catch(const std::exception & ex)
                        {
                            LOG_ERROR("failed to create extrinsics from depth to motion, error : " << ex.what());
                        }
                        already_took_motion_extrinsics = true;
                    }
                    actual_config.motion_sensors_configs[motion_index].extrinsics = convert_extrinsics(motion_extrinsics);
                    actual_config.motion_sensors_configs[motion_index].is_enabled = true;
                }
            }

            auto actual_device_name = device->get_name();
            std::memcpy(actual_config.device_info.name, actual_device_name, std::strlen(actual_device_name));

            return actual_config;
        }

        status pipeline_async_impl::set_minimal_supported_configuration()
        {
            const int config_index = 0;
            video_module_interface::supported_module_config default_config = {};
            auto query_available_status = query_default_config(config_index, default_config);
            if(query_available_status < status_no_error)
            {
                LOG_ERROR("failed to query the available configuration, error code" << query_available_status);
                return query_available_status;
            }

            auto reduced_default_config = default_config;

            //reduce the available config only if cv modules were added
            if(m_cv_modules.size() > 0)
            {
                // disable all streams and motions
                for(uint32_t stream_index = 0; stream_index < static_cast<uint32_t>(stream_type::max); ++stream_index)
                {
                    reduced_default_config.image_streams_configs[stream_index].is_enabled = false;
                }
                for(uint32_t motion_index = 0; motion_index < static_cast<uint32_t>(motion_type::max); ++motion_index)
                {
                    reduced_default_config.motion_sensors_configs[motion_index].is_enabled = false;
                }

                //enable the minimum config required by the cv modules
                for(auto module : m_cv_modules)
                {
                    video_module_interface::supported_module_config satisfying_supported_config = {};
                    if(is_there_a_satisfying_module_config(module, default_config, satisfying_supported_config))
                    {
                        for(uint32_t stream_index = 0; stream_index < static_cast<uint32_t>(stream_type::max); ++stream_index)
                        {
                            if(satisfying_supported_config.image_streams_configs[stream_index].is_enabled)
                            {
                                reduced_default_config.image_streams_configs[stream_index].is_enabled = true;
                            }
                        }

                        for(uint32_t motion_index = 0; motion_index < static_cast<uint32_t>(motion_type::max); ++motion_index)
                        {
                            if(satisfying_supported_config.motion_sensors_configs[motion_index].is_enabled)
                            {
                                reduced_default_config.motion_sensors_configs[motion_index].is_enabled = true;
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
                    if(!reduced_default_config.image_streams_configs[stream_index].is_enabled)
                    {
                        reduced_default_config.image_streams_configs[stream_index] = {};
                    }
                }
                for(uint32_t motion_index = 0; motion_index < static_cast<uint32_t>(motion_type::max); ++motion_index)
                {
                    if(!reduced_default_config.motion_sensors_configs[motion_index].is_enabled)
                    {
                        reduced_default_config.motion_sensors_configs[motion_index] = {};
                    }
                }
            }

            //set the updated config
            auto query_set_config_status = set_config_unsafe(reduced_default_config);
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

