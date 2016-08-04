#pragma once
#include "rs/utils/librealsense_conversion_utils.h"
#include "image_utils.h"
namespace rs
{
    namespace core
    {
        namespace image_utils
        {
            static int8_t get_pixel_size(rs::format format)
            {
                return get_pixel_size(utils::convert_pixel_format(format));
            }
            static int8_t get_pixel_size(rs_format format)
            {
                return get_pixel_size(static_cast<rs::format>(format));
            }

        }
    }
}

