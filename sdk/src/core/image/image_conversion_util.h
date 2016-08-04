// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs/core/types.h"
#include "rs/core/status.h"

namespace rs
{
    namespace core
    {
        class image_conversion_util
        {
            image_conversion_util() = delete;
            image_conversion_util(const image_conversion_util &) = delete;
            image_conversion_util & operator = (const image_conversion_util &) = delete;
            ~image_conversion_util() = delete;
        public:
            static status convert(const image_info &src_info, const uint8_t *src_data, const image_info &dst_info, uint8_t *dst_data);
            static status is_conversion_valid(const image_info &src_info, const image_info &dst_info);
        private:
            static int rs_format_to_cv_pixel_type(rs::core::pixel_format format);
            static int get_cv_convert_enum(rs::core::pixel_format from, rs::core::pixel_format to);
        };
    }
}
