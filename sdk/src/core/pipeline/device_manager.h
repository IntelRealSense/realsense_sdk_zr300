// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <memory>
#include <functional>
#include <librealsense/rs.hpp>
#include "rs/core/correlated_sample_set.h"
#include "device_config_raii.h"
#include "device_streaming_raii.h"

namespace rs
{
    namespace core
    {
        class device_manager
        {
        public:
            device_manager(const video_module_interface::supported_module_config & config,
                           std::function<void(std::shared_ptr<correlated_sample_set> sample_set)> non_blocking_notify_sample,
                           rs::device * device);

            device_manager(const device_manager & other) = delete;
            device_manager & operator=(const device_manager & other) = delete;

            void start();
            void stop();
            void query_current_config(video_module_interface::actual_module_config & current_config) const;
            rs::device * get_underlying_device();
            const video_module_interface::actual_module_config create_actual_config_from_supported_config(
                    const video_module_interface::supported_module_config & supported_config) const;
            projection_interface * get_color_depth_projection();

            virtual ~device_manager();
        private:            
            std::function<void(std::shared_ptr<correlated_sample_set> sample_set)> m_non_blocking_notify_sample;
            rs::device * m_current_device;
            video_module_interface::actual_module_config m_actual_config;
            rs::utils::unique_ptr<projection_interface> m_projection;
            std::unique_ptr<device_config_raii> m_device_config_raii;
            std::unique_ptr<device_streaming_raii> m_device_streaming_raii;

            bool is_there_a_satisfying_device_mode(const video_module_interface::supported_module_config& given_config) const;
        };
    }
}

