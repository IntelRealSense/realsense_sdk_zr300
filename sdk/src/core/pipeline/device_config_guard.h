// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <map>
#include <functional>
#include <librealsense/rs.hpp>
#include "rs/core/video_module_interface.h"
#include <rs/core/correlated_sample_set.h>

namespace rs
{
    namespace core
    {
        class device_config_guard
        {
        public:
            device_config_guard(rs::device * device,
                                const video_module_interface::actual_module_config& given_config,
                                std::function<void(std::shared_ptr<correlated_sample_set> sample_set)> non_blocking_notify_sample);

            device_config_guard(const device_config_guard&) = delete;
            device_config_guard & operator=(const device_config_guard&) = delete;

            virtual ~device_config_guard();
        private:
            rs::device * m_device;
            const video_module_interface::actual_module_config m_config;
            std::function<void(std::shared_ptr<correlated_sample_set> sample_set)> m_non_blocking_notify_sample;

            std::map<stream_type, std::function<void(rs::frame)>> m_stream_callback_per_stream;
            std::function<void(rs::motion_data)> m_motion_callback;
        };
    }
}

