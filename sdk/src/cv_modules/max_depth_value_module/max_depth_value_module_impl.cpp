// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <iostream>
#include <thread>
#include <cstring>
#include <vector>

#include "max_depth_value_module_impl.h"
#include "rs/utils/log_utils.h"

#define NOMINMAX

using namespace rs::core;
using namespace rs::utils;

namespace rs
{
    namespace cv_modules
    {
        max_depth_value_module_impl::max_depth_value_module_impl(uint64_t milliseconds_added_to_simulate_larger_computation_time, bool is_async_processing):
            m_current_module_config({}),
            m_processing_handler(nullptr),
            m_is_closing(false),
            m_output_data({}),
            m_input_depth_image(nullptr),
            m_milliseconds_added_to_simulate_larger_computation_time(milliseconds_added_to_simulate_larger_computation_time)
        {
            m_unique_module_id = CONSTRUCT_UID('M', 'A', 'X', 'D');
            m_async_processing = is_async_processing;
            m_time_sync_mode = supported_module_config::time_sync_mode::sync_not_required;
            m_processing_thread = std::thread(&max_depth_value_module_impl::async_processing_loop, this);
        }

        int32_t max_depth_value_module_impl::query_module_uid()
        {
            return m_unique_module_id;
        }

        status max_depth_value_module_impl::query_supported_module_config(int32_t idx, supported_module_config &supported_config)
        {
            std::vector<std::string> supported_cameras = {"Intel RealSense ZR300", "Intel RealSense R200" };
            //validate input index
            if(idx < 0 || idx >= 2)
            {
                return status_item_unavailable;
            }

            //this cv module works with max of 1 image
            supported_config.concurrent_samples_count = 1;

            //this cv module doesn't require any time syncing of samples
            supported_config.samples_time_sync_mode = m_time_sync_mode;

            //supports both sync and async mode of work
            supported_config.async_processing = m_async_processing;

            //this cv module supports several cameras
            supported_cameras[idx].copy(supported_config.device_name, supported_cameras[idx].size());
            supported_config.device_name[supported_cameras[idx].size()] = '\0';

            video_module_interface::supported_image_stream_config & depth_desc = supported_config[stream_type::depth];
            depth_desc.size.width = 640;
            depth_desc.size.height = 480;
            depth_desc.frame_rate = 30;
            depth_desc.flags = sample_flags::none;
            depth_desc.is_enabled = true;
            return status_no_error;
        }

        status max_depth_value_module_impl::query_current_module_config(actual_module_config &module_config)
        {
            module_config = m_current_module_config;
            return status_no_error;
        }

        status max_depth_value_module_impl::set_module_config(const actual_module_config &module_config)
        {
            //check the configuration is valid
            auto depth_config = module_config.image_streams_configs[static_cast<uint32_t>(stream_type::depth)];
            if(depth_config.size.width != 640 || depth_config.size.height != 480 ||
               depth_config.is_enabled == false || depth_config.frame_rate != 30)
            {
                return status_param_unsupported;
            }

            m_current_module_config = module_config;
            return status_no_error;
        }

        status max_depth_value_module_impl::process_sample_set(const correlated_sample_set & sample_set)
        {
            auto depth_image = sample_set.get_unique(stream_type::depth);

            if(!depth_image)
            {
                return status_item_unavailable;
            }

            if(m_async_processing)
            {
                m_input_depth_image.set(std::move(depth_image));
            }
            else
            {
                max_depth_value_output_interface::max_depth_value_output_data output_data = {};
                auto status = process_depth_max_value(std::move(depth_image), output_data);
                if(status < status_no_error)
                {
                    return status;
                }
                m_output_data.set(output_data);
            }
            return status_no_error;
        }

        status max_depth_value_module_impl::register_event_handler(video_module_interface::processing_event_handler *handler)
        {
            if(m_processing_handler != nullptr)
            {
                return status_handle_invalid;
            }
            m_processing_handler = handler;
            return status_no_error;
        }

        status max_depth_value_module_impl::unregister_event_handler(video_module_interface::processing_event_handler *handler)
        {
            if(m_processing_handler != handler)
            {
                return status_handle_invalid;
            }

            m_processing_handler = nullptr;
            return status_no_error;
        }

        rs::core::status max_depth_value_module_impl::flush_resources()
        {
            m_input_depth_image.set(nullptr);
            return status_no_error;
        }

        rs::core::status max_depth_value_module_impl::reset_config()
        {
            return status_no_error;
        }

        max_depth_value_output_interface::max_depth_value_output_data max_depth_value_module_impl::get_max_depth_value_data()
        {
            return m_output_data.blocking_get();
        }

        status max_depth_value_module_impl::process_depth_max_value(
                std::shared_ptr<core::image_interface> depth_image,
                max_depth_value_output_interface::max_depth_value_output_data & output_data)
        {
            if(!depth_image)
            {
                return status_data_not_initialized;
            }

            //calculate max depth value
            uint16_t max_depth_value = std::numeric_limits<uint16_t>::min();
            //protect algorithm exception safety
            try
            {
                const uint8_t * current_pixel = static_cast<const uint8_t * >(depth_image->query_data());
                auto depth_image_info = depth_image->query_info();

                for(auto y = 0; y < depth_image_info.height; y++)
                {
                    for(auto x = 0; x < depth_image_info.width; x+=2)
                    {
                        auto pixel_with_pitch_allignment = y * depth_image_info.pitch + x;
                        //combine 2 uint8_t to uint16_t
                        uint16_t current_pixel_value =  static_cast<uint16_t>(current_pixel[pixel_with_pitch_allignment] | current_pixel[pixel_with_pitch_allignment + 1] << 8);

                        if(current_pixel_value > max_depth_value)
                        {
                            max_depth_value = current_pixel_value ;
                        }
                    }
                }

                //simulate larger computation time
                std::this_thread::sleep_for(std::chrono::milliseconds(m_milliseconds_added_to_simulate_larger_computation_time));
            }
            catch(const std::exception & ex)
            {
                LOG_ERROR(ex.what())
                return status_exec_aborted;
            }

            output_data = { max_depth_value, depth_image->query_frame_number() };
            return status_no_error;
        }

        void max_depth_value_module_impl::async_processing_loop()
        {
            while(!m_is_closing)
            {
                auto current_depth_image = m_input_depth_image.blocking_get();

                if(!current_depth_image)
                {
                    LOG_INFO("Got null imput depth image");
                    continue;
                }

                max_depth_value_output_interface::max_depth_value_output_data output_data;
                auto status = process_depth_max_value(std::move(current_depth_image), output_data);
                if(status < status_no_error)
                {
                    LOG_INFO("failed to process max value, error code :" << status);
                    continue;
                }
                m_output_data.set(output_data);

                if(m_processing_handler)
                {
                    m_processing_handler->module_output_ready(this, nullptr);
                }
            }
        }

        max_depth_value_module_impl::~max_depth_value_module_impl()
        {
            m_is_closing = true;
            m_input_depth_image.set(nullptr);
            if(m_processing_thread.joinable())
            {
                m_processing_thread.join();
            }

            max_depth_value_output_interface::max_depth_value_output_data empty_output_data = {};
            m_output_data.set(empty_output_data);
        }
    }
}

