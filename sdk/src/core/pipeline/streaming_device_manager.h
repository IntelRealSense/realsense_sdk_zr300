// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <map>
#include <functional>
#include <librealsense/rs.hpp>

#include <rs/core/context_interface.h>
#include <rs/core/types.h>
#include <rs/core/correlated_sample_set.h>
#include <rs/core/video_module_interface.h>

namespace rs
{
    namespace core
    {
        class streaming_device_manager
        {
        public:
            streaming_device_manager(video_module_interface::supported_module_config & module_config,
                                     std::function<void(std::shared_ptr<correlated_sample_set> sample_set)> non_blocking_notify_sample,
                                     rs::device * device);
            streaming_device_manager(const streaming_device_manager&) = delete;
            streaming_device_manager & operator=(const streaming_device_manager&) = delete;

            virtual ~streaming_device_manager();
        private:            
            std::function<void(std::shared_ptr<correlated_sample_set> sample_set)> m_non_blocking_notify_sample;

            rs::device * m_device;
            rs::source m_active_sources;
            std::map<stream_type, std::function<void(rs::frame)>> m_stream_callback_per_stream;
            std::function<void(rs::motion_data)> m_motion_callback;
        };
    }
}

