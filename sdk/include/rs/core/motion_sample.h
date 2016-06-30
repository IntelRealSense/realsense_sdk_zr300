// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"

namespace rs
{
    namespace core
    {
        /**
         * @structure motion_sample
         * represent a sample of inertial sensor units.
         */
        struct motion_sample
        {
            /**
             * @param type
             * type of the imu sample
             */
            motion_type  type;

            /**
             * @param timestamp
             * Timestamp of the IMU sample in micro-second unit.
             * If the value is zero, the sample should be considered as invalid.
             **/
            uint64_t     timestamp;

            /**
             * @param frame_number
             * Frame number uses range of 12 bit.
             **/
            uint16_t     frame_number;

            /**
             * @param data
             * Three dimensional sample data represent x, y, z or yaw, pitch, roll.
             */
            float        data[3];
        };
    }
}
