// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense/rs.hpp>
#include "rs/core/context.h"

namespace rs
{
    namespace playback
    {
        class device;
        /**
        * @class rs::playback::context
        * @brief This class provide access to recorded data through rs::playback::device.
        *
        * This class implements the context interface,
        */
        class context : public rs::core::context_interface
        {
        public:
            context(const char * file_path);
            ~context();

            /**
            * @brief Get number of available playback devices.
            *
            * Currently supporting a single device.
            * @return int      Number of available devices.
            */
            int get_device_count() const override;

            /**
            * @brief Get the single playback device down casted to rs::device.
            *
            * @param[in] index  The zero based index of device to retrieve.
            * @return rs::device*     The requested device.
            */
            rs::device * get_device(int index) override;

             /**
             * @brief Get the single playback device.
             *
             * Makes all playback capabilities available.
             * @return playback::device*     The requested device.
             */
             device * get_playback_device();

        private:
            context(const context& cxt) = delete;
            context& operator=(const context& cxt) = delete;

            rs_device **    m_devices;
            bool            m_init_status;
        };
    }
}
