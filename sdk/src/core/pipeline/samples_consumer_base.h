// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <memory>
#include <vector>
#include "rs/utils/samples_time_sync_interface.h"

namespace rs
{
    namespace core
    {
        class samples_consumer_base
        {
        public:
            samples_consumer_base(const video_module_interface::actual_module_config & module_config, video_module_interface::supported_module_config::time_sync_mode time_sync_mode);

            void non_blocking_set_sample_set(std::shared_ptr<correlated_sample_set> sample_set);
            bool is_sample_set_contains_a_single_required_sample(const std::shared_ptr<correlated_sample_set> & sample_set);
            virtual ~samples_consumer_base();
        protected:
            std::shared_ptr<correlated_sample_set> insert_to_time_sync_util(const std::shared_ptr<correlated_sample_set> & input_sample_set);
            std::vector<std::shared_ptr<correlated_sample_set>> get_unmatched_frames();
            virtual void set_ready_sample_set(std::shared_ptr<correlated_sample_set> ready_sample_set) = 0;
        private:            
            const video_module_interface::actual_module_config m_module_config;
            rs::utils::unique_ptr<rs::utils::samples_time_sync_interface> m_time_sync_util;
            rs::utils::unique_ptr<rs::utils::samples_time_sync_interface> get_time_sync_util_from_module_config(
                    const video_module_interface::actual_module_config & module_config,
                    video_module_interface::supported_module_config::time_sync_mode time_sync_mode);
        };
    }
}
