// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "rs/utils/librealsense_conversion_utils.h"
#include "rs/utils/log_utils.h"
#include "sample_set_releaser.h"
#include "device_config_guard.h"

using namespace std;
using namespace rs::utils;

namespace rs
{
    namespace core
    {
        device_config_guard::device_config_guard(rs::device * device,
                                                 const video_module_interface::actual_module_config &given_config,
                                                 std::function<void(std::shared_ptr<correlated_sample_set> sample_set)> non_blocking_notify_sample):
            m_device(device),
            m_config(given_config),
            m_non_blocking_notify_sample(non_blocking_notify_sample)
        {
            if(!m_device)
            {
                throw std::runtime_error("device is not initialized");
            }

            if(!m_non_blocking_notify_sample)
            {
                throw std::runtime_error("sample notification callback is not initialized");
            }

            //enable streams
            vector<rs::stream> streams_to_disable_on_failure;
            for(uint32_t stream_index = 0; stream_index < static_cast<uint32_t>(stream_type::max); stream_index++)
            {
                auto stream = static_cast<stream_type>(stream_index);
                if(!given_config.image_streams_configs[stream_index].is_enabled)
                {
                    continue;
                }

                bool was_the_required_stream_enabled = false;
                auto librealsense_stream = convert_stream_type(stream);
                for (auto mode_index = 0; mode_index < m_device->get_stream_mode_count(librealsense_stream); mode_index++)
                {
                    int width, height, frame_rate;
                    format librealsense_format;
                    m_device->get_stream_mode(librealsense_stream, mode_index, width, height, librealsense_format, frame_rate);
                    if(given_config.image_streams_configs[stream_index].size.width == width &&
                       given_config.image_streams_configs[stream_index].size.height == height &&
                       given_config.image_streams_configs[stream_index].frame_rate == frame_rate)
                    {
                        //TODO : enable native output buffer for performance
                        m_device->enable_stream(librealsense_stream,width, height, librealsense_format,frame_rate/*, output_buffer_format::native*/);
                        was_the_required_stream_enabled = true;
                        streams_to_disable_on_failure.push_back(librealsense_stream);
                        break;
                    }
                }
                if(!was_the_required_stream_enabled)
                {
                    for (auto stream : streams_to_disable_on_failure)
                    {
                        m_device->disable_stream(stream);
                    }
                    throw std::runtime_error("failed to configure the device");
                }
            }

            //set streams callbacks
            for(auto stream_index = 0; stream_index < static_cast<int32_t>(stream_type::max); stream_index++)
            {
                if (!given_config.image_streams_configs[stream_index].is_enabled)
                {
                    continue;
                }

                //define callbacks to the actual streams and set them.
                stream_type stream = static_cast<stream_type>(stream_index);
                m_stream_callback_per_stream[stream] = [stream, this](rs::frame frame)
                {
                    std::shared_ptr<correlated_sample_set> sample_set(new correlated_sample_set(), sample_set_releaser());
                    (*sample_set)[stream] = image_interface::create_instance_from_librealsense_frame(frame, image_interface::flag::any);
                    if(m_non_blocking_notify_sample)
                    {
                        m_non_blocking_notify_sample(sample_set);
                    };
                };

                m_device->set_frame_callback(convert_stream_type(stream), m_stream_callback_per_stream[stream]);
            }

            //set motion callback
            if (m_device->supports(rs::capabilities::motion_events))
            {
                bool is_motion_required = false;
                for(auto motion_index = 0; motion_index < static_cast<int32_t>(motion_type::max); motion_index++)
                {
                    if (given_config.motion_sensors_configs[motion_index].is_enabled)
                    {
                        is_motion_required = true;
                        break;
                    }
                }

                //single callback registration to motion callbacks.
                if(is_motion_required)
                {
                    //enable motion from the selected module configuration
                    m_motion_callback = [this](rs::motion_data entry)
                    {
                        std::shared_ptr<correlated_sample_set> sample_set(new correlated_sample_set(), sample_set_releaser());

                        auto actual_motion = convert_motion_type(static_cast<rs::event>(entry.timestamp_data.source_id));

                        //check that the motion type is relevant
                        if(static_cast<int>(actual_motion) == 0)
                        {
                            LOG_ERROR("failed to convert motion type");
                            return;
                        }

                        (*sample_set)[actual_motion].timestamp = entry.timestamp_data.timestamp;
                        (*sample_set)[actual_motion].type = actual_motion;
                        (*sample_set)[actual_motion].frame_number = entry.timestamp_data.frame_number;
                        (*sample_set)[actual_motion].data[0] = entry.axes[0];
                        (*sample_set)[actual_motion].data[1] = entry.axes[1];
                        (*sample_set)[actual_motion].data[2] = entry.axes[2];

                        if(m_non_blocking_notify_sample)
                        {
                            m_non_blocking_notify_sample(sample_set);
                        };
                    };

                    m_device->enable_motion_tracking(m_motion_callback);
                }
            }

        }

        device_config_guard::~device_config_guard()
        {
            for(uint32_t stream_index = 0; stream_index < static_cast<int32_t>(stream_type::max); stream_index++)
            {
                if (m_config.image_streams_configs[stream_index].is_enabled)
                {
                    auto librealsense_stream = convert_stream_type(static_cast<stream_type>(stream_index));
                    if(m_device->is_stream_enabled(librealsense_stream))
                    {
                       m_device->disable_stream(librealsense_stream);
                    }
                }
            }

            m_non_blocking_notify_sample = nullptr;
            m_stream_callback_per_stream.clear();
            m_motion_callback = nullptr;
        }
    }
}

