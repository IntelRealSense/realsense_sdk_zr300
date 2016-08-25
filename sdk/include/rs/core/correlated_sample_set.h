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
            inline correlated_sample_set() : images(), motion_samples()
            {
            }

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
             * @brief take shared ownership of the image indexed by stream
             * @param[in]  stream      the stream type
             */
            inline std::shared_ptr<image_interface> take_shared(stream_type stream)
            {
                std::shared_ptr<image_interface> image = std::move(rs::utils::get_unique_ptr_with_releaser(images[static_cast<uint8_t>(stream)]));
                images[static_cast<uint8_t>(stream)] = nullptr;
                return image;
            }

            /**
             * @brief take unique ownership of the image indexed by stream
             * @param[in]  stream      the stream type
             */
            inline rs::utils::unique_ptr<image_interface> take_unique(stream_type stream)
            {
                auto image = rs::utils::get_unique_ptr_with_releaser(images[static_cast<uint8_t>(stream)]);
                images[static_cast<uint8_t>(stream)] = nullptr;
                return image;
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
             * @brief release
             * ref counted samples in the sample set are +1 ref counted, it the responsibility of the sample set user to
             * release all ref counted samples in the sample set. this release function will release all existing ref counted
             * samples in the sample set.
             */
            void release()
            {
                for(int i = 0; i < static_cast<uint8_t>(stream_type::max); i++)
                {
                    if(images[i])
                    {
                        images[i]->release();
                        images[i] = nullptr;
                    }
                }
            }
        };
    }
}

