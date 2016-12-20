// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/**
* \file motion_sample.h
* @brief Describes the \c rs::core::motion_sample struct. 
*/

#pragma once

#include "types.h"

namespace rs
{
    namespace core
    {
         /**
         * @brief Represents a sample of inertial sensor units.
         */
        struct motion_sample
        {
            motion_type  type;         /**< Type of the IMU sample */
            double       timestamp;    /**< Timestamp of the IMU sample in millisecond units.
                                           If the value is zero, the sample should be considered as invalid.*/
            uint64_t     frame_number; /**< Sample frame number */
            float        data[3];      /**< Three-dimensional sample data representing x, y, z or yaw, pitch, roll */
        };
    }
}
