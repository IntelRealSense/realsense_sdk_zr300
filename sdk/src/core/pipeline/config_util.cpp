// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.
#include <cstring>
#include <algorithm>
#include "config_util.h"

using namespace std;

namespace rs
{
    namespace core
    {
        void config_util::generete_matching_supersets(const std::vector<std::vector<video_module_interface::supported_module_config> > &groups,
                                                      std::vector<video_module_interface::supported_module_config>& matching_supersets)
        {
            std::vector<std::vector<video_module_interface::supported_module_config>> combinations;
            recursive_cartesian_multiplicity(groups, 0, {}, combinations);

            //flatten each combination vector to a single superset
            for(auto & combination : combinations)
            {
                video_module_interface::supported_module_config superset = {};
                if(can_flatten_to_superset(combination, superset))
                {
                    matching_supersets.push_back(superset);
                }
            }
        }

        bool config_util::is_config_empty(const video_module_interface::supported_module_config & config)
        {


            for(uint32_t stream_index = 0; stream_index < static_cast<uint32_t>(stream_type::max); ++stream_index)
            {
                if(config.image_streams_configs[stream_index].is_enabled)
                {
                    return false;
                }
            }

            for(uint32_t motion_index = 0; motion_index < static_cast<uint32_t>(motion_type::max); ++motion_index)
            {
                if(config.motion_sensors_configs[motion_index].is_enabled)
                {
                    return false;
                }
            }
            return true;
        }

        void config_util::recursive_cartesian_multiplicity(const std::vector<std::vector<video_module_interface::supported_module_config>>& groups,
                                                           const uint32_t group_index,
                                                           const std::vector<video_module_interface::supported_module_config>& combination_prefix,
                                                           std::vector<std::vector<video_module_interface::supported_module_config>>& combinations)
        {
            if(groups.size() <= group_index)
            {
                combinations.push_back(combination_prefix);
                return;
            }

            if(groups[group_index].size() == 0)
            {
                recursive_cartesian_multiplicity(groups, group_index+1, combination_prefix, combinations);
            }
            else
            {
                for(uint32_t config_index = 0; config_index < groups[group_index].size(); config_index++)
                {
                    vector<video_module_interface::supported_module_config> combination_copy(combination_prefix);
                    combination_copy.push_back(groups[group_index][config_index]);
                    recursive_cartesian_multiplicity(groups, group_index+1, combination_copy, combinations);
                }
            }

        }

        bool config_util::can_flatten_to_superset(const std::vector<video_module_interface::supported_module_config>& combination,
                                                  video_module_interface::supported_module_config &superset)
        {
            superset = {};

            for(const video_module_interface::supported_module_config& config : combination)
            {
                //update device name
                auto check_device_name_conflicting = (std::strcmp(config.device_name, "") != 0);
                if(check_device_name_conflicting)
                {
                    bool does_superset_needs_to_update = (std::strcmp(superset.device_name, "") == 0);
                    if(does_superset_needs_to_update)
                    {
                        std::memcpy(superset.device_name, config.device_name, std::strlen(config.device_name));
                    }
                    else
                    {
                        auto is_conflicting = std::strcmp(config.device_name, superset.device_name) != 0;
                        if(is_conflicting)
                        {
                            return false;
                        }
                    }
                }

                //update streams
                for(uint32_t stream_index = 0; stream_index < static_cast<uint32_t>(stream_type::max); ++stream_index)
                {
                    auto & superset_stream = superset.image_streams_configs[stream_index];
                    auto & config_stream = config.image_streams_configs[stream_index];

                    if(!config_stream.is_enabled)
                    {
                        continue; // check next stream
                    }

                    if(!superset_stream.is_enabled)
                    {
                        std::memcpy(&superset_stream, &config_stream, sizeof(config_stream));
                        continue; // check next stream
                    }

                    // both enabled, check for conflicts
                    if(superset_stream.size.width == 0)
                    {
                        superset_stream.size.width = config_stream.size.width;
                    }
                    else
                    {
                        if(config_stream.size.width != 0 && superset_stream.size.width != config_stream.size.width)
                        {
                            return false;
                        }
                    }

                    if(superset_stream.size.height == 0)
                    {
                        superset_stream.size.height = config_stream.size.height;
                    }
                    else
                    {
                        if(config_stream.size.height != 0 && superset_stream.size.height!= config_stream.size.height)
                        {
                            return false;
                        }
                    }

                    if(superset_stream.frame_rate == 0)
                    {
                        superset_stream.frame_rate = config_stream.frame_rate;
                    }
                    else
                    {
                        if(config_stream.frame_rate != 0 && superset_stream.frame_rate != config_stream.frame_rate)
                        {
                            return false;
                        }
                    }

                    if(superset_stream.flags == sample_flags::none)
                    {
                        superset_stream.flags = config_stream.flags;
                    }
                    else
                    {
                        if(config_stream.flags != sample_flags::none && superset_stream.flags != config_stream.flags)
                        {
                            return false;
                        }
                    }
                }

                //update motions
                for(uint32_t motion_index = 0; motion_index < static_cast<uint32_t>(motion_type::max); ++motion_index)
                {
                    auto & superset_motion = superset.motion_sensors_configs[motion_index];
                    auto & config_motion = config.motion_sensors_configs[motion_index];

                    if(!config_motion.is_enabled)
                    {
                        continue; // check next motion
                    }

                    if(!superset_motion.is_enabled)
                    {
                        std::memcpy(&superset_motion, &config_motion, sizeof(config_motion));
                        continue; // check next motion
                    }

                    if(superset_motion.sample_rate == 0)
                    {
                        superset_motion.sample_rate = config_motion.sample_rate;
                    }
                    else
                    {
                        if(config_motion.sample_rate != 0 && superset_motion.sample_rate != config_motion.sample_rate)
                        {
                            return false;
                        }
                    }

                    if(superset_motion.flags == sample_flags::none)
                    {
                        superset_motion.flags = config_motion.flags;
                    }
                    else
                    {
                        if(config_motion.flags != sample_flags::none && superset_motion.flags != config_motion.flags)
                        {
                            return false;
                        }
                    }
                }

                superset.concurrent_samples_count = std::max(superset.concurrent_samples_count, config.concurrent_samples_count);

            }
            return true;
        }
    }
}

