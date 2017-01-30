// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/**
* \file context.h
* @brief Describes the \c rs::core::context class.
*/

#pragma once
#include <librealsense/rs.hpp>
#include "context_interface.h"

namespace rs
{
    namespace core
    {
        /**
        * @brief Implements \c rs::core::context_interface for live camera streaming. 
        *
		* See the interface class for more details.
        */
        class context : public context_interface
        {
        public:
            context() {}
            virtual ~context() {}

            /**
            * @brief Gets the device count owned by this context.
            *
            * The number of devices do not change throughout the context lifespan.
            * The output of this method should be used for enumerating the devices using \c get_device().
            * @return int Number of devices
            */
            virtual int get_device_count() const override
            {
                return m_context.get_device_count();
            }

            /**
            * @brief Retrieves a device by index.
            *
            * The available devices and their indices do not change throughout the context lifespan. If a device is connected or disconnected
            * in the context lifespan, the device list does not change, and may expose disconnected devices, or not reflect connected devices.
            * @param[in] index Zero-based index of device to retrieve
            * @return rs::device* Requested device
            */
            virtual rs::device * get_device(int index) override
            {
                return m_context.get_device(index);
            }

        protected:
            rs::context m_context; /**< the actual libRealSense context. */
            context(const context &) = delete;
            context & operator = (const context &) = delete;
        };
    }
}
