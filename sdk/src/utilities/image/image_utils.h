#pragma once

#include "rs/core/types.h"

namespace rs
{
    namespace core
    {
        namespace image_utils
        {
            static int8_t get_pixel_size(pixel_format format)
            {
                switch(format)
                {
                    case pixel_format::any:return 0;
                    case pixel_format::z16:return 2;
                    case pixel_format::disparity16:return 2;
                    case pixel_format::xyz32f:return 4;
                    case pixel_format::yuyv:return 2;
                    case pixel_format::rgb8:return 3;
                    case pixel_format::bgr8:return 3;
                    case pixel_format::rgba8:return 4;
                    case pixel_format::bgra8:return 4;
                    case pixel_format::y8:return 1;
                    case pixel_format::y16:return 2;
                    case pixel_format::raw8:return 1;
                    case pixel_format::raw10:return 0;//not supported
                    case pixel_format::raw16:return 2;
                    default: return 0;
                }
            }
        }
    }
}
