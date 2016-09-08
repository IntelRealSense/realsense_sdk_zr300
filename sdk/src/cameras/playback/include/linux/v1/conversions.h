// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense/rs.hpp>
#include "include/linux/v1/file_types.h"
#include "status.h"
#include "rs/utils/log_utils.h"

namespace rs
{
    namespace playback
    {
        namespace linux
        {
            namespace v1
            {
                namespace conversions
                {

                    core::status convert(const file_types::file_header &source, core::file_types::file_header &target)
                    {
                        memset(&target, 0, sizeof(target));
                        target.id = source.id;
                        target.version = source.version;
                        target.coordinate_system = source.coordinate_system;
                        target.first_frame_offset = source.first_frame_offset;
                        target.nstreams = source.nstreams;
                        return core::status_no_error;
                    }

                    core::status convert(const file_types::device_info &source, core::file_types::device_info &target)
                    {
                        memset(&target, 0, sizeof(target));
                        auto nameSize = sizeof(target.name) / sizeof(target.name[0]);
                        for(size_t i = 0; i < nameSize; i++)
                            target.name[i] = source.name[i];
                        auto serial_size = sizeof(target.serial) / sizeof(target.serial[0]);
                        for(size_t i = 0; i < serial_size; i++)
                            target.serial[i] = source.serial[i];
                        auto fw_size = sizeof(target.serial) / sizeof(target.serial[0]);
                        for(size_t i = 0; i < fw_size; i++)
                            target.camera_firmware[i] = source.firmware[i];
                        return core::status_no_error;
                    }

                    core::status convert(const file_types::sw_info &source, core::file_types::sw_info &target)
                    {
                        memset(&target, 0, sizeof(target));
                        target.librealsense = source.librealsense;
                        target.sdk = source.sdk;
                        return core::status_no_error;
                    }

                    core::status convert(const file_types::sample_info &source, core::file_types::sample_info &target)
                    {
                        memset(&target, 0, sizeof(target));
                        target.capture_time = source.capture_time;
                        target.offset = source.offset;
                        target.type = source.type;
                        return core::status_no_error;
                    }

                    core::status convert(const file_types::frame_info &source, core::file_types::frame_info &target)
                    {
                        memset(&target, 0, sizeof(target));
                        target.width = source.width;
                        target.height = source.height;
                        auto bpp_v1_is_bytes_per_pixel = source.bpp;
                        target.bpp = bpp_v1_is_bytes_per_pixel * 8;
                        auto stride_x_v1_is_pixels_per_raw = source.stride_x;
                        target.stride = stride_x_v1_is_pixels_per_raw * bpp_v1_is_bytes_per_pixel;
                        target.format = source.format;
                        target.framerate = source.framerate;
                        target.index_in_stream = source.index_in_stream;
                        target.number = source.number;
                        target.stream = source.stream;
                        target.system_time = source.system_time;
                        target.time_stamp = source.time_stamp;
                        target.time_stamp_domain = RS_TIMESTAMP_DOMAIN_CAMERA;
                        return core::status_no_error;
                    }

                    core::status convert(const file_types::stream_profile &source, core::file_types::stream_profile &target)
                    {
                        memset(&target, 0, sizeof(target));
                        core::file_types::frame_info frame_info;
                        if(convert(source.info, frame_info) != core::status_no_error)
                            return core::status_item_unavailable;
                        target.frame_rate = source.frame_rate;
                        target.info = frame_info;
                        target.depth_scale = source.depth_scale;
                        target.extrinsics = source.extrinsics;
                        target.intrinsics = source.intrinsics;
                        target.rect_intrinsics = source.rect_intrinsics;
                        return core::status_no_error;
                    }

                    core::status convert(const file_types::stream_info &source, core::file_types::stream_info &target)
                    {
                        memset(&target, 0, sizeof(target));
                        core::file_types::stream_profile profile;
                        if(convert(source.profile, profile) != core::status_no_error)
                            return core::status_item_unavailable;
                        target.profile = profile;
                        target.stream = source.stream;
                        target.nframes = source.nframes;
                        target.ctype = source.ctype;
                        return core::status_no_error;
                    }
                }
            }
        }
    }
}
