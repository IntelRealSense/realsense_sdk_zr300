// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <map>
#include <librealsense/rs.hpp>
#include "windows/file_types_windows.h"
#include "include/file_types.h"
#include "projection_types.h"
#include "status.h"
#include "image/librealsense_image_utils.h"
#include "rs/utils/log_utils.h"

namespace rs
{
    namespace win_file
    {
        namespace conversions
        {
            uint64_t rssdk2lrs_timestamp(uint64_t time);
            core::status convert(stream_type source, rs_stream &target);
            core::status convert(compression_type source, core::file_types::compression_type &target);
            core::status convert(rotation source, rs::core::rotation &target);
            core::status convert(pixel_format source, rs_format &target);
            core::status convert(const coordinate_system &source, core::file_types::coordinate_system &target);
            core::status convert(const disk_format::header &source, core::file_types::file_header &target);
            core::status convert(const disk_format::stream_info &source, core::file_types::stream_info &target);
            core::status convert(const disk_format::device_info_disk &source, core::file_types::device_info &target);
            core::status convert(const image_info &source, core::file_types::frame_info &target);
            core::status convert(const disk_format::stream_profile_disk &source, core::file_types::stream_profile &target);
            core::status convert(const disk_format::stream_profile_set_disk &source, std::map<rs_stream, core::file_types::stream_info> &target);
            core::status convert(const disk_format::frame_metadata &source, core::file_types::frame_info &target);
            rs_intrinsics get_intrinsics(stream_type stream, ds_projection::ProjectionData* projection);
            rs_extrinsics get_extrinsics(stream_type stream, ds_projection::ProjectionData *projection);
        }
    }
}
