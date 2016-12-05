// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/**
* \file image_interface.h
* @brief Describes the \c rs::core::image_interface class and \c rs::core::image_interface::image_data_with_data_releaser struct.
*/

#pragma once
#include "metadata_interface.h"
#include "rs/core/ref_count_interface.h"
#include "rs/utils/release_self_base.h"
#include "types.h"

namespace rs
{   /**
     * @brief
     * Forward declaration of \c rs::frame.
     */
    class frame;

    namespace core
    {
        /**
        * @brief Image interface - abstracts interactions with images.
        *
        * Due to an ABI restriction, the \c image_interface object memory is managed by the inherent \c ref_count_interface,
        * so users must release the image memory using the release method, instead of deleting the object directly.
        */
        class image_interface : public ref_count_interface
        {
        public:
            /**
            * @brief Describes image flags
            */
            enum flag
            {
                any = 0x0
            };

            /**
            * @brief Returns image sample information.
            * @return Image sample information in the \c image_info structure

            */
            virtual image_info query_info(void) const = 0;

            /**
            * @brief Gets the image timestamp.
            * @return double Timestamp value, in milliseconds, since the device was started
            */
            virtual double query_time_stamp(void) const = 0;

            /**
            * @brief Gets the image timestamp domain
            *
            * Used to check if two timestamp values are comparable (that is, generated from the same clock).
            * @return timestamp_domain Timestamp domain value
            */
            virtual timestamp_domain query_time_stamp_domain(void) const = 0;

            /**
            * @brief Gets the image flags.
            * @return \c image_interface::flag flags
            */
            virtual flag query_flags(void) const = 0;

            /**
            * @brief Gets the image data.
            *
            * @return const void * Data.
            */
            virtual const void * query_data(void) const = 0;

            /**
            * @brief Returns the image stream type.
            *
            * @return stream_type Stream type.
            */
            virtual stream_type query_stream_type(void) const = 0;

            /**
            * @brief Returns the image frame number.
            *
            * @return uint64_t Number.
            */
            virtual uint64_t query_frame_number(void) const = 0;

            /**
            * @brief Returns metadata of the image.
            *
            * @return metadata_interface * Metadata
            */
            virtual metadata_interface* query_metadata() = 0;

            /**
            * @brief Converts the current image to a given format.
            *
            * @param[in]  format                    Destination format
            * @param[out] converted_image           Converted image allocated internally
            * @return status_no_error               Successful execution
            * @return status_param_unsupported      Conversion to this format is currently unsupported.
            * @return status_feature_unsupported    The feature is currently unsupported.
            * @return status_exec_aborted           Failed to convert
            */
            virtual status convert_to(pixel_format format, const image_interface ** converted_image) = 0;

            /**
            * @brief Converts the current image image to a given format
            *
            * @param[in]  rotation                  Destination rotation
            * @param[out] converted_image           Converted image allocated internally
            * @return status_no_error               Successful execution
            * @return status_param_unsupported      Conversion to this format is currently unsupported.
            * @return status_feature_unsupported    The feature is currently unsupported.
            * @return status_exec_aborted           Failed to convert
            */
            virtual status convert_to(rotation rotation, const image_interface** converted_image) = 0;

            /**
             * @brief SDK image implementation for a frame defined by librealsense.
             *
             * 
             * The returned image takes ownership of the frame, thus the input frame parmeter is invalid on return.
             * @param frame                 Frame object defined by librealsense (\c rs::frame)
             * @param[in] flags             Optional flags - place holder for future options
             * @return \c image_interface*    Object
             */
            static image_interface * create_instance_from_librealsense_frame(rs::frame& frame,
                                                                             flag flags);

            /**
             * @brief Container to unify the image data pointer and a data releaser.
             */
            struct image_data_with_data_releaser
            {
            public:
                image_data_with_data_releaser(const void * data, release_interface * data_releaser = nullptr): data(data), data_releaser(data_releaser) {}

                const void * data; /**< Image data pointer */
                release_interface * data_releaser; /**< Data releaser defined by the user, which serves as a custom deleter for the image data.
                                                        Upon calling the interface release method, this object should release the image data and
                                                        the data releaser memory. A null \c data_releaser means that the image data is managed by the user
                                                        outside of the image class. For a simple data releaser implementation that deletes the data
                                                        pointer with <tt> delete[]</tt>, use sdk/include/rs/utils/self_releasing_array_data_releaser.h. */
            };

            /**
             * @brief SDK image implementation from raw data.
             *
             * In this case, the user provides an allocated image data and
             * an optional image deallocation method with the \c data_releaser_interface, if no deallocation method is provided.
             * It assumes that the user is handling memory deallocation outside of the custom image class.
             * @param[in] info                  Info required to successfully traverse the image data
             * @param[in] data_container        Image data and the data releasing handler. The releasing handler release method will be called by
             *                                  the image destructor. A null \c data_releaser means the user is managing the image data outside of the image instance.
             * @param[in] stream                Stream type
             * @param[in] flags                 Optional flags - place holder for future options
             * @param[in] time_stamp            Timestamp of the image, in milliseconds since the device was started
             * @param[in] frame_number          Number of the image, since the device was started
             * @param[in] time_stamp_domain     Domain in which the timestamp was generated
             * @return image_interface * 		Image instance
             */
            static image_interface * create_instance_from_raw_data(image_info * info,
                                                                   const image_data_with_data_releaser &data_container,
                                                                   stream_type stream,
                                                                   image_interface::flag flags,
                                                                   double time_stamp,
                                                                   uint64_t frame_number,
                                                                   timestamp_domain time_stamp_domain = timestamp_domain::camera);
        protected:
            virtual ~image_interface() {}
        };
    }
}

