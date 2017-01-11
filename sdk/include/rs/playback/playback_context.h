// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/** 
* \file playback_context.h
* @brief Describes the \c rs::core::context class.
*/

#pragma once
#include <librealsense/rs.hpp>
#include "rs/core/context.h"

namespace rs
{
    namespace playback
    {
        class device;
        /**
        * @brief Implements \c rs::core::context_interface for playback from recorded files. 
		*
		* See the interface class for more details.
        */
        class context : public rs::core::context_interface
        {
        public:
            context(const char * file_path);
            ~context();

            /**
            * @brief Gets number of available playback devices.
            *
            * The playback context provides access to the single device that was recorded in the session. Therefore, this method always returns 1.
            * @return int Number of available devices
            */
            int get_device_count() const override;

            /**
            * @brief Gets single playback device.
            *
            * The method returns \c rs::playback::device, down-casted to \c rs::device.
            * @param[in] index Zero-based index of device to retrieve
            * @return rs::device* Requested device
            */
            rs::device * get_device(int index) override;

             /**
             * @brief Gets single playback device.
             *
             * The function returns \c rs::playback::device, to provide access to all playback capabilities, which extend the basic device functionality.
             * @return playback::device* Requested device.
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
