// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <memory>
#include "rs/core/image_interface.h"
#include "rs/utils/librealsense_conversion_utils.h"
#include "image/image_utils.h"
#include "librealsense/rs.hpp"


namespace test_utils
{
    static std::shared_ptr<rs::core::image_interface> create_image(rs::device * device, rs::stream stream)
    {
        if(device->get_frame_data(stream) == nullptr)
            return nullptr;
        auto sdk_stream = rs::utils::convert_stream_type(stream);
        auto sdk_format = rs::utils::convert_pixel_format(device->get_stream_format(stream));
        const int pitch = device->get_stream_width(stream) * rs::core::image_utils::get_pixel_size(sdk_format);
        rs::core::image_info info = {device->get_stream_width(stream),
                                     device->get_stream_height(stream),
                                     sdk_format,
                                     pitch
                                    };
        return std::shared_ptr<rs::core::image_interface>(rs::core::image_interface::create_instance_from_raw_data(&info,
                                                                                      device->get_frame_data(stream),
                                                                                      sdk_stream,
                                                                                      rs::core::image_interface::flag::any,
                                                                                      device->get_frame_timestamp(stream),
                                                                                      device->get_frame_number(stream),
                                                                                      nullptr,
                                                                                      nullptr
                                                                                      ));
    }

}


