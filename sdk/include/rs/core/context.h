// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense/rs.hpp>
#include "context_interface.h"

namespace rs
{
    namespace core
    {
        /**
        * @class rs::core::context
        * @brief rs::core::context implements rs::core::context_interface for live camera streaming. See the interface class for more details.
        */
        class context : public context_interface
        {
        public:
            context() {}
            virtual ~context() {}

            /**
            * @brief Get the device count owned by this context
            *
            * The number of devices owned by the context.
            * The number of devices doesn't change throughout the context lifespan.
            * The output of this function should be used for enumerating the devices using get_device.
            * @return int     The number of devices.
            */
            virtual int get_device_count() const override
            {
                return m_context.get_device_count();
            }

            /**
            * @brief Retrieve a device by index.
            *
            * The available devices and their index doesn't change throughout the context lifespan. If a device is connected or disconnected
            * in the context lifespan, the devices list won't change, and may expose disconnected devices, or not reflect connected devices.
            * @param[in] index  The zero based index of device to retrieve
            * @return rs::device*       The requested device
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
