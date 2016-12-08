// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/**
* \file context_interface.h
* @brief Describes the \c rs::core::context_interface class.
*/

#pragma once
#include <librealsense/rs.hpp>

namespace rs
{
    namespace core
    {
        /**
        * @brief Provides the ability to use the same application with minimal changes and to access a camera while recording or playing back a recorded file. 
		*
		* The changes are encapsulated in the context and device construction 
        * when creating the relevant implementation class.
        * The context object owns the devices lifetime. It is responsible for creating and releasing its devices, and must outlive the lifespan of the devices.
        * The context provides access only to devices, which are available upon context creation. If a device is connected or disconnected
        * after the context is created, the device list will not change, and will not reflect the actual connected devices list.
        */
        class context_interface
        {
        public:
            virtual ~context_interface() {}
            /**
            * @brief Gets the device count owned by this context.
            *
            * The number of devices owned by the context depends on the specific context type.
            * The number of devices does not change throughout the context lifespan.
            * The output of this method should be used for enumerating the devices using \c get_device().
            * @return int Number of devices
            */
            virtual int get_device_count() const = 0;

            /**
            * @brief Retrieves a device by index.
            *
            * The available devices and their index do not change throughout the context lifespan. If a device is connected or disconnected
            * in the context lifespan, the devices list will not change, and may expose disconnected devices, or not reflect connected devices.
            * @param[in] index  Zero-based index of device to retrieve
            * @return rs::device * Requested device
            */
            virtual rs::device * get_device(int index) = 0;
        };
    }
}
