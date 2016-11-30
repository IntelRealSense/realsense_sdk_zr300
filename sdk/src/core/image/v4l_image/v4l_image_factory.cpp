// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <linux/videodev2.h>
#include "rs/core/v4l_image_factory.h"
#include "rs/core/image_interface.h"
#include "rs/core/types.h"

namespace rs
{
    namespace core
    {
        static rs::core::pixel_format convert_pixel_format(uint32_t video4linux_pixel_format)
        {
            switch (video4linux_pixel_format)
            {
                case V4L2_PIX_FMT_Z16                 : return rs::core::pixel_format::z16;
                case V4L2_PIX_FMT_YUYV                : return rs::core::pixel_format::yuyv;
                case V4L2_PIX_FMT_RGB24               : return rs::core::pixel_format::rgb8;
                case V4L2_PIX_FMT_BGR24               : return rs::core::pixel_format::bgr8;
                case V4L2_PIX_FMT_ARGB32              : return rs::core::pixel_format::rgba8;
                case V4L2_PIX_FMT_ABGR32              : return rs::core::pixel_format::bgra8;
                case V4L2_PIX_FMT_Y16                 : return rs::core::pixel_format::y16;
                case V4L2_PIX_FMT_Y10                 : return rs::core::pixel_format::raw10;
                default :  return static_cast<rs::core::pixel_format>(-1);
            }
        }

        image_interface* create_instance_from_v4l_buffer(const image_interface::image_data_with_data_releaser& data_container,
                                                                          v4l2_buffer v4l_buffer_info,
                                                                          stream_type stream,
                                                                          v4l2_pix_format v4l_image_info)
        {
            rs::core::image_info image_info = {
                static_cast<int32_t>(v4l_image_info.width),
                static_cast<int32_t>(v4l_image_info.height),
                convert_pixel_format(v4l_image_info.pixelformat),
                static_cast<int32_t>(v4l_image_info.bytesperline)
            };

            //Create an image from the raw buffer and its information
            return image_interface::create_instance_from_raw_data(
                    &image_info,
                    data_container,
                    stream,
                    rs::core::image_interface::flag::any,
                    static_cast<double>(v4l_buffer_info.timestamp.tv_usec),
                    v4l_buffer_info.sequence,
                    rs::core::timestamp_domain::camera);
         }
    }
}
