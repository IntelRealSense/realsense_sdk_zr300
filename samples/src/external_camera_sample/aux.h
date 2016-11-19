#pragma once

#include <cstdint>
#include <linux/videodev2.h>
#include <librealsense/rs.hpp>
#include <rs/core/types.h>

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