// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/** 
* \file max_depth_value_output_interface.h
* @brief Describes the \c rs::cv_modules::max_depth_value_output_interface class.
**/ 

#pragma once

namespace rs
{
    namespace cv_modules
    {
        /**
         * @brief Example of a computer vision module interface that calculates the maximum depth pixel value of the current depth image from the depth images stream.
         *
         * This interface represent a single module specific output. The input stream and motion samples
         * are defined commonly for all the CV modules through \c rs::core::video_module_interface.
         */
        class max_depth_value_output_interface
        {
        public:
            /**
             * @brief CV module output data structure.
             */
            struct max_depth_value_output_data
            {
                uint16_t max_depth_value; /**< Max depth value */
                uint64_t frame_number;    /**< Frame number for which the maximum depth value was calculated */
            };

            /**
             * @brief Returns latest maximum depth value data.
             *
             * @return Latest maximum depth value data
             */
            virtual max_depth_value_output_data get_max_depth_value_data() = 0;
            virtual ~max_depth_value_output_interface() {}
        };
    }
}
