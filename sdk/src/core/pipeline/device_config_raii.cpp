// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <vector>
#include "rs/utils/librealsense_conversion_utils.h"
#include "rs/utils/log_utils.h"
#include "device_config_raii.h"

using namespace std;
using namespace rs::utils;

namespace rs
{
    namespace core
    {
        device_config_raii::device_config_raii(const video_module_interface::supported_module_config& given_config,
                                               rs::device * device):
            m_device(device)
        {
            if(!m_device)
            {
                throw std::runtime_error("device is not initialized");
            }

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
        }

        device_config_raii::~device_config_raii()
        {
            auto last_native_stream_type = stream_type::fisheye;
            for(uint32_t stream_index = 0; stream_index <= static_cast<uint32_t>(last_native_stream_type); stream_index++)
            {
                auto librealsense_stream = convert_stream_type(static_cast<stream_type>(stream_index));
                if(m_device->is_stream_enabled(librealsense_stream))
                {
                   m_device->disable_stream(librealsense_stream);
                }
            }
        }
    }
}

