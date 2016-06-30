// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <memory>
#include <vector>
#include <librealsense/rs.hpp>
#include "rs/core/context.h"

namespace rs
{
    namespace record
    {
        class device;
        /**
        This class provides access to the connected RealSense devices with record capabilities extentions.
        */
        class context : public rs::core::context
        {
        public:
            context(const std::string& file_path);
            virtual ~context();

            /**
            @brief Returns a record device by the given index, down casted to rs::device.
            @param[in] index  The zero based index of the device to retrieve
            @return rs::device*     The requested device.
            */
            rs::device * get_device(int index) override;

            /**
            @brief Returns a record device by the given index. Makes all record capabilities available.
            @param[in] index  The zero based index of the device to retrieve
            @return record::device*     The requested device.
            */
            device * get_record_device(int index);

        private:
            std::vector<std::unique_ptr<rs_device>> m_devices;
        };
    }
}
