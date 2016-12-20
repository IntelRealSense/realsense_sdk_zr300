// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs/utils/ref_count_base.h"
#include "rs/utils/smart_ptr_helpers.h"
#include "rs/core/image_interface.h"
#include "metadata.h"
#include <map>
#include <mutex>

#ifdef WIN32 
#ifdef realsense_image_EXPORTS
#define  DLL_EXPORT __declspec(dllexport)
#else
#define  DLL_EXPORT __declspec(dllimport)
#endif /* realsense_image_EXPORTS */
#else /* defined (WIN32) */
#define DLL_EXPORT
#endif

namespace rs
{
    namespace core
    {
        /**
         * @brief The image_base class
         * base implementation to common image api.
         */
        class DLL_EXPORT image_base : public rs::utils::ref_count_base<image_interface>
        {
        public:
            image_base();
            virtual metadata_interface* query_metadata() override;
            virtual status convert_to(pixel_format format, const image_interface ** converted_image) override;
            virtual status convert_to(rs::core::rotation rotation, const image_interface ** converted_image) override;

        protected:
            std::map<pixel_format, rs::utils::unique_ptr<const image_interface>> image_cache_per_pixel_format;
            std::mutex image_caching_lock;
            virtual ~image_base() = default;
        private:
            rs::core::metadata metadata;
        };
    }
}
