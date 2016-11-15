// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"

namespace rs
{
    namespace core
    {
        /**
         * @struct motion_sample
         *
         * represent a sample of inertial sensor units.
         */
        struct motion_sample
        {
            motion_type  type;         /** Type of the imu sample */
            double       timestamp;    /** Timestamp of the IMU sample in milliseconds units.
                                           If the value is zero, the sample should be considered as invalid.*/
            uint64_t     frame_number; /** The sample frame number */
            float        data[3];      /** Three dimensional sample data represent x, y, z or yaw, pitch, roll. */
        };
    }
}
