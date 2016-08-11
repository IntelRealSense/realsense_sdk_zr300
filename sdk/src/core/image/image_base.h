// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs/core/image_interface.h"
#include <map>
#include <mutex>

namespace rs
{
    namespace core
    {
        /**
         * @brief The image_base class
         * base implementation to common image api.
         */
        class image_base : public image_interface
        {
        public:
            image_base(rs::utils::smart_ptr<metadata_interface> metadata);
            virtual rs::utils::smart_ptr<metadata_interface> query_metadata() override;
            virtual status convert_to(pixel_format format, rs::utils::smart_ptr<const image_interface> & converted_image) override;
            virtual status convert_to(rs::core::rotation rotation, rs::utils::smart_ptr<const image_interface> & converted_image) override;
            virtual ~image_base() {}
        protected:
            rs::utils::smart_ptr<metadata_interface> metadata;
            std::map<pixel_format, rs::utils::smart_ptr<const image_interface>> image_cache_per_pixel_format;
            std::mutex image_caching_lock;
        };
    }
}
