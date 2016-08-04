// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "include/file_types.h"

#define PXC_DEFINE_CONST(Y,X) enum {Y=X}
#define NUM_OF_PLANES 4

namespace rs
{
    namespace win_file
    {
        static const int streams_limit = 8;
        class File;

        /**
        @enum Property
        Describes the device properties.
        Use the inline functions to access specific device properties.
        */
        enum property
        {
            // Misc. Properties
            property_projection_serializable = 3003,    /* pxcU32         R    The meta data identifier of the Projection instance serialization data. */
        };

        /**
        @structure DeviceCap
        Describes a pair value of device property and its value.
        Use the inline functions to access specific device properties.
        */
        struct device_cap
        {
            property   label;       /* Property type */
            float      value;       /* Property value */
        };

        /**
        @enum StreamOption
        Describes the steam options.
        */
        enum stream_option
        {
            so_any = 0,

            // Optional options
            so_optional_mask = 0x0000FFFF,       /* The option can be added to any profile, but not necessarily supported for any profile */
            so_depth_precalculate_uvmap = 0x00000001,   /* A flag to ask the device to precalculate UVMap */
            so_strong_stream_sync = 0x00000002,  /* A flag to ask the device to perform strong (HW-based) synchronization on the streams with this flag. */

            // Mandatory options
            so_mandatory_mask = 0xFFFF0000,      /* If the option is supported - the device sets this flag in the profile */
            so_unrectified = 0x00010000,         /* A mandatory flag to ask the device to stream unrectified images on the stream with this flag */
            so_depth_confidence = 0x00020000     /* A mandatory flag to ask the device to attach confidence data to depth images (see PIXEL_FORMAT_DEPTH_CONFIDENCE) */
        };

        /**
        @enum PixelFormat
        Describes the image sample pixel format
        */
        enum pixel_format
        {
            pf_any = 0,                    /* Unknown/undefined */

            // STREAM_TYPE_COLOR
            pf_yuy2 = 0x00010000,         /* YUY2 image  */
            pf_nv12,                      /* NV12 image */
            pf_rgb32,                     /* BGRA layout on a little-endian machine */
            pf_rgb24,                     /* BGR layout on a little-endian machine */
            pf_y8,                        /* 8-Bit Gray Image, or IR 8-bit */

            // STREAM_TYPE_DEPTH
            pf_depth = 0x00020000,        /* 16-bit unsigned integer with precision mm. */
            pf_raw,                 /* 16-bit unsigned integer with device specific precision (call device->QueryDepthUnit()) */
            pf_depth_f32,                 /* 32-bit float-point with precision mm. */
            pf_depth_confidence = 0x40000004, /* Additional plane with 8-bits depth confidence (MSB) */

            // STREAM_TYPE_IR
            pf_y16 = 0x00040000,          /* 16-Bit Gray Image */
            pf_y8_ur_relative = 0x00080000    /* Relative IR Image */
        };

        /**
        @enum DeviceModel
        Describes the device model
        */
        enum device_model
        {
            dm_generic        = 0x00000000,    /* a generic device or unknown device */
            dm_f200           = 0x0020000E,    /* the Intel(R) RealSense(TM) 3D Camera, model F200 */
            dm_ivcam          = 0x0020000E,    /* deprecated: the Intel(R) RealSense(TM) 3D Camera, model F200 */
            dm_r200           = 0x0020000F,    /* the Intel(R) RealSense(TM) 3D Camera, model R200 */
            dm_ds4            = 0x0020000F,    /* deprecated: the Intel(R) RealSense(TM) 3D Camera, model R200 */
            dm_sr300          = 0x00200010,    /* The Intel(R) RealSense(TM) 3D Camera, model SR300 */
            dm_r200_enhanced  = 0x0020001F,   /* The Intel(R) RealSense(TM) 3D Camera, model R200 and Platform Camera */
        };

        /**
        @enum DeviceOrientation
        Describes the device orientation
        */
        enum device_orientation
        {
            do_any = 0x0,  /* Unknown orientation */
            do_user_facing = 0x1,  /* A user facing camera */
            do_front_facing = 0x1,  /* A front facing camera */
            do_world_facing = 0x2,  /* A world facing camera */
            do_rear_facing = 0x2,  /* A rear facing camera */
        };


        /**
        @enum Rotation
        Image rotation options.
        */
        enum rotation
        {
            rotation_0_degree   = 0x0,   /* 0 Degree rotation */
            rotation_90_degree  = 90,    /* 90 degree clockwise rotation */
            rotation_180_degree = 180,   /* 180 degree clockwise rotation */
            rotation_270_degree = 270,   /* 270 degree clockwise rotation */
        };

        /**
        @struct ImageInfo
        Describes the image sample detailed information.
        */
        struct image_info
        {
            int32_t      width;              /* width of the image in pixels */
            int32_t      height;             /* height of the image in pixels */
            pixel_format format;             /* image pixel format */
            int32_t      reserved;
        };

        /**
        @enum Option
        Describes the image options.
        */
        enum image_option
        {
            OPTION_ANY = 0,
        };

        enum compression_type
        {
            compression_none = 0,
            compression_h264,
            compression_lzo
        };

        /**
        @enum StreamType
        Bit-OR'ed values of stream types, physical or virtual streams.
        */
        enum stream_type
        {
            stream_type_any = 0,          /* Unknown/undefined type */
            stream_type_color = 0x0001,     /* the color stream type  */
            stream_type_depth = 0x0002,     /* the depth stream type  */
            stream_type_ir = 0x0004,     /* the infrared stream type */
            stream_type_left = 0x0008,     /* the stereoscopic left intensity image */
            stream_type_right = 0x0010,     /* the stereoscopic right intensity image */
        };

        /**
        @enum ConnectionType
        Describes the Connection type of the device
        */
        enum connection_type
        {
            connection_type_unknown = 0,                /* Any connection type */
            connection_type_usb_integrated,              /* USB Integrated Camera */
            connection_type_usb_peripheral                /* USB Peripheral Camera */
        };

        struct version
        {
            uint32_t major;
            uint32_t minor;
            uint32_t build;
            uint32_t revision;
        };

        /**
            @enum CoordinateSystem
            SDK supports several 3D coordinate systems for front and rear facing cameras.
        */
        enum coordinate_system
        {
            coordinate_system_rear_default      = 0x100,    /* Right-hand system: X right, Y up, Z to the user */
            coordinate_system_rear_opencv       = 0x200,    /* Right-hand system: X right, Y down, Z to the world */
            coordinate_system_front_default     = 0x001,    /* Left-hand system: X left, Y up, Z to the user */
        };

        /**
            @enum ImplGroup
            The SDK group I/O and algorithm modules into groups and subgroups.
            This is the enumerator for algorithm groups.
        */
        enum impl_group
        {
            impl_group_any                  =    0,             /* Undefine group */
            impl_group_object_recognition   =    0x00000001,    /* Object recognition algorithms */
            impl_group_speech_recognition   =    0x00000002,    /* Speech recognition algorithms */
            impl_group_sensor               =    0x00000004,    /* I/O modules */
            impl_group_photography          =    0x00000008,    /* Photography/Videography algorithms */
            impl_group_utilities            =    0x00000010,	/* Utility modules */
            impl_group_core                 =    0x80000000,    /* Core SDK modules */
            impl_group_user                 =    0x40000000,    /* User defined algorithms */
        };

        /**
            @enum ImplSubgroup
            The SDK group I/O and algorithm modules into groups and subgroups.
            This is the enumerator for algorithm subgroups.
        */
        enum impl_subgroup
        {
            impl_subgroup_any                   = 0,            /* Undefined subgroup */

            /* object recognition building blocks */
            impl_subgroup_face_analysis         = 0x00000001,    /* face analysis subgroup */
            impl_subgroup_gesture_recognition   = 0x00000010,    /* gesture recognition subgroup */
            impl_subgroup_segmentation          = 0x00000020,    /* segmentation subgroup */
            impl_subgroup_pulse_estimation      = 0x00000040,    /* pulse estimation subgroup */
            impl_subgroup_emotion_recognition   = 0x00000080,    /* emotion recognition subgroup */
            impl_subgroup_object_tracking       = 0x00000100,    /* object detection subgroup */
            impl_subgroup_3dseg                 = 0x00000200,    /* user segmentation subgroup */
            impl_subgroup_3dscan                = 0x00000400,    /* mesh capture subgroup */
            impl_subgroup_scene_perception      = 0x00000800,    /* scene perception subgroup */

            /* Photography building blocks */
            impl_subgroup_enhanced_photography  = 0x00001000,    /* enhanced photography subgroup */
            impl_subgroup_enhanced_videography  = 0x00002000,    /* enhanced videography subgroup */

            /* sensor building blocks */
            impl_subgroup_audio_capture         = 0x00000001,    /* audio capture subgroup */
            impl_subgroup_video_capture         = 0x00000002,    /* video capture subgroup */

            /* speech recognition building blocks */
            impl_subgroup_speech_recognition     = 0x00000001,    /* speech recognition subgroup */
            impl_subgroup_speech_synthesis       = 0x00000002,    /* speech synthesis subgroup */
        };
        enum
        {
            image_metadata_power_state = 0x35467859,
            image_metadata_sample_id   = 0x9F228B51,
        };

        /**
        @brief Get the stream index number
        @param[in] StreamType    The stream type
        @return The stream index number.
        **/
        __inline static int32_t stream_type_to_index(stream_type type)
        {
            int32_t s = 0;
            while (type > 1) type = (stream_type)(type >> 1), s++;
            return s;
        }

        class disk_format
        {
        public:

            enum chunk_id
            {
                chunk_deviceinfo        = 1,
                chunk_streaminfo        = 2,
                chunk_properties        = 3,
                chunk_profiles          = 4,
                chunk_serializeable     = 5,
                chunk_frame_meta_data   = 6,
                chunk_frame_data        = 7,
                chunk_image_meta_data   = 8,
                chunk_frame_indexing    = 9,
                chunk_sw_info           = 10
            };

            struct header
            {
                PXC_DEFINE_CONST(FILE_IDENTIFIER, UID('R', 'S', 'L', 'X'));
                int32_t id; // File identifier
                int32_t version; // file version
                int32_t first_frame_offset; // The byte offset to the meta data of the first frame.
                int32_t nstreams; // Number of streams
                int64_t frame_indexing_offset; // The byte offset to the frame indexing structure.
                win_file::coordinate_system coordinate_system;
                int32_t reserved[25];
            };

            struct chunk
            {
                disk_format::chunk_id chunk_id;
                int32_t  chunk_size;
            };

            struct frame_indexing
            {
                int32_t nframes[streams_limit];
                struct frame_info_st
                {
                    rs_stream type;    // stream type
                    int64_t offset;					// file offset in bytes
                    int64_t time_stamp;				// the time stamp in 100ns.
                    int32_t frame_number;				// absolution frame number in the file
                } *frame_list;                       // all frames in file
            };

            struct stream_info
            {
                stream_type stype;
                compression_type ctype;
                int32_t nframes;
            };

            struct frame_metadata
            {
                int32_t                 frame_number;
                win_file::stream_type   stream_type;
                int64_t                 time_stamp;
                image_option            options;
                int32_t                 reserved[3];
            };

            struct device_info_disk
            {
                uint16_t             name[224];
                uint16_t             serial[32];
                uint16_t             did[256];
                int32_t              firmware[4];
                float                location[2];
                device_model         model;
                device_orientation   orientation;
                stream_type          streams;
                int32_t              didx;
                int32_t              duid;
                win_file::rotation   rotation;
                connection_type	     connectionType;
                int32_t              reserved[11];
            };

            struct stream_profile_disk
            {
                win_file::image_info    image_info;        /* resolution and color format */
                float                   frame_rate[2];        /* frame rate range. Set max when configuring FPS */
                stream_option           options;          /* bit-mask of stream options */
                int32_t                 reserved[5];
            };
            /**
                @structure StreamProfileSet
                The set of StreamProfile that describes the configuration parameters of all streams.
            */
            struct stream_profile_set_disk
            {
                stream_profile_disk color;
                stream_profile_disk depth;
                stream_profile_disk ir;
                stream_profile_disk left;
                stream_profile_disk right;
                stream_profile_disk reserved[streams_limit-5];

                /**
                    @brief Access the configuration parameters by the stream type.
                    @param[in] type        The stream type.
                    @return The StreamProfile instance.
                */
                __inline stream_profile_disk &operator[](stream_type type)
                {
                    if (type==stream_type::stream_type_color) return color;
                    if (type==stream_type::stream_type_depth) return depth;
                    if (type==stream_type::stream_type_ir)    return ir;
                    if (type==stream_type::stream_type_left)  return left;
                    if (type==stream_type::stream_type_right) return right;
                    for (int i=sizeof(reserved)/sizeof(reserved[0])-1,j=(1<<(streams_limit-1)); i>=0; i--,j>>=1)
                        if (type&j) return reserved[i];
                    return reserved[sizeof(reserved)/sizeof(reserved[0])-1];
                }
            };
        };
    }
}

