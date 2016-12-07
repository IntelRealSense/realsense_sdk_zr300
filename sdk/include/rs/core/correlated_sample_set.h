// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/**
* \file correlated_sample_set.h
* @brief Describes the \c rs::core::correlated_sample_set struct.
*/

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
        * @brief 
        * A container for synced device samples.
		*
        * It contains at most a single image of each camera stream, and at most a single motion sample for each motion type.
        */
        struct correlated_sample_set
        {
            inline correlated_sample_set() : images(), motion_samples() {}

            image_interface* images[static_cast<uint8_t>(stream_type::max)];      /**< images of the correlated sample, index by stream_type             */
            motion_sample motion_samples[static_cast<uint8_t>(motion_type::max)]; /**< motion samples of the correlated sample set, index by motion_type */

            /**
             * @brief Accesses image indexed by stream
             *
             * @param[in]  stream Stream type
             * @return \c image_interface *  Image instance
             */
            inline image_interface* &operator[](stream_type stream)
            {
                return images[static_cast<uint8_t>(stream)];
            }

            /**
             * @brief Provides \c const access to the image indexed by stream
             *
             * @param[in]  stream Stream type
             * @return image_interface * Image instance
             */
            inline image_interface* operator[](stream_type stream) const
            {
                return images[static_cast<uint8_t>(stream)];
            }

            /**
             * @brief A wrapper of a stream image with a \c unique_ptr.
             *
             * Gets a unique managed ownership of an explicit image reference by calling \c rs::core::ref_count_interface::add_ref()
             * and wrapping the image with a \c unique_ptr with a custom deleter that calls release on destruction.
             * @param[in] stream Stream type
             * @return rs::utils::unique_ptr<image_interface> Managed image instance
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
             * @brief A getter indexed by motion_type.
             *
             * @param[in] motion_type Motion type
             * @return motion_sample Motion sample
             */
            inline motion_sample &operator[](rs::core::motion_type motion_type)
            {
                return motion_samples[static_cast<uint8_t>(motion_type)];
            }

            /**
             * @brief A \c motion_sample getter indexed by \c motion_type
             *
             * @param[in] motion_type Motion type
             * @return motion_sample Motion sample
             */
            inline const motion_sample &operator[](rs::core::motion_type motion_type) const
            {
                return motion_samples[static_cast<uint8_t>(motion_type)];
            }
        };
    }
}

