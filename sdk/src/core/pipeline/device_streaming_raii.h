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
        class device_streaming_raii
        {
        public:
            device_streaming_raii(video_module_interface::actual_module_config & module_config,
                                  std::function<void(std::shared_ptr<correlated_sample_set> sample_set)> non_blocking_notify_sample,
                                  rs::device * device);
            device_streaming_raii(const device_streaming_raii&) = delete;
            device_streaming_raii & operator=(const device_streaming_raii&) = delete;

            virtual ~device_streaming_raii();
        private:            
            std::function<void(std::shared_ptr<correlated_sample_set> sample_set)> m_non_blocking_notify_sample;
            rs::device * m_device;
            rs::source m_active_sources;
            std::map<stream_type, std::function<void(rs::frame)>> m_stream_callback_per_stream;
            std::function<void(rs::motion_data)> m_motion_callback;
        };
    }
}

