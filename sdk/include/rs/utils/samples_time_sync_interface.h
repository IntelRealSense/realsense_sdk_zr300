// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.
#pragma once

#include "rs_sdk.h"
#include "rs/utils/cyclic_array.h"

namespace rs
{
    namespace utils
    {
        class samples_time_sync_interface : public rs::core::release_interface
        {
        public:
            /**
            @brief Create and initialize the sync utility - register streams and motions that are required to be synced.
            @param[in]  streams_fps               Array of fps values for every stream needed to be registered. Zero value streams are not registered.
            @param[in]  motions_fps               Array of fps values for every motion needed to be registered. Zero value motions are not registered.
            @param[in]  max_input_latency         The maximum latency in milliseconds that is allowed to be when receiving two frames from
                                                  different streams with same timestamp. Defines the number of frames (max_number_of_buffered_images)
                                                  to be stored in sync utility. Increasing this value will cause a larger number of buffered images.
                                                  Be carefull: Sync utility will buffer the images in it, and those images will not be
                                                  released till the match is found or till the <max_number_of buffered_images> is reached. If you are using
                                                  libRealSense bufferes, please ensure that libRealSense buffer queue size is larger than max_number_of_buffered_images.
                                                  max_number_of_buffered_images = (fps * max_input_latency) / 1000;

            @param[in]	not_matched_frames_buffer_size     Unmatched frames are thrown by default behaviour.  If the user wants to get unmatched frames,
                                                           he may set the value of this variable to non-zero. Sync utility will create buffer of this size and will
                                                           save unmatched frames to this buffer. The user can get unmatched frames from this buffer using
                                                           get_not_matched_frame function (see below).
            */
            static samples_time_sync_interface *
            create_instance(int streams_fps[static_cast<int>(rs::core::stream_type::max)],
                            int motions_fps[static_cast<int>(rs::core::motion_type::max)],
                            const char * device_name
                            );

            static samples_time_sync_interface *
            create_instance(int streams_fps[static_cast<int>(rs::core::stream_type::max)],
                            int motions_fps[static_cast<int>(rs::core::motion_type::max)],
                            const char * device_name,
                            unsigned int max_input_latency
                            );

            static samples_time_sync_interface *
            create_instance(int streams_fps[static_cast<int>(rs::core::stream_type::max)],
                            int motions_fps[static_cast<int>(rs::core::motion_type::max)],
                            const char * device_name,
                            unsigned int max_input_latency,
                            unsigned int not_matched_frames_buffer_size);


            /**
            @brief inserts the new image to the sync utility. Returns true if the correlated sample was found.
            @param[in]  new_image                 New image.
            @param[out] sample_set                Correlated sample containing correlated images and/or motions. May be empty.
                                                  ref counted resources in the sample set must be released by the caller.
            @return                               true if the match was found
            */
            virtual bool insert(rs::core::image_interface * new_image, rs::core::correlated_sample_set& sample_set)= 0;

            /**
            @brief inserts the new motion to the sync utility. Returns true if the correlated sample was found.
            @param[in]  new_motion                New motion.
            @param[out] sample_set                Correlated sample containing correlated images and/or motions. May be empty.
                                                  ref counted resources in the sample set must be released by the caller.
            @return                               true if the match was found            */
            virtual bool insert(rs::core::motion_sample& new_motion, rs::core::correlated_sample_set& sample_set) = 0;


            /**
            @brief Puts the first unmatched frame of stream_type to the location specified by second parameter.
                   Returns true if there are more unmatched frames of this stream_type available.
            @param[out]  not_matched_frame         Smart_ptr to put unmatched frame to
            @return                               true if more unmatched frames available      */
            virtual bool get_not_matched_frame(rs::core::stream_type stream_type, rs::core::image_interface ** not_matched_frame) = 0;

            /**
            @brief Removes all the frames from the internal lists.
            @return void
            */
            virtual void flush() = 0;


            virtual ~samples_time_sync_interface() { };

        };

    }
}

