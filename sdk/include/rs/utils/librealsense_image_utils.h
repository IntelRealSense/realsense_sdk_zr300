// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.


/**
* \file librealsense_image_utils.h
* @brief Describes utilities for images using librealsense types.
*/

#pragma once
#include "rs/utils/librealsense_conversion_utils.h"
#include "rs/utils/image_utils.h"
namespace rs
{
    namespace utils
    {
        /**
        * @brief Providing pixel byte size for a given librealsense CPP style pixel format.
        * @param[in] format     Librealsense CPP style pixel format.
        * @return int8_t        Byte size of the given pixel format.
        */
        static int8_t get_pixel_size(rs::format format)
        {
            return get_pixel_size(convert_pixel_format(format));
        }

        /**
        * @brief Providing pixel byte size for a given librealsense C style pixel format.
        * @param[in] format     Librealsense C style pixel format.
        * @return int8_t        Byte size of the given pixel format.
        */
        static int8_t get_pixel_size(rs_format format)
        {
            return get_pixel_size(static_cast<rs::format>(format));
        }
    }
}

