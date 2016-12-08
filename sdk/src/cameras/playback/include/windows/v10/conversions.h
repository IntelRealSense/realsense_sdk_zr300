// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <map>
#include <librealsense/rs.hpp>
#include "windows/v10/file_types.h"
#include "include/file_types.h"
#include "include/windows/projection_types.h"
#include "status.h"
#include "types.h"

namespace rs
{
    namespace playback
    {
        namespace windows
        {
            namespace v10
            {
                namespace conversions
                {
                    uint64_t rssdk2lrs_timestamp(uint64_t time);
                    core::status convert(file_types::stream_type source, rs_stream &target);
                    core::status convert(file_types::compression_type source, core::file_types::compression_type &target);
                    core::status convert(file_types::rotation source, rs::core::rotation &target);
                    core::status convert(file_types::pixel_format source, rs_format &target);
                    core::status convert(const file_types::coordinate_system &source, core::file_types::coordinate_system &target);
                    core::status convert(const file_types::disk_format::header &source, core::file_types::file_header &target);
                    core::status convert(const file_types::disk_format::stream_info &source, core::file_types::stream_info &target);
                    core::status convert(const file_types::disk_format::device_info_disk &source, std::map<rs_camera_info, std::string>& target);
                    core::status convert(const file_types::image_info &source, std::map<rs_camera_info, std::string>& target);
                    core::status convert(const file_types::disk_format::stream_profile_disk &source, core::file_types::stream_profile &target);
                    core::status convert(const file_types::disk_format::stream_profile_set_disk &source, std::map<rs_stream, core::file_types::stream_info> &target);
                    core::status convert(const file_types::disk_format::frame_metadata &source, core::file_types::frame_info &target);
                    rs_intrinsics get_intrinsics(file_types::stream_type stream, ds_projection::ProjectionData* projection);
                    rs_extrinsics get_extrinsics(file_types::stream_type stream, ds_projection::ProjectionData *projection);
                }
            }
        }
    }
}
