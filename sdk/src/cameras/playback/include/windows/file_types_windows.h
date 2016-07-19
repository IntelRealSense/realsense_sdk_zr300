// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once

#define PXC_DEFINE_CONST(Y,X) enum {Y=X}
#define NUM_OF_PLANES 4

namespace rs
{
    namespace PXC
    {
        static const int  STREAMS_LIMIT = 8;
        class File;

        /**
        @enum Property
        Describes the device properties.
        Use the inline functions to access specific device properties.
        */
        enum Property
        {
            /* Color Stream Properties */
            PROPERTY_COLOR_EXPOSURE = 1,                /* int32_t       RW    The color stream exposure, in log base 2 seconds. */
            PROPERTY_COLOR_BRIGHTNESS = 2,              /* int32_t       RW    The color stream brightness from  -10,000 (pure black) to 10,000 (pure white). */
            PROPERTY_COLOR_CONTRAST = 3,                /* int32_t       RW    The color stream contrast, from 0 to 10,000. */
            PROPERTY_COLOR_SATURATION = 4,              /* int32_t       RW    The color stream saturation, from 0 to 10,000. */
            PROPERTY_COLOR_HUE = 5,                     /* int32_t       RW    The color stream hue, from -180,000 to 180,000 (representing -180 to 180 degrees.) */
            PROPERTY_COLOR_GAMMA = 6,                   /* int32_t       RW    The color stream gamma, from 1 to 500. */
            PROPERTY_COLOR_WHITE_BALANCE = 7,           /* int32_t       RW    The color stream balance, as a color temperature in degrees Kelvin. */
            PROPERTY_COLOR_SHARPNESS = 8,               /* int32_t       RW    The color stream sharpness, from 0 to 100. */
            PROPERTY_COLOR_BACK_LIGHT_COMPENSATION = 9, /* bool32_t      RW    The color stream back light compensation. */
            PROPERTY_COLOR_GAIN = 10,                   /* int32_t       RW    The color stream gain adjustment, with negative values darker, positive values brighter, and zero as normal. */
            PROPERTY_COLOR_POWER_LINE_FREQUENCY = 11,   /* int32_t       RW    The power line frequency in Hz. */
            PROPERTY_COLOR_FOCAL_LENGTH_MM = 12,        /* float          R    The color-sensor focal length in mm. */
            PROPERTY_COLOR_FIELD_OF_VIEW = 1000,        /* PXCPointF32    R    The color-sensor horizontal and vertical field of view parameters, in degrees. */
            PROPERTY_COLOR_FOCAL_LENGTH = 1006,         /* PXCPointF32    R    The color-sensor focal length in pixels. The parameters vary with the resolution setting. */
            PROPERTY_COLOR_PRINCIPAL_POINT = 1008,      /* PXCPointF32    R    The color-sensor principal point in pixels. The parameters vary with the resolution setting. */

            // Depth Stream Properties
            PROPERTY_DEPTH_LOW_CONFIDENCE_VALUE = 201,  /* pxcU16         R    The special depth map value to indicate that the corresponding depth map pixel is of low-confidence. */
            PROPERTY_DEPTH_CONFIDENCE_THRESHOLD = 202,  /* pxcI16        RW    The confidence threshold that is used to floor the depth map values. The range is from  0 to 15. */
            PROPERTY_DEPTH_UNIT = 204,                  /* int32_t        R    The unit of depth values in micrometer if PIXEL_FORMAT_DEPTH_RAW */
            PROPERTY_DEPTH_FOCAL_LENGTH_MM = 205,       /* float          R    The depth-sensor focal length in mm. */
            PROPERTY_DEPTH_FIELD_OF_VIEW = 2000,        /* PXCPointF32    R    The depth-sensor horizontal and vertical field of view parameters, in degrees. */
            PROPERTY_DEPTH_SENSOR_RANGE = 2002,         /* PXCRangeF32    R    The depth-sensor, sensing distance parameters, in millimeters. */
            PROPERTY_DEPTH_FOCAL_LENGTH = 2006,         /* PXCPointF32    R    The depth-sensor focal length in pixels. The parameters vary with the resolution setting. */
            PROPERTY_DEPTH_PRINCIPAL_POINT = 2008,      /* PXCPointF32    R    The depth-sensor principal point in pixels. The parameters vary with the resolution setting. */

            // Device Properties
            PROPERTY_DEVICE_ALLOW_PROFILE_CHANGE = 302, /* bool32_t      RW    If true, allow resolution change and throw PXC_STATUS_STREAM_CONFIG_CHANGED */
            PROPERTY_DEVICE_MIRROR = 304,               /* MirrorMode    RW    The mirroring options. */

            // Misc. Properties
            PROPERTY_PROJECTION_SERIALIZABLE = 3003,    /* pxcU32         R    The meta data identifier of the Projection instance serialization data. */

            // Device Specific Properties - IVCam
            PROPERTY_IVCAM_LASER_POWER = 0x10000,       /* int32_t        RW    The laser power value from 0 (minimum) to 16 (maximum). */
            PROPERTY_IVCAM_ACCURACY = 0x10001,          /* IVCAMAccuracy  RW    The IVCAM accuracy value. */
            PROPERTY_IVCAM_FILTER_OPTION = 0x10003,     /* int32_t        RW    The filter option (smoothing aggressiveness) ranged from 0 (close range) to 7 (far range). */
            PROPERTY_IVCAM_MOTION_RANGE_TRADE_OFF = 0x10004, /* int32_t   RW    This option specifies the motion and range trade off. The value ranged from 0 (short exposure, range, and better motion) to 100 (long exposure, range). */

            // Device Specific Properties - DS
            PROPERTY_DS_CROP = 0x20000,                 /* bool32_t       RW    Indicates whether to crop left and right images to match size of z image*/
            PROPERTY_DS_EMITTER = 0x20001,              /* bool32_t       RW    Enable or disable DS emitter*/
            PROPERTY_DS_DISPARITY_OUTPUT = 0x20003,     /* bool32_t       RW    Switches the range output mode between distance (Z) and disparity (inverse distance)*/
            PROPERTY_DS_DISPARITY_MULTIPLIER = 0x20004, /* int32_t        RW    Sets the disparity scale factor used when in disparity output mode. Default value is 32.*/
            PROPERTY_DS_DISPARITY_SHIFT = 0x20005,      /* int32_t        RW    Reduces both the minimum and maximum depth that can be computed.
                                                                                Allows range to be computed for points in the near field which would otherwise be beyond the disparity search range.*/
            PROPERTY_DS_MIN_MAX_Z = 0x20006,            /* PXCRangeF32    RW    The minimum z and maximum z in Z units that will be output   */
            PROPERTY_DS_COLOR_RECTIFICATION = 0x20008,  /* bool32_t        R     if true rectification is enabled to DS color*/
            PROPERTY_DS_DEPTH_RECTIFICATION = 0x20009,  /* bool32_t        R     if true rectification is enabled to DS depth*/
            PROPERTY_DS_LEFTRIGHT_EXPOSURE = 0x2000A,   /* float          RW    The depth stream exposure, in log base 2 seconds. */
            PROPERTY_DS_LEFTRIGHT_GAIN = 0x2000B,       /* int32_t        RW    The depth stream gain adjustment, with negative values darker, positive values brighter, and zero as normal. */
            PROPERTY_DS_Z_TO_DISPARITY_CONSTANT = 0x2000C,      /* float   R     used to convert between Z distance (in mm) and disparity (in pixels)*/
            PROPERTY_DS_ROBINS_MUNROE_MINUS_INCREMENT = 0x2000D, /* float  RW    Sets the value to subtract when estimating the median of the correlation surface.*/
            PROPERTY_DS_ROBINS_MUNROE_PLUS_INCREMENT = 0x2000E, /* float   RW    Sets the value to add when estimating the median of the correlation surface. */
            PROPERTY_DS_MEDIAN_THRESHOLD = 0x2000F,     /* float           RW    Sets the threshold for how much the winning score must beat the median to be judged a reliable depth measurement. */
            PROPERTY_DS_SCORE_MIN_THRESHOLD = 0x20010,  /* float           RW    Sets the minimum correlation score that is considered acceptable. */
            PROPERTY_DS_SCORE_MAX_THRESHOLD = 0x20011,  /* float           RW    Sets the maximum correlation score that is considered acceptable. */
            PROPERTY_DS_TEXTURE_COUNT_THRESHOLD = 0x20012, /* float        RW    Set parameter for determining how much texture in the region is sufficient to be judged a reliable depth measurement. */
            PROPERTY_DS_TEXTURE_DIFFERENCE_THRESHOLD = 0x20013, /* float   RW    Set parameter for determining whether the texture in the region is sufficient to justify a reliable depth measurement. */
            PROPERTY_DS_SECOND_PEAK_THRESHOLD = 0x20014, /* float          RW    Sets the threshold for how much the minimum correlation score must differ from the next best score to be judged a reliable depth measurement. */
            PROPERTY_DS_NEIGHBOR_THRESHOLD = 0x20015,   /* float           RW    Sets the threshold for how much at least one adjacent disparity score must differ from the minimum score to be judged a reliable depth measurement. */
            PROPERTY_DS_LR_THRESHOLD = 0x20016,         /* float           RW    Determines the current threshold for determining whether the left-right match agrees with the right-left match. */

            PROPERTY_SR300_COLOR_EXPOSURE_PRIORITY = 0x30000, /* float        RW    Sets the Color Exposure Priority. */
            PROPERTY_SR300_HDR_MODE = 0x30001, /* float        RW    Sets the HDR mode (0 = DISABLED). */

            // Customized properties
            PROPERTY_CUSTOMIZED = 0x04000000,                        /* CUSTOMIZED properties */
        };

        /**
        @structure DeviceCap
        Describes a pair value of device property and its value.
        Use the inline functions to access specific device properties.
        */
        struct DeviceCap
        {
            Property   label;       /* Property type */
            float      value;       /* Property value */
        };

        /**
        @enum StreamOption
        Describes the steam options.
        */
        enum StreamOption
        {
            STREAM_OPTION_ANY = 0,

            // Optional options
            STREAM_OPTION_OPTIONAL_MASK = 0x0000FFFF,       /* The option can be added to any profile, but not necessarily supported for any profile */
            STREAM_OPTION_DEPTH_PRECALCULATE_UVMAP = 0x00000001,   /* A flag to ask the device to precalculate UVMap */
            STREAM_OPTION_STRONG_STREAM_SYNC = 0x00000002,  /* A flag to ask the device to perform strong (HW-based) synchronization on the streams with this flag. */

            // Mandatory options
            STREAM_OPTION_MANDATORY_MASK = 0xFFFF0000,      /* If the option is supported - the device sets this flag in the profile */
            STREAM_OPTION_UNRECTIFIED = 0x00010000,         /* A mandatory flag to ask the device to stream unrectified images on the stream with this flag */
            STREAM_OPTION_DEPTH_CONFIDENCE = 0x00020000     /* A mandatory flag to ask the device to attach confidence data to depth images (see PIXEL_FORMAT_DEPTH_CONFIDENCE) */
        };

        /**
        @enum PixelFormat
        Describes the image sample pixel format
        */
        enum PixelFormat
        {
            PIXEL_FORMAT_ANY = 0,                    /* Unknown/undefined */

            // STREAM_TYPE_COLOR
            PIXEL_FORMAT_YUY2 = 0x00010000,         /* YUY2 image  */
            PIXEL_FORMAT_NV12,                      /* NV12 image */
            PIXEL_FORMAT_RGB32,                     /* BGRA layout on a little-endian machine */
            PIXEL_FORMAT_RGB24,                     /* BGR layout on a little-endian machine */
            PIXEL_FORMAT_Y8,                        /* 8-Bit Gray Image, or IR 8-bit */

            // STREAM_TYPE_DEPTH
            PIXEL_FORMAT_DEPTH = 0x00020000,        /* 16-bit unsigned integer with precision mm. */
            PIXEL_FORMAT_DEPTH_RAW,                 /* 16-bit unsigned integer with device specific precision (call device->QueryDepthUnit()) */
            PIXEL_FORMAT_DEPTH_F32,                 /* 32-bit float-point with precision mm. */
            PIXEL_FORMAT_DEPTH_CONFIDENCE = 0x40000004, /* Additional plane with 8-bits depth confidence (MSB) */

            // STREAM_TYPE_IR
            PIXEL_FORMAT_Y16 = 0x00040000,          /* 16-Bit Gray Image */
            PIXEL_FORMAT_Y8_IR_RELATIVE = 0x00080000    /* Relative IR Image */
        };

        /**
        @enum DeviceModel
        Describes the device model
        */
        enum DeviceModel
        {
            DEVICE_MODEL_GENERIC = 0x00000000,    /* a generic device or unknown device */
            DEVICE_MODEL_F200 = 0x0020000E,    /* the Intel(R) RealSense(TM) 3D Camera, model F200 */
            DEVICE_MODEL_IVCAM = 0x0020000E,    /* deprecated: the Intel(R) RealSense(TM) 3D Camera, model F200 */
            DEVICE_MODEL_R200 = 0x0020000F,    /* the Intel(R) RealSense(TM) 3D Camera, model R200 */
            DEVICE_MODEL_DS4 = 0x0020000F,    /* deprecated: the Intel(R) RealSense(TM) 3D Camera, model R200 */
            DEVICE_MODEL_SR300 = 0x00200010,    /* The Intel(R) RealSense(TM) 3D Camera, model SR300 */
            DEVICE_MODEL_R200_ENHANCED = 0x0020001F,   /* The Intel(R) RealSense(TM) 3D Camera, model R200 and Platform Camera */
        };

        /**
        @enum DeviceOrientation
        Describes the device orientation
        */
        enum DeviceOrientation
        {
            DEVICE_ORIENTATION_ANY = 0x0,  /* Unknown orientation */
            DEVICE_ORIENTATION_USER_FACING = 0x1,  /* A user facing camera */
            DEVICE_ORIENTATION_FRONT_FACING = 0x1,  /* A front facing camera */
            DEVICE_ORIENTATION_WORLD_FACING = 0x2,  /* A world facing camera */
            DEVICE_ORIENTATION_REAR_FACING = 0x2,  /* A rear facing camera */
        };


        /**
        @enum Rotation
        Image rotation options.
        */
        enum Rotation
        {
            ROTATION_0_DEGREE   = 0x0,   /* 0 Degree rotation */
            ROTATION_90_DEGREE  = 90,    /* 90 degree clockwise rotation */
            ROTATION_180_DEGREE = 180,   /* 180 degree clockwise rotation */
            ROTATION_270_DEGREE = 270,   /* 270 degree clockwise rotation */
        };

        /**
        @struct ImageInfo
        Describes the image sample detailed information.
        */
        struct ImageInfo
        {
            int32_t      width;              /* width of the image in pixels */
            int32_t      height;             /* height of the image in pixels */
            PixelFormat  format;             /* image pixel format */
            int32_t      reserved;
        };

        /**
        @enum Option
        Describes the image options.
        */
        enum ImageOption
        {
            OPTION_ANY = 0,
        };

        enum compression_type
        {
            COMPRESSION_NONE = 0,
            COMPRESSION_H264,
            COMPRESSION_LZO
        };

        /**
        @enum StreamType
        Bit-OR'ed values of stream types, physical or virtual streams.
        */
        enum StreamType
        {
            STREAM_TYPE_ANY = 0,          /* Unknown/undefined type */
            STREAM_TYPE_COLOR = 0x0001,     /* the color stream type  */
            STREAM_TYPE_DEPTH = 0x0002,     /* the depth stream type  */
            STREAM_TYPE_IR = 0x0004,     /* the infrared stream type */
            STREAM_TYPE_LEFT = 0x0008,     /* the stereoscopic left intensity image */
            STREAM_TYPE_RIGHT = 0x0010,     /* the stereoscopic right intensity image */
        };

        /**
        @enum ConnectionType
        Describes the Connection type of the device
        */
        enum ConnectionType
        {
            CONNECTION_TYPE_UNKNOWN = 0,                /* Any connection type */
            CONNECTION_TYPE_USB_INTEGRATED,              /* USB Integrated Camera */
            CONNECTION_TYPE_USB_PERIPHERAL                /* USB Peripheral Camera */
        };

        struct Version
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
        enum CoordinateSystem
        {
            COORDINATE_SYSTEM_REAR_DEFAULT      = 0x100,    /* Right-hand system: X right, Y up, Z to the user */
            COORDINATE_SYSTEM_REAR_OPENCV       = 0x200,    /* Right-hand system: X right, Y down, Z to the world */
            COORDINATE_SYSTEM_FRONT_DEFAULT     = 0x001,    /* Left-hand system: X left, Y up, Z to the user */
        };

        /**
            @enum ImplGroup
            The SDK group I/O and algorithm modules into groups and subgroups.
            This is the enumerator for algorithm groups.
        */
        enum ImplGroup
        {
            IMPL_GROUP_ANY                  =    0,             /* Undefine group */
            IMPL_GROUP_OBJECT_RECOGNITION   =    0x00000001,    /* Object recognition algorithms */
            IMPL_GROUP_SPEECH_RECOGNITION   =    0x00000002,    /* Speech recognition algorithms */
            IMPL_GROUP_SENSOR               =    0x00000004,    /* I/O modules */
            IMPL_GROUP_PHOTOGRAPHY          =    0x00000008,    /* Photography/Videography algorithms */
            IMPL_GROUP_UTILITIES            =    0x00000010,	/* Utility modules */
            IMPL_GROUP_CORE                 =    0x80000000,    /* Core SDK modules */
            IMPL_GROUP_USER                 =    0x40000000,    /* User defined algorithms */
        };

        /**
            @enum ImplSubgroup
            The SDK group I/O and algorithm modules into groups and subgroups.
            This is the enumerator for algorithm subgroups.
        */
        enum ImplSubgroup
        {
            IMPL_SUBGROUP_ANY                   = 0,            /* Undefined subgroup */

            /* object recognition building blocks */
            IMPL_SUBGROUP_FACE_ANALYSIS         = 0x00000001,    /* face analysis subgroup */
            IMPL_SUBGROUP_GESTURE_RECOGNITION   = 0x00000010,    /* gesture recognition subgroup */
            IMPL_SUBGROUP_SEGMENTATION          = 0x00000020,    /* segmentation subgroup */
            IMPL_SUBGROUP_PULSE_ESTIMATION      = 0x00000040,    /* pulse estimation subgroup */
            IMPL_SUBGROUP_EMOTION_RECOGNITION   = 0x00000080,    /* emotion recognition subgroup */
            IMPL_SUBGROUP_OBJECT_TRACKING       = 0x00000100,    /* object detection subgroup */
            IMPL_SUBGROUP_3DSEG                 = 0x00000200,    /* user segmentation subgroup */
            IMPL_SUBGROUP_3DSCAN                = 0x00000400,    /* mesh capture subgroup */
            IMPL_SUBGROUP_SCENE_PERCEPTION      = 0x00000800,    /* scene perception subgroup */

            /* Photography building blocks */
            IMPL_SUBGROUP_ENHANCED_PHOTOGRAPHY  = 0x00001000,    /* enhanced photography subgroup */
            IMPL_SUBGROUP_ENHANCED_VIDEOGRAPHY  = 0x00002000,    /* enhanced videography subgroup */

            /* sensor building blocks */
            IMPL_SUBGROUP_AUDIO_CAPTURE         = 0x00000001,    /* audio capture subgroup */
            IMPL_SUBGROUP_VIDEO_CAPTURE         = 0x00000002,    /* video capture subgroup */

            /* speech recognition building blocks */
            IMPL_SUBGROUP_SPEECH_RECOGNITION     = 0x00000001,    /* speech recognition subgroup */
            IMPL_SUBGROUP_SPEECH_SYNTHESIS       = 0x00000002,    /* speech synthesis subgroup */
        };
        enum
        {
            IMAGE_METADATA_POWER_STATE = 0x35467859,
            IMAGE_METADATA_SAMPLE_ID   = 0x9F228B51,
        };

        /**
        @brief Get the stream index number
        @param[in] StreamType    The stream type
        @return The stream index number.
        **/
        __inline static int32_t StreamTypeToIndex(StreamType type)
        {
            int32_t s = 0;
            while (type > 1) type = (StreamType)(type >> 1), s++;
            return s;
        }

        class CMDiskFormat
        {
        public:

            enum ChunkId
            {
                CHUNK_DEVICEINFO        = 1,
                CHUNK_STREAMINFO        = 2,
                CHUNK_PROPERTIES        = 3,
                CHUNK_PROFILES          = 4,
                CHUNK_SERIALIZEABLE     = 5,
                CHUNK_FRAME_META_DATA   = 6,
                CHUNK_FRAME_DATA        = 7,
                CHUNK_IMAGE_META_DATA   = 8,
                CHUNK_FRAME_INDEXING    = 9,
                CHUNK_SW_INFO           = 10
            };

            struct Header
            {
                PXC_DEFINE_CONST(FILE_IDENTIFIER, UID('R', 'S', 'L', 'X'));
                int32_t id; // File identifier
                int32_t version; // file version
                int32_t firstFrameOffset; // The byte offset to the meta data of the first frame.
                int32_t nstreams; // Number of streams
                int64_t frameIndexingOffset; // The byte offset to the frame indexing structure.
                CoordinateSystem coordinateSystem;
                int32_t reserved[25];
            };

            struct Chunk
            {
                ChunkId chunkId;
                int32_t  chunkSize;
            };

            struct FrameIndexing
            {
                int32_t nframes[STREAMS_LIMIT];
                struct frameInfo_st
                {
                    rs_stream type;    // stream type
                    int64_t offset;					// file offset in bytes
                    int64_t timeStamp;				// the time stamp in 100ns.
                    int32_t frameNumber;				// absolution frame number in the file
                } *frameList;                       // all frames in file
            };

            struct StreamInfo
            {
                StreamType stype;
                compression_type ctype;
                int32_t nframes;
            };

            struct FrameMetaData
            {
                int32_t frameNumber;
                StreamType streamType;
                int64_t timeStamp;
                ImageOption options;
                int32_t reserved[3];
            };

            struct DeviceInfoDisk
            {
                uint16_t           name[224];
                uint16_t           serial[32];
                uint16_t           did[256];
                int32_t             firmware[4];
                float               location[2];
                DeviceModel         model;
                DeviceOrientation   orientation;
                StreamType          streams;
                int32_t             didx;
                int32_t             duid;
                Rotation            rotation;
                ConnectionType	    connectionType;
                int32_t             reserved[11];
            };

            struct StreamProfileDisk
            {
                ImageInfo               imageInfo;        /* resolution and color format */
                float                   frameRate[2];        /* frame rate range. Set max when configuring FPS */
                StreamOption            options;          /* bit-mask of stream options */
                int32_t                 reserved[5];
            };
            /**
                @structure StreamProfileSet
                The set of StreamProfile that describes the configuration parameters of all streams.
            */
            struct StreamProfileSetDisk
            {
                StreamProfileDisk color;
                StreamProfileDisk depth;
                StreamProfileDisk ir;
                StreamProfileDisk left;
                StreamProfileDisk right;
                StreamProfileDisk reserved[STREAMS_LIMIT-5];

                /**
                    @brief Access the configuration parameters by the stream type.
                    @param[in] type        The stream type.
                    @return The StreamProfile instance.
                */
                __inline StreamProfileDisk &operator[](StreamType type)
                {
                    if (type==StreamType::STREAM_TYPE_COLOR) return color;
                    if (type==StreamType::STREAM_TYPE_DEPTH) return depth;
                    if (type==StreamType::STREAM_TYPE_IR)    return ir;
                    if (type==StreamType::STREAM_TYPE_LEFT)  return left;
                    if (type==StreamType::STREAM_TYPE_RIGHT) return right;
                    for (int i=sizeof(reserved)/sizeof(reserved[0])-1,j=(1<<(STREAMS_LIMIT-1)); i>=0; i--,j>>=1)
                        if (type&j) return reserved[i];
                    return reserved[sizeof(reserved)/sizeof(reserved[0])-1];
                }
            };
        };
    }
}

