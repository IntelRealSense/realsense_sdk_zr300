// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <vector>
#include "rs/core/projection_interface.h"
#include "rs/utils/librealsense_conversion_utils.h"
#include "rs/utils/log_utils.h"
#include "device_manager.h"

using namespace std;
using namespace rs::utils;

namespace rs
{
    namespace core
    {
        device_manager::device_manager(rs::device *device,
                                       const video_module_interface::supported_module_config & config,
                                       std::function<void(std::shared_ptr<correlated_sample_set> sample_set)> non_blocking_notify_sample) :
            m_device(device),
            m_device_config_guard(nullptr),
            m_device_streaming_guard(nullptr)
        {
            if(!m_device)
            {
                throw std::runtime_error("device is not initialized");
            }

            if(!non_blocking_notify_sample)
            {
                throw std::runtime_error("sample notification callback is not initialized");
            }

            video_module_interface::actual_module_config actual_config = {};
            if(!is_there_a_satisfying_device_mode(config, actual_config))
            {
                throw std::runtime_error("no valid device configuration");
            }

            m_device_config_guard.reset(new device_config_guard(m_device, actual_config, non_blocking_notify_sample));

            m_actual_config = actual_config;
            m_projection.reset();

            //create projection object if the streams are relevant
            if(m_device->is_stream_enabled(rs::stream::color) && m_device->is_stream_enabled(rs::stream::depth))
            {
                try
                {
                    rs::core::intrinsics color_intrin = rs::utils::convert_intrinsics(m_device->get_stream_intrinsics(rs::stream::color));
                    rs::core::intrinsics depth_intrin = rs::utils::convert_intrinsics(m_device->get_stream_intrinsics(rs::stream::depth));
                    rs::core::extrinsics extrinsics = rs::utils::convert_extrinsics(m_device->get_extrinsics(rs::stream::depth, rs::stream::color));
                    m_projection.reset(rs::core::projection_interface::create_instance(&color_intrin, &depth_intrin, &extrinsics));
                }
                catch(const std::exception & ex)
                {
                    LOG_ERROR("failed to create projection object, error : " << ex.what());
                }
            }

            m_actual_config.projection = m_projection.get();
        }

        void device_manager::start()
        {
            rs::source source_type;
            if(!does_config_contains_valid_source_type(m_actual_config, source_type))
            {
                throw std::runtime_error("not valid source to configure");
            }
            m_device_streaming_guard.reset(new rs::core::device_streaming_guard(m_device, source_type));
        }

        void device_manager::stop()
        {
            m_device_streaming_guard.reset();
        }

        rs::device * device_manager::get_underlying_device()
        {
            return m_device;
        }

        void device_manager::query_current_config(video_module_interface::actual_module_config & current_config) const
        {
            current_config = m_actual_config;
        }

        const video_module_interface::actual_module_config device_manager::create_actual_config_from_supported_config(
            const video_module_interface::supported_module_config & supported_config) const
        {
            video_module_interface::actual_module_config actual_config = m_actual_config;
            for(uint32_t stream_index = 0; stream_index < static_cast<uint32_t>(stream_type::max); ++stream_index)
            {
                if(!supported_config.image_streams_configs[stream_index].is_enabled)
                {
                    actual_config.image_streams_configs[stream_index] = {};
                }
            }

            for(uint32_t motion_index = 0; motion_index < static_cast<uint32_t>(motion_type::max); ++motion_index)
            {
                if(!supported_config.motion_sensors_configs[motion_index].is_enabled)
                {
                    actual_config.motion_sensors_configs[motion_index] = {};
                }
            }

            return actual_config;
        }

        bool device_manager::is_there_a_satisfying_device_mode(const video_module_interface::supported_module_config& given_config,
                                                               video_module_interface::actual_module_config& actual_config) const
        {
            actual_config = {};
            for(uint32_t stream_index = 0; stream_index < static_cast<uint32_t>(stream_type::max); stream_index++)
            {
                if(!given_config.image_streams_configs[stream_index].is_enabled)
                {
                    continue;
                }
                video_module_interface::actual_image_stream_config & actual_stream = actual_config.image_streams_configs[stream_index];
                actual_stream.is_enabled = true;

                bool is_current_required_stream_satisfied = false;
                auto librealsense_stream = convert_stream_type(static_cast<stream_type>(stream_index));
                for (auto mode_index = 0; mode_index < m_device->get_stream_mode_count(librealsense_stream); mode_index++)
                {
                    int width, height, frame_rate;
                    format librealsense_format;
                    m_device->get_stream_mode(librealsense_stream, mode_index, width, height, librealsense_format, frame_rate);
                    if((given_config.image_streams_configs[stream_index].size.width == 0 ||
                        given_config.image_streams_configs[stream_index].size.width == width) &&
                       (given_config.image_streams_configs[stream_index].size.height == 0 ||
                        given_config.image_streams_configs[stream_index].size.height == height) &&
                       (given_config.image_streams_configs[stream_index].frame_rate == 0 ||
                        given_config.image_streams_configs[stream_index].frame_rate == frame_rate))
                    {
                        is_current_required_stream_satisfied = true;

                        actual_stream.size.width = width;
                        actual_stream.size.height = height;
                        actual_stream.frame_rate = static_cast<float>(frame_rate);

                        rs::intrinsics stream_intrinsics = {};
                        try
                        {
                            stream_intrinsics = m_device->get_stream_intrinsics(librealsense_stream);
                        }
                        catch(const std::exception & ex)
                        {
                            LOG_ERROR("failed to create intrinsics to stream : " << stream_index << ", error : " << ex.what());
                        }
                        actual_config.image_streams_configs[stream_index].intrinsics = convert_intrinsics(stream_intrinsics);
                        rs::extrinsics depth_to_stream_extrinsics = {};
                        try
                        {
                            depth_to_stream_extrinsics = m_device->get_extrinsics(rs::stream::depth, librealsense_stream);
                        }
                        catch(const std::exception & ex)
                        {
                            //TODO : FISHEYE extrinsics will throw exception on uncalibrated camera, need to handle...
                            LOG_ERROR("failed to create extrinsics from depth to stream : " << stream_index << ", error : " << ex.what());
                        }
                        actual_config.image_streams_configs[stream_index].extrinsics = convert_extrinsics(depth_to_stream_extrinsics);

                        rs::extrinsics motion_extrinsics_from_stream = {};
                        try
                        {
                            motion_extrinsics_from_stream = m_device->get_motion_extrinsics_from(librealsense_stream);
                        }
                        catch(const std::exception & ex)
                        {
                            LOG_ERROR("failed to create motion extrinsics from stream : " << stream_index << ", error : " << ex.what());
                        }
                        actual_config.image_streams_configs[stream_index].extrinsics_motion = convert_extrinsics(motion_extrinsics_from_stream);

                        actual_config.image_streams_configs[stream_index].is_enabled = true;

                        break;         
                    }
                }

                if(!is_current_required_stream_satisfied)
                {
                    return false;
                }
            }

            //check for motion configuration
            rs::motion_intrinsics motion_intrinsics = {};
            rs::extrinsics motion_extrinsics_from_depth = {};
            if(m_device->supports(capabilities::motion_events))
            {
                try
                {
                    motion_intrinsics = m_device->get_motion_intrinsics();
                }
                catch(const std::exception & ex)
                {
                    LOG_ERROR("failed to create motion intrinsics, error : " << ex.what());
                }

                try
                {
                    motion_extrinsics_from_depth = m_device->get_motion_extrinsics_from(rs::stream::depth);
                }
                catch(const std::exception & ex)
                {
                    LOG_ERROR("failed to create extrinsics from depth to motion, error : " << ex.what());
                }
            }

            for(uint32_t motion_index = 0; motion_index < static_cast<uint32_t>(motion_type::max); ++motion_index)
            {
                if(!given_config.motion_sensors_configs[motion_index].is_enabled)
                {
                    continue;
                }

                if(!m_device->supports(capabilities::motion_events))
                {
                    return false; // the configuration requires motion event but the device dont support them.
                }

                motion_type motion = static_cast<motion_type>(motion_index);
                if(given_config.motion_sensors_configs[motion_index].is_enabled)
                {
                    actual_config.motion_sensors_configs[motion_index].flags = given_config.motion_sensors_configs[motion_index].flags;

                    switch (motion) {
                    case motion_type::accel:
                        actual_config.motion_sensors_configs[motion_index].intrinsics = convert_motion_device_intrinsics(motion_intrinsics.acc);
                        actual_config.motion_sensors_configs[motion_index].sample_rate = 250;
                        break;
                    case motion_type::gyro:
                        actual_config.motion_sensors_configs[motion_index].intrinsics = convert_motion_device_intrinsics(motion_intrinsics.gyro);
                        actual_config.motion_sensors_configs[motion_index].sample_rate = 200;
                        break;
                    default:
                        throw std::runtime_error("unknown motion type, can't translate intrinsics");
                    }

                    actual_config.motion_sensors_configs[motion_index].extrinsics = convert_extrinsics(motion_extrinsics_from_depth);
                    actual_config.motion_sensors_configs[motion_index].is_enabled = true;
                }
            }

            auto actual_device_name = m_device->get_name();
            std::memcpy(actual_config.device_info.name, actual_device_name, std::strlen(actual_device_name));

            return true;
        }

        bool device_manager::does_config_contains_valid_source_type(const video_module_interface::actual_module_config& config, rs::source & source_type) const
        {
            bool is_any_stream_enabled = false, is_any_motion_enabled = false;
            for(uint32_t stream_index = 0; stream_index < static_cast<uint32_t>(stream_type::max); stream_index++)
            {
                if(config.image_streams_configs[stream_index].is_enabled)
                {
                    is_any_stream_enabled = true;
                    source_type = rs::source::video;
                    break;
                }
            }
            for(uint32_t motion_index = 0; motion_index < static_cast<uint32_t>(motion_type::max); motion_index++)
            {
                if(config.motion_sensors_configs[motion_index].is_enabled)
                {
                    is_any_motion_enabled = true;
                    if(is_any_stream_enabled)
                    {
                        source_type = rs::source::all_sources;
                    }
                    else
                    {
                        source_type = rs::source::motion_data;
                    }
                    break;
                }
            }
            return is_any_stream_enabled || is_any_motion_enabled;
        }

        device_manager::~device_manager()
        {
            //stop streaming before disabling each stream
            m_device_streaming_guard.reset();
            m_device_config_guard.reset();
        }
    }
}

