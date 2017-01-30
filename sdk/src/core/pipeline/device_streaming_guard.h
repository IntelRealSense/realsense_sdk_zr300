// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense/rs.hpp>

namespace rs
{
    namespace core
    {
        class device_streaming_guard
        {
        public:
            device_streaming_guard(rs::device * device,
                                   rs::source enabled_sources);

            device_streaming_guard(const device_streaming_guard&) = delete;
            device_streaming_guard & operator=(const device_streaming_guard&) = delete;

            virtual ~device_streaming_guard();
        private:            
            rs::device * m_device;
            rs::source m_enabled_sources;
        };
    }
}

