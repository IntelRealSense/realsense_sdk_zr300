// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "samples_consumer_base.h"
using namespace rs::utils;

namespace rs
{
    namespace core
    {
        samples_consumer_base::samples_consumer_base(const video_module_interface::supported_module_config & module_config) :
            m_module_config(module_config)
        {
            m_time_sync_util = get_time_sync_util_from_module_config(m_module_config);
        }

        bool samples_consumer_base::is_sample_set_contains_a_single_required_sample(const std::shared_ptr<correlated_sample_set> & sample_set)
        {
            bool is_a_single_sample_found = false;
            for(auto stream_index = 0; stream_index < static_cast<int32_t>(stream_type::max); stream_index++)
            {
                if(sample_set->images[stream_index])
                {
                    if(m_module_config.image_streams_configs[stream_index].is_enabled)
                    {
                        if(!is_a_single_sample_found)
                        {
                            is_a_single_sample_found = true;
                        }
                        else //already found one
                        {
                            return false;
                        }
                    }
                    else //not requested
                    {
                        return false;
                    }
                }
            }
            for(auto motion_index = 0; motion_index < static_cast<int32_t>(motion_type::max); motion_index++)
            {
                if(sample_set->motion_samples[motion_index].timestamp != 0) //motion samples set
                {
                    if(m_module_config.motion_sensors_configs[motion_index].is_enabled)
                    {
                        if(!is_a_single_sample_found)
                        {
                            is_a_single_sample_found = true;
                        }
                        else //already found one
                        {
                            return false;
                        }
                    }
                    else //not requested
                    {
                        return false;
                    }
                }
            }
            return is_a_single_sample_found;
        }


        void samples_consumer_base::non_blocking_set_sample_set(std::shared_ptr<correlated_sample_set> sample_set)
        {
            if(!sample_set)
            {
                return;
            }

            std::shared_ptr<correlated_sample_set> ready_sample_set = insert_to_time_sync_util(sample_set);

            auto unmatched_frames = get_unmatched_frames(); // empty on no time sync or time sync input only modes
            for(auto unmatched_frame : unmatched_frames)
            {
                set_ready_sample_set(unmatched_frame);
            }

            if(ready_sample_set)
            {
                set_ready_sample_set(ready_sample_set);
            }
        }

        std::shared_ptr<correlated_sample_set> samples_consumer_base::insert_to_time_sync_util(const std::shared_ptr<correlated_sample_set> & input_sample_set)
        {
            if(!m_time_sync_util) //no time sync utils means pass through samples without time sync
            {
                return input_sample_set;
            }

            for(auto stream_index = 0; stream_index < static_cast<int32_t>(stream_type::max); stream_index++)
            {
                if(input_sample_set->images[stream_index])
                {
                    correlated_sample_set ready_sample_set = {};
                    input_sample_set->images[stream_index]->add_ref();
                    if(m_time_sync_util->insert(input_sample_set->images[stream_index], ready_sample_set))
                    {
                        std::shared_ptr<correlated_sample_set> output_sample_set(new correlated_sample_set(), sample_set_releaser());
                        *output_sample_set = ready_sample_set;
                        return output_sample_set;
                    }
                }
            }
            for(auto motion_index = 0; motion_index < static_cast<int32_t>(motion_type::max); motion_index++)
            {
                if(input_sample_set->motion_samples[motion_index].timestamp != 0) //motion samples set
                {
                    correlated_sample_set ready_sample_set = {};
                    if(m_time_sync_util->insert(input_sample_set->motion_samples[motion_index], ready_sample_set))
                    {
                        std::shared_ptr<correlated_sample_set> output_sample_set(new correlated_sample_set(), sample_set_releaser());
                        *output_sample_set = ready_sample_set;
                        return output_sample_set;
                    }
                }
            }
            LOG_WARN("havent processed sample set by sync utility");
            return nullptr;
        }

        rs::utils::unique_ptr<rs::utils::samples_time_sync_interface> samples_consumer_base::get_time_sync_util_from_module_config(
                const video_module_interface::supported_module_config & module_config)
        {
            //default time sync configuration values
            unsigned int max_input_latency = 100;
            unsigned int not_matched_frames_buffer_size = 0;

            rs::utils::unique_ptr<samples_time_sync_interface> time_sync_util;
            switch (module_config.samples_time_sync_mode) {
                case video_module_interface::supported_module_config::time_sync_mode::sync_not_required:
                    break;
                case video_module_interface::supported_module_config::time_sync_mode::time_synced_input_accepting_unmatch_samples:
                    // update the default time sync configuration values to accept unmatched samples
                    max_input_latency = 100;
                    not_matched_frames_buffer_size = 1;
                case video_module_interface::supported_module_config::time_sync_mode::time_synced_input_only:
                {
                    int streams_fps[static_cast<int32_t>(stream_type::max)] = {};
                    for(auto stream_index = 0; stream_index < static_cast<int32_t>(stream_type::max); stream_index++)
                    {
                        if(module_config.image_streams_configs[stream_index].is_enabled)
                        {
                            streams_fps[stream_index] = module_config.image_streams_configs[stream_index].ideal_frame_rate;
                        }
                    }
                    int motions_fps[static_cast<int32_t>(motion_type::max)] = {};
                    for(auto motion_index = 0; motion_index < static_cast<int32_t>(motion_type::max); motion_index++)
                    {
                        if(module_config.motion_sensors_configs[motion_index].is_enabled)
                        {
                            motions_fps[motion_index] = module_config.motion_sensors_configs[motion_index].ideal_frame_rate;
                        }
                    }

                    try
                    {
                        time_sync_util = get_unique_ptr_with_releaser(samples_time_sync_interface::create_instance(streams_fps,
                                                                                                                   motions_fps,
                                                                                                                   module_config.device_name,
                                                                                                                   max_input_latency,
                                                                                                                   not_matched_frames_buffer_size));
                    }
                    catch(const std::exception & ex)
                    {
                        LOG_ERROR("failed to create time syncing utility, ex : " << ex.what());
                    }
                    break;
                }
            }
            return time_sync_util;
        }

        std::vector<std::shared_ptr<correlated_sample_set>> samples_consumer_base::get_unmatched_frames()
        {
            std::vector<std::shared_ptr<correlated_sample_set>> unmatched_samples;
            if(!m_time_sync_util) //no time sync utils means no unmatched samples
            {
                return unmatched_samples;
            }

            //get all unmatched frames from each stream - TODO : sort them by timestamp?
            bool is_there_more_unmatched_samples_for_at_least_one_stream = true;
            while(is_there_more_unmatched_samples_for_at_least_one_stream)
            {
                is_there_more_unmatched_samples_for_at_least_one_stream = false;
                for(auto stream_index = 0; stream_index < static_cast<int32_t>(stream_type::max); stream_index++)
                {
                    stream_type stream = static_cast<stream_type>(stream_index);
                    image_interface* image = nullptr;
                    is_there_more_unmatched_samples_for_at_least_one_stream  |= m_time_sync_util->get_not_matched_frame(stream, &image);
                    if (image)
                    {
                        std::shared_ptr<correlated_sample_set> sample_set(new correlated_sample_set(), sample_set_releaser());
                        (*sample_set)[stream] = image;
                        unmatched_samples.push_back(std::move(sample_set));
                    }
                }
            }

            return unmatched_samples;
        }

        samples_consumer_base::~samples_consumer_base()
        {

        }
    }
}
