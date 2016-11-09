// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense/rs.hpp>

namespace rs
{
    namespace record
    {
        /**
        This class provides rs::device capablities with extentions of record capablities.
        */
        class device : public rs::device
        {
        public:
            /**
            @brief Pause recording.
            */
            void pause_record();
            /**
            @brief Resume recording.
            */
            void resume_record();
            /**
            @brief Set the selected stream compression behavior. Possible only before record starts. Compression level range: 0-100. Enabled by default
            For all streams higher compression level will produce smaller file size.
            For color stream, higher compression level will reduce image quality with no significant CPU utilization difference.
            For depth/infrared/fisheye streams, higher compression level will cause significant increment  of CPU utilization while maintain lossless compression..
            @param[in] stream  The relevant stream.
            @param[in] enable  Enable / disable compression for the requsted stream.
            @param[in] compression_level  The requsted compression level.
            */
            void set_compression(rs::stream stream, bool enable, float compression_level = 0);
        };
    }
}
