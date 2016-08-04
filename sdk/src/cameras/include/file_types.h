// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense/rs.hpp>
#include "rs/core/image_interface.h"
#include "image/librealsense_image_utils.h"

/** This macro constructs a UID given four byte values.  The arguments will
be evaluated exactly once, cast to unsigned int and shifted into one of the
byte positions.  Hence, arguments must not hold values larger than a byte.
The result is a UID. */
#define UID(X1,X2,X3,X4) ((int32_t)(((unsigned int)(X4)<<24)+((unsigned int)(X3)<<16)+((unsigned int)(X2)<<8)+(unsigned int)(X1)))

namespace rs
{
    namespace core
    {
        namespace file_types
        {
            enum coordinate_system
            {
                rear_default      = 0,    /* Right-hand system: X right, Y up, Z to the user */
                rear_opencv       = 1,    /* Right-hand system: X right, Y down, Z to the world */
                front_default     = 2,    /* Left-hand system: X left, Y up, Z to the user */
            };

            enum compression_type
            {
                none = 0,
                h264 = 1,
                lzo = 2
            };

            enum sample_type
            {
                st_image,
                st_motion,
                st_time
            };

            enum chunk_id
            {
                chunk_device_info     = 1,
                chunk_stream_info     = 2,
                chunk_properties      = 3,
                chunk_profile         = 4,
                chunk_serializeable   = 5,
                chunk_frame_info      = 6,//frame stream type, frame width, frame height, frame format etc.
                chunk_sample_data     = 7,//rs_timestamp_data / rs_motion_data / image buffer
                chunk_image_metadata  = 8,
                chunk_frame_indexing  = 9,
                chunk_sw_info         = 10,
                chunk_sample_info     = 11,//sample type, capture time, offset
                chunk_capabilities    = 12
            };

            struct device_cap
            {
                rs_option   label;       /* option type */
                double      value;       /* option value */
            };

            struct version
            {
                uint32_t major;
                uint32_t minor;
                uint32_t build;
                uint32_t revision;
            };

            struct chunk_info
            {
                chunk_id id;
                int32_t  size;
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
                version    sdk;
                version    librealsense;
            };

            struct sample_info
            {
                sample_type type;
                uint64_t    capture_time;
                uint64_t    offset;
            };

            struct sample
            {
                sample(sample_info info) : info(info) {}
                sample(sample_type type, uint64_t capture_time, uint64_t offset)
                {
                    info.type = type;
                    info.capture_time = capture_time;
                    info.offset = offset;
                }
                sample_info info;
                virtual ~sample() {}
            };

            struct time_stamp_sample : public sample
            {
                time_stamp_sample(rs_timestamp_data time_stamp_data, uint64_t capture_time, uint64_t offset = 0) :
                    sample::sample(sample_type::st_time, capture_time, offset), data(time_stamp_data) {}
                time_stamp_sample(rs_timestamp_data time_stamp_data, sample_info info) : sample::sample(info), data(time_stamp_data) {}
                rs_timestamp_data   data;
            };

            struct motion_sample : public sample
            {
                motion_sample(rs_motion_data motion_data, uint64_t capture_time, uint64_t offset = 0) :
                    sample::sample(sample_type::st_motion, capture_time, offset), data(motion_data) {}
                motion_sample(rs_motion_data motion_data, sample_info info) : sample::sample(info), data(motion_data) {}
                rs_motion_data   data;
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

            struct frame_sample : public sample
            {
                frame_sample(const frame_sample * frame) : sample::sample(frame->info), finfo(frame->finfo) {}
                frame_sample(frame_info finfo, sample_info info) : sample::sample(info), finfo(finfo) {}
                frame_sample(frame_info finfo, uint64_t capture_time, uint64_t offset = 0) :
                    sample::sample(sample_type::st_image, capture_time, offset), finfo(finfo) {}
                frame_sample(rs_stream stream, rs_frame_ref *ref, uint64_t capture_time) :
                    sample::sample(sample_type::st_image, capture_time, 0)
                {
                    if (std::is_pod<frame_info>::value)
                        memset(&finfo, 0, sizeof(frame_info));
                    finfo.width = ref->get_frame_width();
                    finfo.height = ref->get_frame_height();
                    finfo.stride_x = ref->get_frame_stride_x();
                    finfo.stride_y = ref->get_frame_stride_y();
                    finfo.bpp = ref->get_frame_bpp();
                    finfo.format = ref->get_frame_format();
                    finfo.stream = stream;
                    finfo.number = ref->get_frame_number();
                    finfo.time_stamp = ref->get_frame_timestamp();
                    finfo.system_time = ref->get_frame_system_time();
                    finfo.framerate = ref->get_frame_framerate();
                    data = ref->get_frame_data();
                }
                frame_sample(rs_stream stream, const rs_stream_interface &si, uint64_t capture_time) :
                    sample::sample(sample_type::st_image, capture_time, 0)
                {
                    if (std::is_pod<frame_info>::value)
                        memset(&finfo, 0, sizeof(frame_info));
                    finfo.width = si.get_intrinsics().width;
                    finfo.height = si.get_intrinsics().height;
                    finfo.stride_x = si.get_intrinsics().width;//rs_stream_interface is missing stride and bpp data
                    finfo.stride_y = si.get_intrinsics().height;//rs_stream_interface is missing stride and bpp data
                    finfo.bpp = image_utils::get_pixel_size(si.get_format());
                    finfo.format = si.get_format();
                    finfo.stream = stream;
                    finfo.number = si.get_frame_number();
                    finfo.time_stamp = si.get_frame_timestamp();
                    finfo.system_time = si.get_frame_system_time();
                    finfo.framerate = si.get_framerate();
                    data = si.get_frame_data();
                }
                frame_sample * copy()
                {
                    auto rv = new frame_sample(this);
                    size_t size = finfo.stride_x * finfo.bpp * finfo.stride_y;
                    auto data_clone = new uint8_t[size];
                    memcpy(data_clone, data, size);
                    rv->data = data_clone;
                    return rv;
                }
                frame_info      finfo;
                const uint8_t * data;
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
                rs_stream           stream;
                compression_type    ctype;
                int32_t             nframes;
                stream_profile      profile;
            };

            struct file_header
            {
                int32_t                         id;                     // File identifier
                int32_t                         version;                // file version for windows files
                int32_t                         first_frame_offset;     // The byte offset to the meta data of the first frame.
                int32_t                         nstreams;               // Number of streams
                file_types::coordinate_system   coordinate_system;
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

