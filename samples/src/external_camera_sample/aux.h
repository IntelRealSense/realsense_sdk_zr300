#pragma once

#include <cstdint>
#include <linux/videodev2.h>
#include <librealsense/rs.hpp>

uint32_t num_bpp(rs::core::pixel_format format)
{
    switch (format)
    {
        case rs::core::pixel_format::any        : return 1;
        case rs::core::pixel_format::z16        : return 2;
        case rs::core::pixel_format::disparity16: return 2;
        case rs::core::pixel_format::xyz32f     : return 4;
        case rs::core::pixel_format::yuyv       : return 2;
        case rs::core::pixel_format::rgb8       : return 1;
        case rs::core::pixel_format::bgr8       : return 1;
        case rs::core::pixel_format::rgba8      : return 1;
        case rs::core::pixel_format::bgra8      : return 1;
        case rs::core::pixel_format::y8         : return 1;
        case rs::core::pixel_format::y16        : return 2;
        case rs::core::pixel_format::raw8       : return 1;
        case rs::core::pixel_format::raw10      : return 2;
        case rs::core::pixel_format::raw16      : return 2;
        default: return 1;
    }
}

rs::format convert_to_rs_format(uint32_t v4l_format)
{
    switch (v4l_format)
    {
        case V4L2_PIX_FMT_Z16                 : return rs::format::z16;
        case V4L2_PIX_FMT_YUYV                : return rs::format::yuyv;
        case V4L2_PIX_FMT_RGB24               : return rs::format::rgb8;
        case V4L2_PIX_FMT_BGR24               : return rs::format::bgr8;
        case V4L2_PIX_FMT_ARGB32              : return rs::format::rgba8;
        case V4L2_PIX_FMT_ABGR32              : return rs::format::bgra8;
        case V4L2_PIX_FMT_Y16                 : return rs::format::y16;
        case V4L2_PIX_FMT_Y10                 : return rs::format::raw10;
        default : return (rs::format) -1;
    }
}

const char* pixel_format_to_string(rs::format format)
{
    switch(format)
    {
        case rs::format::rgb8: return "rgb8";
        case rs::format::rgba8: return "rgba8";
        case rs::format::bgr8: return "bgr8";
        case rs::format::bgra8: return "bgra8";
        case rs::format::yuyv: return "yuyv";
        case rs::format::raw8: return "raw8";
        case rs::format::raw10: return "raw10";
        case rs::format::raw16: return "raw16";
        case rs::format::y8: return "y8";
        case rs::format::y16: return "y16";
        case rs::format::z16: return "z16";
        case rs::format::any: return "any";
        default: return "";
    }
}