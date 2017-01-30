// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/**
* @file samples_time_sync_interface.h
* @brief Describes for the \c rs::utils::samples_time_sync_interface class.
*/

#pragma once

#include "rs_sdk.h"
#include "rs/utils/cyclic_array.h"

#ifdef WIN32 
#ifdef realsense_samples_time_sync_EXPORTS
#define  DLL_EXPORT __declspec(dllexport)
#else
#define  DLL_EXPORT __declspec(dllimport)
#endif /* realsense_samples_time_sync_EXPORTS */
#else /* defined (WIN32) */
#define DLL_EXPORT
#endif

namespace rs
{
    namespace utils
    {
         /**
         * @brief Defines the interface for the sync utilities classes for different
         *        cameras, and contains static factory methods for getting the sync utility instance.
         */
        class DLL_EXPORT samples_time_sync_interface : public rs::core::release_interface
        {
        public:
    
            /** @brief The device_name for external devices
             *
             * Use this string as the device_name parameter  when you call samples_time_sync_interface::create_instance
             * to create a samples_time_sync implementation which synchronizes images between a librealsense camera
             * and an external device (which doesn't get timestamps from the camera's microcontroller)
             */
            static constexpr const char* external_device_name = "external_device";
    
            /**
            * @brief Creates and initializes the sync utility: registers streams and motions that are required to be synced.
            * @param[in]  streams_fps               Array of frames per second (FPS) values for every stream needed to be registered. Zero value streams are not registered.
            * @param[in]  motions_fps               Array of FPS values for every motion needed to be registered. Zero value motions are not registered.
            * @param[in]  device_name               Device name. Defines the type of the \c sync_utility which will be created.
            */
            static samples_time_sync_interface *
            create_instance(int streams_fps[static_cast<int>(rs::core::stream_type::max)],
                            int motions_fps[static_cast<int>(rs::core::motion_type::max)],
                            const char * device_name
                            );
            /**
            * @brief Creates and initializes the sync utility: registers streams and motions that are required to be synced.
            * @param[in]  streams_fps               Array of frames per second (FPS) values for every stream needed to be registered. Zero value streams are not registered.
            * @param[in]  motions_fps               Array of FPS values for every motion needed to be registered. Zero value motions are not registered.
            * @param[in]  device_name               Device name. Defines the type of the \c sync_utility which will be created.
            * @param[in]  max_input_latency         The maximum latency in milliseconds that is allowed when receiving two frames from
            *                                       different streams with the same timestamp. Defines the number of frames (\c max_number_of_buffered_images)
            *                                       to be stored in the sync utility. Increasing this value will cause a larger number of buffered images.
            *                                       Be careful: The sync utility will buffer the images in it, and those images will not be
            *                                       released until a match is found or until the \c max_number_of buffered_images is reached. If you are using
            *                                       librealsense buffers, ensure that the librealsense buffer queue size is larger than \c max_number_of_buffered_images.
            *                                       Note that <tt>max_number_of_buffered_images = (fps * max_input_latency) / 1000 </tt>;
            */
            static samples_time_sync_interface *
            create_instance(int streams_fps[static_cast<int>(rs::core::stream_type::max)],
                            int motions_fps[static_cast<int>(rs::core::motion_type::max)],
                            const char * device_name,
                            unsigned int max_input_latency
                            );
            /**
            * @brief Creates and initializes the sync utility: registers streams and motions that are required to be synced.
            * @param[in]  streams_fps               Array of frames per second (FPS) values for every stream needed to be registered. Zero value streams are not registered.
            * @param[in]  motions_fps               Array of FPS values for every motion needed to be registered. Zero value motions are not registered.
            * @param[in]  device_name               Device name. Defines the type of the \c sync_utility which will be created.
            * @param[in]  max_input_latency         The maximum latency in milliseconds that is allowed when receiving two frames from
            *                                       different streams with same timestamp. Defines the number of frames (\ max_number_of_buffered_images)
            *                                       to be stored in the sync utility. Increasing this value will cause a larger number of buffered images.
            *                                       Be careful: The sync utility will buffer the images in it, and those images will not be
            *                                       released until a match is found or until the \c max_number_of buffered_images is reached. If you are using
            *                                       librealsense buffers, ensure that the librealsense buffer queue size is larger than \c max_number_of_buffered_images.
            *                                       Note that <tt>max_number_of_buffered_images = (fps * max_input_latency) / 1000 </tt>;
            *
            * @param[in]	not_matched_frames_buffer_size   Unmatched frames are thrown by default behaviour.  If you want to get unmatched frames,
            *                                                you may set the value of this variable to non-zero. The sync utility will create a buffer of this size and will
            *                                                save unmatched frames to this buffer. You can get unmatched frames from this buffer using
            *                                                the \c get_not_matched_frame() method.
            */
            static samples_time_sync_interface *
            create_instance(int streams_fps[static_cast<int>(rs::core::stream_type::max)],
                            int motions_fps[static_cast<int>(rs::core::motion_type::max)],
                            const char * device_name,
                            unsigned int max_input_latency,
                            unsigned int not_matched_frames_buffer_size);


            /**
            * @brief Inserts the new image sample to the sync utility. Returns true if the correlated sample was found.
            * @param[in]  new_image                 New image
            * @param[out] sample_set                Correlated sample containing correlated images and/or motions. May be empty.
            *                                       Reference counted resources in the sample set must be released by the caller.
            * @return bool                          true if the match was found
            */
            virtual bool insert(rs::core::image_interface * new_image, rs::core::correlated_sample_set& sample_set)= 0;

            /**
            * @brief Inserts the new motion sample to the sync utility. Returns true if the correlated sample was found.
            * @param[in]  new_motion                New motion
            * @param[out] sample_set                Correlated sample containing correlated images and/or motions. May be empty.
            *                                       Reference counted resources in the sample set must be released by the caller.
            * @return bool                          true if the match was found
            */
            virtual bool insert(rs::core::motion_sample& new_motion, rs::core::correlated_sample_set& sample_set) = 0;


            /**
            * @brief Puts the first (if available) unmatched frame of \c stream_type to the location specified by \c not_matched_frame.
			
            *        Returns true if there are more unmatched frames of this stream type available.
            * @param[in]   stream_type              Stream from which to get unmatched frame
            * @param[out]  not_matched_frame        \c smart_ptr to put unmatched frame to
            * @return bool                          true if more unmatched frames are available
            */
            virtual bool get_not_matched_frame(rs::core::stream_type stream_type, rs::core::image_interface ** not_matched_frame) = 0;

            /**
            * @brief Removes all the frames from the internal lists.
            * @return void
            */
            virtual void flush() = 0;


            virtual ~samples_time_sync_interface() { };

        };

    }
}

