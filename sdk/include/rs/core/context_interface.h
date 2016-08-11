// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense/rs.hpp>

namespace rs
{
    namespace core
    {
        class context_interface
        {
        public:
            virtual ~context_interface() {}
            /**
            @brief determine number of connected devices
            @return  the count of devices
            */
            virtual int get_device_count() const = 0;

            /**
            @brief retrieve connected device by index
            @param[in] index  the zero based index of device to retrieve
            @return           the requested device
            */
            virtual rs::device * get_device(int index) = 0;
        };
    }
}
