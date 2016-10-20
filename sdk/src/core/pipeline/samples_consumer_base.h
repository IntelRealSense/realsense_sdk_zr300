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
            samples_consumer_base(const video_module_interface::actual_module_config &module_config,
                                  const video_module_interface::supported_module_config::time_sync_mode time_sync_mode);
            void notify_sample_set_non_blocking(std::shared_ptr<correlated_sample_set> sample_set);
            virtual ~samples_consumer_base();
        protected:
            virtual void on_complete_sample_set(std::shared_ptr<correlated_sample_set> ready_sample_set) = 0;
        private:            
            const video_module_interface::actual_module_config m_module_config;
            rs::utils::unique_ptr<rs::utils::samples_time_sync_interface> m_time_sync_util;

            bool is_sample_set_relevant(const std::shared_ptr<correlated_sample_set> & sample_set) const;
            std::shared_ptr<correlated_sample_set> insert_to_time_sync_util(const std::shared_ptr<correlated_sample_set> & input_sample_set);
            std::vector<std::shared_ptr<correlated_sample_set>> get_unmatched_frames();
            rs::utils::unique_ptr<rs::utils::samples_time_sync_interface> get_time_sync_util_from_module_config(const video_module_interface::actual_module_config &module_config,
                                                                                                                const video_module_interface::supported_module_config::time_sync_mode time_sync_mode);
        };
    }
}
