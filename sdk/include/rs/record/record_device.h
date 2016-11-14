// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense/rs.hpp>
#include "rs/core/status.h"

namespace rs
{
    namespace record
    {
        /** @class rs::record::device
        *
        * The rs::record::device extends rs::device with set of record capablities.
        * Accessing a camera using the record device will record to file all the relevant information of the device along with the device configuration.
        * All the video-frames / motion-samples that are available to the application while streaming using the device are captured to the file, in case the appliction cause frame drop,
        * the droped frames will not be captured to the file.
        * Using rs::record::device togther with rs:playback device provides an easy way to test and debug SDK applications with
        * known input.
        * Creating the rs::record::device and defining the target file location is done using rs::record::context.
        */
        class device : public rs::device
        {
        public:
            /** @brief Pause recording.
            *
            * Stop record to file without modifying the streaming state of the device.
            */
            void pause_record();

            /** @brief Resume recording.
            *
            * Continue record to file without modifying the streaming state of the device. .
            */
            void resume_record();

            /** @brief Set the selected stream compression behavior.
            *
            * The function can be called only before record device start is called. The call is ignored while record device is in streaming state.
            * Setting the compression level adjusts the recorded file size - the higher the level, the smaller the file.
            * Compression level range: 0-100 percent.
            * Recorded file size control is achieved differently for different streams, based on the type of data:
            * -	Color stream images compression reduces the image quality. Different compression levels have no significant CPU utilization difference.
            * -	Depth/infrared/fisheye streams compression is lossless, thus there is no impact on image quality. However, higher compression level causes significant increment of CPU utilization.
            * The default behavior is enabled compression with compression level 0 for all streams but color stream (not implemented).
            * @param[in] stream  The stream for which the compression properties are requested.
            * @param[in] enable  Enable / disable compression for the requsted stream.
            * @param[in] compression_level  The requsted compression level.
            * @return core::status
            * status_no_error               Successful execution.
            * status_invalid_argument       Compression level value is out of legal range.
            */
            core::status set_compression(rs::stream stream, bool enable, float compression_level = 0);
        };
    }
}
