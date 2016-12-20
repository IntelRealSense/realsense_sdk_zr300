// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/** 
* \file record_context.h
* @brief Describes the \c rs::record::context class.
*/

#pragma once
#include <librealsense/rs.hpp>
#include "rs/core/context.h"

#ifdef WIN32 
#ifdef realsense_record_EXPORTS
#define  DLL_EXPORT __declspec(dllexport)
#else
#define  DLL_EXPORT __declspec(dllimport)
#endif /* realsense_record_EXPORTS */
#else /* defined (WIN32) */
#define DLL_EXPORT
#endif

namespace rs
{
    namespace record
    {
        class device;
        /**
        * @brief Extends \c rs::core::context for capturing data to file during live camera streaming. 
		*
		* See the interface class for more details.
        */
        class DLL_EXPORT context : public rs::core::context
        {
        public:
            context(const char * file_path);
            virtual ~context();

            /**
            * @brief Retrieves a device by index.
            *
            * The available devices and their indices do not change throughout the context lifespan. If a device is connected or disconnected
            * in the context lifespan, the devices list will not change, and may expose disconnected devices, or not reflect connected devices.
            * @param[in] index Zero-based index of device to retrieve
            * @return rs::device* Requested device
            */
            rs::device * get_device(int index) override;

            /**
            * @brief Returns a record device by the given index. Makes all record capabilities available.
            *
            * The method returns \c rs::record::device, to provide access to all record capabilities, which extend the basic device functionality.
            * @param[in] index Zero-based index of device to retrieve
            * @return record::device* Requested device
            */
            device * get_record_device(int index);

        private:
            context(const context& cxt) = delete;
            context& operator=(const context& cxt) = delete;

            rs_device ** m_devices;
        };
    }
}
