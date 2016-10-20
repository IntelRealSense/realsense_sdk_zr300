// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs/core/image_interface.h"
#include "rs/core/motion_sample.h"
#include "rs/utils/smart_ptr_helpers.h"
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
            inline correlated_sample_set() : images(), motion_samples() {}

            //images of the correlated sample, index by stream_type
            image_interface* images[static_cast<uint8_t>(stream_type::max)];

            //motion samples of the correlated sample set, index by motion_type
            motion_sample motion_samples[static_cast<uint8_t>(motion_type::max)];

            /**
             * @brief access the image indexed by stream
             * @param[in]  stream      the stream type
             */
            inline image_interface* &operator[](stream_type stream)
            {
                return images[static_cast<uint8_t>(stream)];
            }

            /**
             * @brief const access to the image indexed by stream
             * @param[in]  stream      the stream type
             */
            inline image_interface* operator[](stream_type stream) const
            {
                return images[static_cast<uint8_t>(stream)];
            }

            /**
             * @brief take unique ownership of the image indexed by stream
             * @param[in]  stream      the stream type
             */
            inline rs::utils::unique_ptr<image_interface> get_unique(stream_type stream) const
            {
                auto image = images[static_cast<uint8_t>(stream)];
                if(image)
                {
                    image->add_ref();
                }
                return rs::utils::get_unique_ptr_with_releaser(image);
            }

            /**
             * @brief motion_sample getter indexed by motion_type
             * @param[in]  motion_type      the motion type
             */
            inline motion_sample &operator[](rs::core::motion_type motion_type)
            {
                return motion_samples[static_cast<uint8_t>(motion_type)];
            }

            /**
             * @brief const motion_sample getter indexed by motion_type
             * @param[in]  motion_type      the motion type
             */
            inline const motion_sample &operator[](rs::core::motion_type motion_type) const
            {
                return motion_samples[static_cast<uint8_t>(motion_type)];
            }
        };
    }
}

