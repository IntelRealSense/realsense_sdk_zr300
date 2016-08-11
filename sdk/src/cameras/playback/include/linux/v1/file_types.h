// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense/rs.hpp>
#include "include/file_types.h"

namespace rs
{
    namespace playback
    {
        namespace linux
        {
            namespace v1
            {
                namespace file_types
                {
                    struct chunk_info
                    {
                        core::file_types::chunk_id  id;
                        int32_t                     size;
                    };

                    struct device_info
                    {
                        char name[224];    // device name
                        char serial[32];   // serial number
                        char firmware[32]; // firmware version
                        char usb_port_id[256]; // firmware version
                    };

                    struct sw_info
                    {
                        core::file_types::version    sdk;
                        core::file_types::version    librealsense;
                    };

                    struct sample_info
                    {
                        core::file_types::sample_type   type;
                        uint64_t                        capture_time;
                        uint64_t                        offset;
                    };

                    struct frame_info
                    {
                        int                 width;
                        int                 height;
                        rs_format           format;
                        int                 stride_x;
                        int                 stride_y;
                        float               bpp;
                        rs_stream           stream;
                        int                 number;
                        double              time_stamp;
                        long long           system_time;
                        int                 framerate;
                        uint32_t            index_in_stream;
                    };

                    struct stream_profile
                    {
                        frame_info      info;
                        int32_t         frame_rate;
                        rs_intrinsics   intrinsics;
                        rs_intrinsics   rect_intrinsics;
                        rs_extrinsics   extrinsics;
                        float           depth_scale;
                    };

                    struct stream_info
                    {
                        rs_stream                           stream;
                        core::file_types::compression_type  ctype;
                        int32_t                             nframes;
                        stream_profile                      profile;
                    };

                    struct file_header
                    {
                        int32_t                                 id;                     // File identifier
                        int32_t                                 version;                // file version for windows files
                        int32_t                                 first_frame_offset;     // The byte offset to the meta data of the first frame.
                        int32_t                                 nstreams;               // Number of streams
                        core::file_types::coordinate_system     coordinate_system;
                    };

                    class disk_format
                    {
                    public:
                        struct device_info
                        {
                            file_types::device_info     data;
                            int32_t                     reserved[25];
                        };

                        struct sw_info
                        {
                            file_types::sw_info data;
                            int32_t             reserved[10];
                        };

                        struct stream_info
                        {
                            file_types::stream_info data;
                            int32_t                 reserved[10];

                        };

                        struct sample_info
                        {
                            file_types::sample_info data;
                            int32_t                 reserved[10];
                        };

                        struct frame_info
                        {
                            file_types::frame_info  data;
                            int32_t                 reserved[10];
                        };

                        struct time_stamp_data
                        {
                            rs_timestamp_data   data;
                            int32_t             reserved[10];
                        };

                        struct motion_data
                        {
                            rs_motion_data data;
                            int32_t        reserved[10];
                        };

                        struct file_header
                        {
                            file_types::file_header data;
                            int32_t                 reserved[25];
                        };
                    };
                }
            }
        }
    }
}

