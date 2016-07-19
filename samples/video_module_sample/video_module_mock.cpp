// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "video_module_mock.h"
#include <iostream>
#include <cstring>
#include <vector>

using namespace rs::core;
using namespace rs::utils;

namespace rs
{
    namespace mock
    {

        video_module_mock::video_module_mock(bool is_complete_sample_set_required):
            m_processing_handler(nullptr),
            m_projection(nullptr),
            m_is_complete_sample_set_required(is_complete_sample_set_required) {}

        int32_t video_module_mock::query_module_uid()
        {
            return CONSTRUCT_UID('M', 'O', 'C', 'K');
        }

        status video_module_mock::query_supported_module_config(int32_t idx, supported_module_config &supported_config)
        {
            switch (idx)
            {
                case 0:
                {
                    const char device_name[] = "Intel RealSense ZR300";
                    std::memcpy(supported_config.device_name, device_name, std::strlen(device_name));

                    supported_config.concurrent_samples_count = 1;
                    supported_config.complete_sample_set_required = m_is_complete_sample_set_required;

                    video_module_interface::supported_image_stream_config & color_desc = supported_config[stream_type::color];
                    color_desc.min_size.width = 640;
                    color_desc.min_size.height = 480;
                    color_desc.ideal_size.width = 640;
                    color_desc.ideal_size.height = 480;
                    color_desc.ideal_frame_rate = 60;
                    color_desc.minimal_frame_rate = 60;
                    color_desc.flags = sample_flags::none;
                    color_desc.preset = preset_type::default_config;
                    color_desc.is_enabled = true;

                    video_module_interface::supported_image_stream_config & depth_desc = supported_config[stream_type::depth];
                    depth_desc.min_size.width = 628;
                    depth_desc.min_size.height = 468;
                    depth_desc.ideal_size.width = 628;
                    depth_desc.ideal_size.height = 468;
                    depth_desc.ideal_frame_rate = 60;
                    depth_desc.minimal_frame_rate = 60;
                    depth_desc.flags = sample_flags::none;
                    depth_desc.preset = preset_type::default_config;
                    depth_desc.is_enabled = true;

                    video_module_interface::supported_motion_sensor_config & accel_desc = supported_config[motion_type::accel];
                    accel_desc.flags = sample_flags::none;
                    accel_desc.is_enabled = true;

                    video_module_interface::supported_motion_sensor_config & gyro_desc = supported_config[motion_type::gyro];
                    gyro_desc.flags = sample_flags::none;
                    gyro_desc.is_enabled = true;

                    break;
                }
                default:
                    return status_item_unavailable;
            }

            return status_no_error;
        }

        status video_module_mock::query_current_module_config(actual_module_config &module_config)
        {
            module_config = m_current_module_config;
            return status_no_error;
        }

        status video_module_mock::set_module_config(const actual_module_config &module_config)
        {
            m_current_module_config = module_config;

            //config internal module

            return status_no_error;
        }

        status video_module_mock::process_sample_set_sync(correlated_sample_set *sample_set)
        {
            if(!sample_set)
            {
                return status_data_not_initialized;
            }

            std::vector<stream_type> streams =  { stream_type::depth,
                                                  stream_type::color
                                                };
            for(auto &stream : streams)
            {
                if(!m_current_module_config[stream].is_enabled)
                    continue;

                smart_ptr<image_interface> image = (*sample_set)[stream];

                if(image)
                {
                    std::cout<< "processing stream " << static_cast<int32_t>(image->query_stream_type()) <<std::endl;
                }
            }

            return status_no_error;
        }

        status video_module_mock::process_sample_set_async(correlated_sample_set *sample_set)
        {
            if(!sample_set)
            {
                return status_data_not_initialized;
            }

            std::vector<stream_type> streams =  { stream_type::depth,
                                                  stream_type::color
                                                };
            for(auto stream : streams)
            {
                if(!m_current_module_config[stream].is_enabled)
                    continue;

                smart_ptr<image_interface> image = (*sample_set)[stream];

                if(image)
                {
                    std::cout<< "processing stream " << static_cast<int32_t>(image->query_stream_type()) <<std::endl;
                }
            }

            std::vector<motion_type> motions =  { motion_type::accel,
                                                  motion_type::gyro
                                                };

            for(auto motion: motions)
            {
                if(!m_current_module_config[motion].is_enabled)
                    continue;

                motion_sample sample = (*sample_set)[motion];

                std::cout<< "processing motion type :" << static_cast<int32_t>(motion)
                         << "\tx: " << sample.data[0] << "\ty: " << sample.data[1] << "\tz: " << sample.data[2]<<std::endl;
            }

            if(m_processing_handler != nullptr)
            {
                m_processing_handler->process_sample_complete(this, sample_set);
            }

            return status_no_error;
        }

        status video_module_mock::register_event_hander(video_module_interface::processing_event_handler *handler)
        {
            if(m_processing_handler != nullptr)
            {
                return status_handle_invalid;
            }
            m_processing_handler = handler;
            return status_no_error;
        }

        status video_module_mock::unregister_event_hander(video_module_interface::processing_event_handler *handler)
        {
            if(m_processing_handler != handler)
            {
                return status_handle_invalid;
            }

            m_processing_handler = nullptr;
            return status_no_error;
        }

        void video_module_mock::set_projection(rs::core::projection * projection)
        {
            m_projection = projection;
        }

        video_module_control_interface * video_module_mock::query_video_module_control()
        {
            return nullptr;
        }

    }
}

