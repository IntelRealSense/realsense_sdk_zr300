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
        * @struct The correlated_sample_set
        *
        * a container for time synced device samples.
        */
        struct correlated_sample_set
        {
            inline correlated_sample_set() : images(), motion_samples() {}

            image_interface* images[static_cast<uint8_t>(stream_type::max)]; /** images of the correlated sample, index by stream_type */
            motion_sample motion_samples[static_cast<uint8_t>(motion_type::max)]; /** motion samples of the correlated sample set, index by motion_type */

            /**
             * @brief access the image indexed by stream
             *
             * @param[in]  stream      the stream type.
             * @return image_interface *    an image instance.
             */
            inline image_interface* &operator[](stream_type stream)
            {
                return images[static_cast<uint8_t>(stream)];
            }

            /**
             * @brief const access to the image indexed by stream
             *
             * @param[in]  stream      the stream type.
             * @return image_interface *    an image instance.
             */
            inline image_interface* operator[](stream_type stream) const
            {
                return images[static_cast<uint8_t>(stream)];
            }

            /**
             * @brief wrapping of a stream image with an unique_ptr.
             *
             * get a unique managed ownership of an explicit image reference by calling add_ref
             * and wrapping the image with a unique_ptr with a custom deleter that calls release on destruction.
             * @param[in]  stream      the stream type.
             * @return rs::utils::unique_ptr<image_interface>    a managed image instance.
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
             * @brief motion_sample getter indexed by motion_type.
             *
             * @param[in]  motion_type      the motion type.
             * @return motion_sample    a motion sample.
             */
            inline motion_sample &operator[](rs::core::motion_type motion_type)
            {
                return motion_samples[static_cast<uint8_t>(motion_type)];
            }

            /**
             * @brief const motion_sample getter indexed by motion_type
             *
             * @param[in]  motion_type      the motion type.
             * @return motion_sample    a motion sample.
             */
            inline const motion_sample &operator[](rs::core::motion_type motion_type) const
            {
                return motion_samples[static_cast<uint8_t>(motion_type)];
            }
        };
    }
}

