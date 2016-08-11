// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "metadata_interface.h"
#include "rs/utils/smart_ptr.h"
#include "types.h"

namespace rs
{   /**
     * forward declaraion of librealsense frame.
     */
    class frame;

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
            virtual double query_time_stamp(void) const = 0;

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
            @brief Return the image frame number..
            @return the frame number.
            */
            virtual uint64_t query_frame_number(void) const = 0;

            /**
            @brief Return metadata of the image.
            @return smart_ptr<metadata_interface>      image metadata
            */
            virtual rs::utils::smart_ptr<metadata_interface> query_metadata() = 0;

            /**
            @brief Convert the current image image to a given format.
            @param[in]  format                    Destination format.
            @param[out] converted_image           Converted image allocated internaly.
            @return STATUS_NO_ERROR               Successful execution.
            @return STATUS_PARAM_UNSUPPORTED      Convertion to this format is currently unsupported.
            @return STATUS_FEATURE_UNSUPPORTED    The feature is currently unsupported.
            @return STATUS_EXEC_ABORTED           Failed to convert.
            */
            virtual status convert_to(pixel_format format, rs::utils::smart_ptr<const image_interface> & converted_image) = 0;

            /**
            @brief Convert the current image image to a given format.
            @param[in]  rotation                  Destination rotation.
            @param[out] converted_image           Converted image allocated internaly.
            @return STATUS_NO_ERROR               Successful execution.
            @return STATUS_PARAM_UNSUPPORTED      The given rotation is currently unsupported.
            @return STATUS_FEATURE_UNSUPPORTED    The feature is currently unsupported.
            @return STATUS_EXEC_ABORTED           Failed to convert.
            */
            virtual status convert_to(rotation rotation, rs::utils::smart_ptr<const image_interface> & converted_image) = 0;

            virtual ~image_interface() {}

            /**
             * @brief create_instance_from_librealsense_frame.
             * sdk image implementation for a frame defined by librealsense.
             * takes ownership on the frame.
             * @param frame - frame object defined by librealsense (rs::frame)
             * @param flags - optional flags, place holder for future options
             * @param metadata - image extended metadata
             * @return image_interface object
             */
            static image_interface * create_instance_from_librealsense_frame(rs::frame& frame,
                                                                             flag flags,
                                                                             rs::utils::smart_ptr<metadata_interface> metadata);

            /**
             * @brief The data_releaser_interface class
             * optional custom deallocation method to be called by the image destructor.
             */
            class data_releaser_interface
            {
            public:
                virtual void release() = 0;
                virtual ~data_releaser_interface() {}
            };

            /**
             * @brief create_instance_from_raw_data
             * sdk image implementation from raw data, where the user provides an allocated image data and
             * an optional image deallocation method with the data_releaser_interface, if no deallocation method is provided,
             * it assumes that the user is handling memory deallocation outside of the custom image class.
             * @param info - info required to successfully traverse the image data/
             * @param data - the image raw data.
             * @param stream - the stream type.
             * @param flags - optional flags, place holder for future options.
             * @param time_stamp - the timestamp of the image, in milliseconds since the device was started.
             * @param frame_number - the number of the image, since the device was started.
             * @param metadata - image extended metadata.
             * @param data_releaser - optional data releaser, if null, no deallocation is done.
             * @return image_interface object
             */
            static image_interface * create_instance_from_raw_data(image_info * info,
                                                                   const void * data,
                                                                   stream_type stream,
                                                                   image_interface::flag flags,
                                                                   double time_stamp,
                                                                   uint64_t frame_number,
                                                                   rs::utils::smart_ptr<metadata_interface> metadata,
                                                                   rs::utils::smart_ptr<data_releaser_interface> data_releaser);
        };
    }
}

