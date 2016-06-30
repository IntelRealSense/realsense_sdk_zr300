// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "metadata_interface.h"
#include "rs/utils/smart_ptr.h"
#include "types.h"

namespace rs
{
    namespace core
    {
        /**
        image interface abstracts interactions with images
        */
        class image_interface
        {
        public:
            /**
            @enum flag
            Describes the image flags.
            */
            enum flag
            {
                any = 0x0
            };

            /**
            @brief Return the image sample information.
            @return the image sample information in the image_info structure.
            */
            virtual image_info query_info(void) const = 0;

            /**
            @brief Get the image time stamp.
            @return the time stamp value, in milliseconds since the device was started.
            */
            virtual uint64_t query_time_stamp(void) const = 0;

            /**
            @brief Get the image flags.
            @return the flags.
            */
            virtual flag query_flags(void) const = 0;

            /**
            @brief Get the image data.
            @return the data.
            */
            virtual const void * query_data(void) const = 0;

            /**
            @brief Return the image stream type. The application should cast the returned type to PXCCapture::StreamType.
            @return the stream type.
            */
            virtual stream_type query_stream_type(void) const = 0;

            /**
            @brief Return metadata of the image.
            @return smart_ptr<metadata_interface>      image metadata
            */
            virtual rs::utils::smart_ptr<metadata_interface> query_metadata() = 0;

            /**
            @brief Convert the current image image to a given format.
            @param[in]  format                    Destanation format.
            @param[out] converted_image           Converted image allocated internaly.
            @return PXC_STATUS_NO_ERROR           Successful execution.
            @return STATUS_PARAM_UNSUPPORTED      Convertion to this format is currently unsupported.
            @return STATUS_FEATURE_UNSUPPORTED    The feature is currently unsupported.
            @return STATUS_EXEC_ABORTED           Failed to convert.
            */
            virtual status convert_to(pixel_format format, rs::utils::smart_ptr<const image_interface> & converted_image) = 0;

            /**
            @brief Convert the current image image to a given format.
            @param[in]  rotation                  Destanation rotation.
            @param[out] converted_image           Converted image allocated internaly.
            @return PXC_STATUS_NO_ERROR           Successful execution.
            @return STATUS_PARAM_UNSUPPORTED      The given rotation is currently unsupported.
            @return STATUS_FEATURE_UNSUPPORTED    The feature is currently unsupported.
            @return STATUS_EXEC_ABORTED           Failed to convert.
            */
            virtual status convert_to(rotation rotation, rs::utils::smart_ptr<const image_interface> & converted_image) = 0;

            virtual ~image_interface() {}
        };
    }
}

