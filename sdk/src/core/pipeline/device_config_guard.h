// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense/rs.hpp>
#include "rs/core/video_module_interface.h"

namespace rs
{
    namespace core
    {
        class device_config_guard
        {
        public:
            device_config_guard(const video_module_interface::supported_module_config& given_config,
                               rs::device * device);

            device_config_guard(const device_config_guard&) = delete;
            device_config_guard & operator=(const device_config_guard&) = delete;

            virtual ~device_config_guard();
        private:
            rs::device * m_device;
        };
    }
}

