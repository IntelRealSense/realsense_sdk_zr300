// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once

namespace rs
{
    namespace cv_modules
    {
        /**
         * @brief The max_depth_value_module_interface class
         * an example for computer vision module interface that calculates the max depth pixel value of the current depth image
         * from the depth images stream. This interface represent a single module specific output. the input stream and motion samples
         * are defined commonly for all the cv modules through the rs::core::video_module_interface.
         */
        class max_depth_value_module_interface
        {
        public:
            /**
             * @brief The max_depth_value_output_data struct
             * the cv module output data container
             */
            struct max_depth_value_output_data
            {
                uint16_t max_depth_value;
                uint64_t frame_number;
            };

            /**
             * @brief get_max_depth_value_data
             * @return the latest max depth value data
             */
            virtual max_depth_value_output_data get_max_depth_value_data() = 0;
            virtual ~max_depth_value_module_interface() {}
        };
    }
}
