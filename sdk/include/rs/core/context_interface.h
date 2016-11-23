// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense/rs.hpp>

namespace rs
{
    namespace core
    {
        /**
        * @class rs::core::context_interface
        * @brief rs::core::context_interface provides the ability to use the same application with minimal changes, to access live camera,
        *
        * access a camera while recording or playback a recorded file. The changes are encapsulated in the context and device construction,
        * when creating the relevant implementation class.
        * The context object owns the devices lifetime. It is responsible for creating and releasing its devices, and must outlive the lifespan of the devices.
        * The context provides access only to devices, which are available upon context creation. If a device is connected or disconnected
        * after the context is created, the devices list won't change, and won't reflect the actual connected devices list.
        */
        class context_interface
        {
        public:
            virtual ~context_interface() {}
            /**
            * @brief Get the device count owned by this context
            *
            * The number of devices owned by the context depends on the specific context type.
            * The number of devices doesn't change throughout the context lifespan.
            * The output of this function should be used for enumerating the devices using get_device.
            * @return int     The number of devices.
            */
            virtual int get_device_count() const = 0;

            /**
            * @brief Retrieve a device by index.
            *
            * The available devices and their index doesn't change throughout the context lifespan. If a device is connected or disconnected
            * in the context lifespan, the devices list won't change, and may expose disconnected devices, or not reflect connected devices.
            * @param[in] index  The zero based index of device to retrieve
            * @return rs::device*       The requested device
            */
            virtual rs::device * get_device(int index) = 0;
        };
    }
}
