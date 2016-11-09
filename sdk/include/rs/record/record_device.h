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
            @brief Set the selected stream compression behavior.
            This function can be called only before record device start is called. The call is disregarded if set while record device is in streaming state.
            The default behavior is enabled compression with compression level 0 for all streams but color stream (not implemented).
            Setting the compression level adjusts the recorded file size - the higher the level, the smaller the file.  Compression level range: 0-100 percent.
            Recorded file size control is achieved differently for different streams, based on the type of data:
            -	Color stream images compression reduces the image quality. Different compression levels have no significant CPU utilization difference.
            -	Depth/infrared/fisheye streams compression is lossless, thus there is no impact on image quality. However, higher compression level causes significant increment of CPU utilization.
            @param[in] stream  The stream for which the compression properties are requested.
            @param[in] enable  Enable / disable compression for the requsted stream.
            @param[in] compression_level  The requsted compression level.
            @return bool  Indicates whether the compression level value is in legal range.
            */
            bool set_compression(rs::stream stream, bool enable, float compression_level = 0);
        };
    }
}
