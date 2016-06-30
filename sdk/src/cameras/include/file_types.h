// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense/rs.hpp>
#include "rs/core/image_interface.h"
#include "image/image_utils.h"

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
                chunk_sample_info     = 6,
                chunk_sample_data     = 7,
                chunk_image_metadata  = 8,
                chunk_frame_indexing  = 9,
                chunk_sw_info         = 10,
                chunk_sample_type     = 11,
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
                char                       name[224];    // device name
                char                       serial[32];   // serial number
                char                       firmware[32]; // firmware version
                char                       usb_port_id[256]; // firmware version
                rs::core::rotation         rotation;     // how the camera device is physically mounted
            };

            struct sw_info
            {
                version    sdk;
                version    librealsense;
            };

            struct sample
            {
                sample(sample_type type) : type(type) {}
                sample_type type;
                virtual ~sample() {}
            };

            struct time_stamp_info
            {
                rs_timestamp_data   data;
                uint64_t            offset;
                uint64_t            sync;
            };

            struct time_stamp : public sample
            {
                time_stamp(time_stamp_info info) : info(info), sample::sample(sample_type::st_time) {}
                time_stamp(rs_timestamp_data time_stamp_data, uint64_t sync) : sample::sample(sample_type::st_time)
                {
                    info.data = time_stamp_data;
                    info.sync = sync;
                }
                time_stamp_info info;
            };

            struct motion_info
            {
                rs_motion_data  data;
                uint64_t        offset;
                uint64_t        sync;
            };

            struct motion : public sample
            {
                motion(motion_info info) : info(info), sample::sample(sample_type::st_motion) {}
                motion(rs_motion_data motion_data, uint64_t sync) : sample::sample(sample_type::st_motion)
                {
                    info.data = motion_data;
                    info.sync = sync;
                }
                motion_info info;
            };

            struct frame_info
            {
                int                 width;
                int                 height;
                rs_format           format;
                int                 stride;
                int                 bpp;
                rs_stream           stream;
                int                 number;
                uint64_t            time_stamp;
                long long           system_time;
                int                 framerate;
                uint64_t            offset;
                uint64_t            sync;
                uint32_t            index_in_stream;
            };

            struct frame : public sample
            {
                frame(frame_info info) : info(info), sample::sample(sample_type::st_image) {}
                frame(rs_stream stream, rs_frame_ref *ref, uint64_t sync) : sample::sample(sample_type::st_image)
                {
                    memset(&info, 0, sizeof(frame_info));
                    info.width = ref->get_frame_width();
                    info.height = ref->get_frame_height();
                    info.stride = ref->get_frame_stride();
                    info.bpp = ref->get_frame_bpp();
                    info.format = ref->get_frame_format();
                    info.stream = stream;
                    info.number = ref->get_frame_number();
                    info.time_stamp = ref->get_frame_timestamp();
                    info.system_time = ref->get_frame_system_time();
                    info.framerate = ref->get_frame_framerate();
                    info.sync = sync;
                    data = ref->get_frame_data();
                }
                frame(rs_stream stream, const rs_stream_interface &si, uint64_t sync) : sample::sample(sample_type::st_image)
                {
                    memset(&info, 0, sizeof(frame_info));
                    info.width = si.get_intrinsics().width;
                    info.height = si.get_intrinsics().height;
                    info.stride = si.get_intrinsics().width;//rs_stream_interface is missing stride and bpp data
                    info.bpp = image_utils::get_pixel_size((rs::format)si.get_format());
                    info.format = si.get_format();
                    info.stream = stream;
                    info.number = si.get_frame_number();
                    info.time_stamp = si.get_frame_number();//librealsense bug - replace to time stamp when fixed
                    info.system_time = si.get_frame_system_time();
                    info.framerate = si.get_framerate();
                    info.sync = sync;
                    data = si.get_frame_data();
                }
                frame * copy()
                {
                    auto rv = new frame(info);
                    auto size = info.stride * info.bpp * info.height;
                    auto data_clone = new uint8_t[size];
                    memcpy(data_clone, data, size);
                    rv->data = data_clone;
                    return rv;
                }
                frame_info      info;
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
                int32_t                         version;                // file version
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
                    int32_t             reserved[5];
                };

                struct stream_info
                {
                    file_types::stream_info data;
                    int32_t                 reserved[5];

                };

                struct time_stamp_info
                {
                    file_types::time_stamp_info data;
                    int32_t                     reserved[5];
                };

                struct motion_info
                {
                    file_types::motion_info data;
                    int32_t                 reserved[5];
                };

                struct frame_info
                {
                    file_types::frame_info  data;
                    int32_t                 reserved[5];
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

