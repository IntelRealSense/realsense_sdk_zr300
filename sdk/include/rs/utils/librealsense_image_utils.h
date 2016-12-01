// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs/utils/librealsense_conversion_utils.h"
#include "image_utils.h"
namespace rs
{
    namespace utils
    {
        static int8_t get_pixel_size(rs::format format)
        {
            return get_pixel_size(convert_pixel_format(format));
        }
        static int8_t get_pixel_size(rs_format format)
        {
            return get_pixel_size(static_cast<rs::format>(format));
        }
    }
}

