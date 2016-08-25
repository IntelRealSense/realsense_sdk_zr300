// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <iostream>
#include <thread>
#include <cstring>
#include <vector>

#include "max_depth_value_module_impl.h"
#include "rs/utils/log_utils.h"

using namespace rs::core;
using namespace rs::utils;

namespace rs
{
    namespace cv_modules
    {
        max_depth_value_module_impl::max_depth_value_module_impl(uint64_t milliseconds_added_to_simulate_larger_computation_time):
            m_processing_handler(nullptr),
            m_is_closing(false),
            m_output_data({}),
            m_input_depth_image(nullptr),
            m_milliseconds_added_to_simulate_larger_computation_time(milliseconds_added_to_simulate_larger_computation_time)
        {
            m_processing_thread = std::thread(&max_depth_value_module_impl::async_processing_loop, this);
        }

        int32_t max_depth_value_module_impl::query_module_uid()
        {
            return CONSTRUCT_UID('M', 'A', 'X', 'D');
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

            //supports both sync and async mode of work
            supported_config.config_flags = static_cast<supported_module_config::flags>(
                                                            supported_module_config::flags::sync_processing_supported |
                                                            supported_module_config::flags::async_processing_supported);

            //this cv module doesn't require any time syncing of samples
            supported_config.samples_time_sync_mode = supported_module_config::time_sync_mode::sync_not_required;

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

            supported_cameras[idx].copy(supported_config.device_name, supported_cameras[idx].size());
            supported_config.device_name[supported_cameras[idx].size()] = '\0';

            return status_no_error;
        }

        status max_depth_value_module_impl::query_current_module_config(actual_module_config &module_config)
        {
            module_config = m_current_module_config;
            return status_no_error;
        }

        status max_depth_value_module_impl::set_module_config(const actual_module_config &module_config)
        {
            m_current_module_config = module_config;
            return status_no_error;
        }

        status max_depth_value_module_impl::process_sample_set_sync(correlated_sample_set *sample_set)
        {
            if(!sample_set)
            {
                return status_data_not_initialized;
            }

            //its important to set the sample_set in a unique ptr with a releaser since all the ref counted samples in the
            //samples set need to be released out of this scope even if this module is not using them, otherwise there will
            //be a memory leak.
            auto scoped_sample_set = get_unique_ptr_with_releaser(sample_set);

            auto depth_image = scoped_sample_set->take_shared(stream_type::depth);

            if(!depth_image)
            {
                return status_item_unavailable;
            }

            max_depth_value_module_interface::max_depth_value_output_data output_data;

            auto status = process_depth_max_value(std::move(depth_image), output_data);
            if(status < status_no_error)
            {
                return status;
            }

            m_output_data.set(output_data);
            return status_no_error;
        }

        status max_depth_value_module_impl::process_sample_set_async(correlated_sample_set *sample_set)
        {
            if(!sample_set)
            {
                return status_data_not_initialized;
            }

            //its important to set the sample_set in a unique ptr with a releaser since all the ref counted samples in the
            //samples set need to be released out of this scope even if this module is not using them, otherwise there will
            //be a memory leak.
            auto scoped_sample_set = get_unique_ptr_with_releaser(sample_set);
            auto depth_image = scoped_sample_set->take_shared(stream_type::depth);

            if(!depth_image)
            {
                return status_item_unavailable;
            }

            m_input_depth_image.set(depth_image);

            return status_no_error;
        }

        status max_depth_value_module_impl::register_event_hander(video_module_interface::processing_event_handler *handler)
        {
            if(m_processing_handler != nullptr)
            {
                return status_handle_invalid;
            }
            m_processing_handler = handler;
            return status_no_error;
        }

        status max_depth_value_module_impl::unregister_event_hander(video_module_interface::processing_event_handler *handler)
        {
            if(m_processing_handler != handler)
            {
                return status_handle_invalid;
            }

            m_processing_handler = nullptr;
            return status_no_error;
        }

        video_module_control_interface * max_depth_value_module_impl::query_video_module_control()
        {
            return nullptr;
        }

        max_depth_value_module_interface::max_depth_value_output_data max_depth_value_module_impl::get_max_depth_value_data()
        {
            return m_output_data.blocking_get();
        }

        status max_depth_value_module_impl::process_depth_max_value(
                std::shared_ptr<core::image_interface> depth_image,
                max_depth_value_module_interface::max_depth_value_output_data & output_data)
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
                const uint16_t * current_pixel = static_cast<const uint16_t * >(depth_image->query_data());
                auto depth_image_info = depth_image->query_info();
                for(auto i = 0; i < depth_image_info.height * depth_image_info.pitch; i++)
                {
                    if(current_pixel[i] > max_depth_value)
                    {
                        max_depth_value = current_pixel[i];
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

                max_depth_value_module_interface::max_depth_value_output_data output_data;
                auto status = process_depth_max_value(std::move(current_depth_image), output_data);
                if(status < status_no_error)
                {
                    LOG_INFO("failed to process max value, error code :" << status);
                    continue;
                }
                m_output_data.set(output_data);
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

            max_depth_value_module_interface::max_depth_value_output_data empty_output_data = {};
            m_output_data.set(empty_output_data);
        }
    }
}

