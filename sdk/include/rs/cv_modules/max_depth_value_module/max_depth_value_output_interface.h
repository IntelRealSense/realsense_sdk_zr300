// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once

namespace rs
{
    namespace cv_modules
    {
        /**
         * @class max_depth_value_output_interface
         * @brief max_depth_value_output_interface is an example for computer vision module interface that calculates the max depth pixel value of the current depth image from the depth images stream.
         *
         * This interface represent a single module specific output. the input stream and motion samples
         * are defined commonly for all the cv modules through the rs::core::video_module_interface.
         */
        class max_depth_value_output_interface
        {
        public:
            /**
             * @struct max_depth_value_output_data
             * @brief max_depth_value_output_data is the cv module output data structure
             */
            struct max_depth_value_output_data
            {
                uint16_t max_depth_value; /** the max depth value */
                uint64_t frame_number;    /** the frame number to which the max depth value was calculated. */
            };

            /**
             * @brief get_max_depth_value_data
             *
             * @return the latest max depth value data
             */
            virtual max_depth_value_output_data get_max_depth_value_data() = 0;
            virtual ~max_depth_value_output_interface() {}
        };
    }
}
