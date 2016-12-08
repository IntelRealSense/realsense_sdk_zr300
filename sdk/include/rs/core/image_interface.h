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

#ifdef WIN32 
#ifdef realsense_image_EXPORTS
#define  DLL_EXPORT __declspec(dllexport)
#else
#define  DLL_EXPORT __declspec(dllimport)
#endif /* realsense_image_EXPORTS */
#else /* defined (WIN32) */
#define DLL_EXPORT
#endif

namespace rs
{   /**
     * @brief Forward declaration of \c rs::frame. Required to create an image from librealsense frame input.
     */
    class frame;

    namespace core
    {
        /**
        * @brief Describes detailed image data.
        */
        struct image_info
        {
            int32_t       width;  /**< Width of the image in pixels                        */
            int32_t       height; /**< Height of the image in pixels                       */
            pixel_format  format; /**< Image pixel format                                  */
            int32_t       pitch;  /**< Pitch of the image in pixels - also known as stride */
        };

        /**
        * @brief Image interface abstracts interactions with a single image.
        *
        * The image interface provides access to the image raw buffer, for read only operations, as well as the image info, which is required to parse 
        * the raw buffer. It may also include additional buffers produced during image format conversions, and image metadata. The metadata may contain
        * additional information attached to the image, such as camera data or processing information related to the image.
        * The image origin is usually a single camera stream. Otherwise, it's a synthetic image, created from any raw buffer.
        * The image lifetime is managed by the image user, through calling the inherent \c ref_count_interface. The user must increase the image reference count
        * when required, and release the image, instead of deleting the object directly. This interface is designed to conform with ABI compatibility requirements.  
        */
        class DLL_EXPORT image_interface : public ref_count_interface
        {
        public:
            /**
            * @brief Describes image flags
            * 
            * Currently no image flags are exposed. 
            */
            enum flag
            {
                any = 0x0
            };

            /**
            * @brief Returns image sample information.
            * 
            * The image information includes the required data to parse the image raw buffer.
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
            * The timestamp domain represents the clock, which produced the image timestamp. It can be internal camera clock, or external clock, which synchronizes a few sensors timestamps. 
            * The timestamp domain of two images must match for the timestamps to be comparable. 
            * @return timestamp_domain Timestamp domain value
            */
            virtual timestamp_domain query_time_stamp_domain(void) const = 0;

            /**
            * @brief Gets the image flags.
            *
            * @return \c image_interface::flag flags
            */
            virtual flag query_flags(void) const = 0;

            /**
            * @brief Gets the image data.
            * 
            * Provides a pointer to the image raw buffer, for read only operations. To convert the pixel format, convert_to function should be called. 
            * To modify the image, the user can copy the image buffer, and create a new image from this data using \c create_instance_from_raw_data.
            * @return const void* Data 
            */
            virtual const void * query_data(void) const = 0;

            /**
            * @brief Returns the image stream type. 
			* 
            * The image stream type represents the camera type from which the image was produced. 
            * @return stream_type Stream type
            */
            virtual stream_type query_stream_type(void) const = 0;

            /**
            * @brief Returns the image frame number.
            * 
            * The image sequence number in the camera stream. 
            * @return uint64_t Number
            */
            virtual uint64_t query_frame_number(void) const = 0;

            /**
            * @brief Returns metadata of the image.
            *
            * The image metadata can include information items, which are relevant to the image, such as camera capture information, or image processing
            * information. The user can access \c metadata_interface in order to read or attach new metadata items. 
            * @return metadata_interface * Metadata
            */
            virtual metadata_interface* query_metadata() = 0;

            /**
            * @brief Creates a converted image from the current image and a given pixel format.
            *
            * The function creates a converted image from the current image buffer to the requested pixel format, if such conversion is supported. 
            * The converted image is cached by the original image, so that multiple requests for the same conversion are calculated only once.
            * On a successful conversion the calling user shares the image ownership with the original image instance, the user is obligated to release 
			* the image in his context. its recommended to use \c sdk/include/rs/utils/smart_ptr_helpers.h helper functions to wrap the image object 
			* for automatic image release mechanism.
            * @param[in]  format                    Destination format
            * @param[out] converted_image           Converted image allocated internally
            * @return status_no_error               Successful execution
            * @return status_param_unsupported      Conversion to this format is currently unsupported.
            * @return status_feature_unsupported    The feature is currently unsupported.
            * @return status_exec_aborted           Failed to convert
            */
            virtual status convert_to(pixel_format format, const image_interface ** converted_image) = 0;

            /**
            * @brief Creates a rotated image from the current image and a given rotation parameter.
            *
			* The feature is currently unsupported.
            * @param[in]  rotation                  Destination rotation
            * @param[out] converted_image           Converted image allocated internally
            * @return status_no_error               Successful execution
            * @return status_param_unsupported      This rotation is currently unsupported.
            * @return status_feature_unsupported    The feature is currently unsupported.
            * @return status_exec_aborted           Failed to convert
            */
            virtual status convert_to(rotation rotation, const image_interface** converted_image) = 0;

            /**
            * @brief SDK image implementation for a frame defined by librealsense.
            *
            * The returned image takes ownership of the \c rs::frame, meaning that the input frame parameter is moved after the image instance is created.
			* The returned image instance will have reference count of 1, to release the image call release instead of delete. its recommended to use 
			* \c sdk/include/rs/utils/smart_ptr_helpers.h helper functions to wrap the image object for automatic image release mechanizm.
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

                const void * data; 				   /**< Image data pointer. */
                release_interface * data_releaser; /**< Data releaser defined by the user, which serves as a custom deleter for the image data.
                                                        Upon calling the interface release method, this object should release the image data and
                                                        the data releaser memory. A null \c data_releaser means that the image data is managed by the user
                                                        outside of the image class. For a simple data releaser implementation that deletes the data
                                                        pointer with <tt> delete[]</tt>, use sdk/include/rs/utils/self_releasing_array_data_releaser.h. */
            };

            /**
            * @brief SDK image implementation from raw data
            *
            * The function creates an \c image_interface object from the input data. The user provides an allocated image data and
            * an optional image deallocation method with the \c release_interface, by implementing its release function. If no deallocation method is provided,
            * It assumes that the user is handling memory deallocation outside of the image interface instance.
			* The returned image instance will have reference count of 1, to release the image call release instead of delete. its recommended to use 
			* \c sdk/include/rs/utils/smart_ptr_helpers.h helper functions to wrap the image object for automatic image release mechanism. 
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

        /**
        * @brief Providing pixel byte size for a given pixel format.
        * @param[in] format     Pixel format.
        * @return int8_t        Byte size of the given pixel format.
        */
        static int8_t get_pixel_size(rs::core::pixel_format format)
        {
            switch(format)
            {
                case rs::core::pixel_format::any:return 0;
                case rs::core::pixel_format::z16:return 2;
                case rs::core::pixel_format::disparity16:return 2;
                case rs::core::pixel_format::xyz32f:return 4;
                case rs::core::pixel_format::yuyv:return 2;
                case rs::core::pixel_format::rgb8:return 3;
                case rs::core::pixel_format::bgr8:return 3;
                case rs::core::pixel_format::rgba8:return 4;
                case rs::core::pixel_format::bgra8:return 4;
                case rs::core::pixel_format::y8:return 1;
                case rs::core::pixel_format::y16:return 2;
                case rs::core::pixel_format::raw8:return 1;
                case rs::core::pixel_format::raw10:return 0;//not supported
                case rs::core::pixel_format::raw16:return 2;
                default: return 0;
            }
        }
    }
}

