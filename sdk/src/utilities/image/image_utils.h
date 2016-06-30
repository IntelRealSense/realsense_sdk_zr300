#pragma once

#include <librealsense/rs.hpp>
#include "rs/core/types.h"
#include "rs/utils/librealsense_conversion_utils.h"

namespace rs
{
    namespace core
    {
        namespace image_utils
        {
            static int8_t get_pixel_size(rs::format format)
            {
                switch(format)
                {
                    case rs::format::any:return 0;
                    case rs::format::z16:return 2;
                    case rs::format::disparity16:return 2;
                    case rs::format::xyz32f:return 4;
                    case rs::format::yuyv:return 2;
                    case rs::format::rgb8:return 3;
                    case rs::format::bgr8:return 3;
                    case rs::format::rgba8:return 4;
                    case rs::format::bgra8:return 4;
                    case rs::format::y8:return 1;
                    case rs::format::y16:return 2;
                    case rs::format::raw8:return 1;
                    case rs::format::raw10:return 0;//not supported
                    case rs::format::raw16:return 2;
                }
                return 0;
            }

            static int8_t get_pixel_size(pixel_format format)
            {
                return get_pixel_size(utils::convert_pixel_format(format));
            }
        }
    }
}
