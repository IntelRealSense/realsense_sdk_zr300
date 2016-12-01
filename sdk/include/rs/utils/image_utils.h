// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once

#include "rs/core/types.h"

namespace rs
{
    namespace utils
    {
        static int8_t get_pixel_size(rs::core::pixel_format format)
        {
            switch(format)
            {
                case rs::core::pixel_format::any:return 0;
                case rs::core::pixel_format::z16:return 2;
                case rs::core::pixel_format::disparity16:return 2;
                case rs::core::pixel_format::xyz32f:return 4;
                case rs::core::pixel_format::yuyv:return 2;
                case rs::core::pixel_format::rgb8:return 3;
                case rs::core::pixel_format::bgr8:return 3;
                case rs::core::pixel_format::rgba8:return 4;
                case rs::core::pixel_format::bgra8:return 4;
                case rs::core::pixel_format::y8:return 1;
                case rs::core::pixel_format::y16:return 2;
                case rs::core::pixel_format::raw8:return 1;
                case rs::core::pixel_format::raw10:return 0;//not supported
                case rs::core::pixel_format::raw16:return 2;
                default: return 0;
            }
        }
    }
}
