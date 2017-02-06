// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <vector>
#include "rs/core/video_module_interface.h"

namespace rs
{
    namespace core
    {
        class config_util
        {
        public:
            config_util(const config_util&) = delete;
            config_util& operator=(const config_util&) = delete;

            static void generete_matching_supersets(const std::vector<std::vector<video_module_interface::supported_module_config>>& groups,
                                                    std::vector<video_module_interface::supported_module_config> &matching_supersets);

            static bool is_config_empty(const video_module_interface::supported_module_config & config);

        private:
            config_util(){}

            static void recursive_cartesian_multiplicity(const std::vector<std::vector<video_module_interface::supported_module_config> > &groups,
                                                         const uint32_t group_index,
                                                         const std::vector<video_module_interface::supported_module_config> &combination_prefix,
                                                         std::vector<std::vector<video_module_interface::supported_module_config>>& combinations);

            static bool can_flatten_to_superset(const std::vector<video_module_interface::supported_module_config>& combination,
                                            video_module_interface::supported_module_config &matching_supersets);

        };
    }
}

