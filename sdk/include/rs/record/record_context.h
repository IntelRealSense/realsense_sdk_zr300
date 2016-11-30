// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense/rs.hpp>
#include "rs/core/context.h"

namespace rs
{
    namespace record
    {
        class device;
        /**
        * @class rs::record::context
        * @brief rs::record::context extends rs::core::context for capturing data to file during live camera streaming. See the interface class for more details.
        */
        class context : public rs::core::context
        {
        public:
            context(const char * file_path);
            virtual ~context();

            /**
            * @brief Retrieve a device by index.
            *
            * The available devices and their index doesn't change throughout the context lifespan. If a device is connected or disconnected
            * in the context lifespan, the devices list won't change, and may expose disconnected devices, or not reflect connected devices.
            * @param[in] index  The zero based index of device to retrieve
            * @return rs::device*       The requested device
            */
            rs::device * get_device(int index) override;

            /**
            * @brief Returns a record device by the given index. Makes all record capabilities available.
            *
            * The function returns rs::record::device, to provide access to all record capabilities, which extend the basic device functionality.
            * @param[in] index  The zero based index of the device to retrieve
            * @return record::device*     The requested device.
            */
            device * get_record_device(int index);

        private:
            context(const context& cxt) = delete;
            context& operator=(const context& cxt) = delete;

            rs_device ** m_devices;
        };
    }
}
