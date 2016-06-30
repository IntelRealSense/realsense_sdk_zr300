// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs/core/image_interface.h"
#include "rs/core/motion_sample.h"
#include "rs/utils/smart_ptr.h"
#include "stdint.h"

namespace rs
{
    namespace core
    {
        /**
        * @brief The correlated_sample_set struct
        * a container for synced device samples.
        */
        struct correlated_sample_set
        {
            //images of the correlated sample, index by stream_type
            rs::utils::smart_ptr<image_interface> images[static_cast<uint8_t>(stream_type::max)];

            /**
             * @brief images getter indexed by rs::stream
             * @param[in]  stream      the stream type
             */
            __inline rs::utils::smart_ptr<image_interface> &operator[](stream_type stream)
            {
                return images[static_cast<uint8_t>(stream)];
            }

            //motion samples of the correlated sample set, index by motion_type
            motion_sample motion_samples[static_cast<uint8_t>(motion_type::max)];

            /**
             * @brief motion_sample getter indexed by motion_type
             * @param[in]  motion_type      the motion type
             */
            __inline motion_sample &operator[](rs::core::motion_type motion_type)
            {
                return motion_samples[static_cast<uint8_t>(motion_type)];
            }
        };
    }
}

