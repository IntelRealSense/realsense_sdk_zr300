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
        *
        * This interface provides the user the ability to use the same application with minimal changes, to access live camera,
        * access a camera while recording or playback a recorded file. The changes are encapsulated in the context and device construction,
        * when creating the relevant implementation class.
        * The context object owns the devices lifetime. It is responsible for creating and releasing its devices, and must outlive the lifespan of the devices.
        */
        class context_interface
        {
        public:
            virtual ~context_interface() {}
            /**
            * @brief Get the device count owned by this context
            *
            * The number of devices owned by the context depends on the specific context type.
            * The output of this function should be used for enumerating the devices using get_device.
            * @return int     The number of devices.
            */
            virtual int get_device_count() const = 0;

            /**
            * @brief Retrieve device by index.
            *
            * @param[in] index  The zero based index of device to retrieve
            * @return rs::device*       The requested device
            */
            virtual rs::device * get_device(int index) = 0;
        };
    }
}
