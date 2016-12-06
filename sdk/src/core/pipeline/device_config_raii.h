// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense/rs.hpp>
#include "rs/core/video_module_interface.h"

namespace rs
{
    namespace core
    {
        class device_config_raii
        {
        public:
            device_config_raii(const video_module_interface::supported_module_config& given_config,
                               rs::device * device);

            virtual ~device_config_raii();
        private:
            rs::device * m_device;
        };
    }
}

