// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense/rs.hpp>
#include "rs/core/status.h"

namespace rs
{
    namespace record
    {
        enum compression_level
        {
            disabled  = 0,
            low       = 1,
            medium    = 2,
            high      = 3
        };

        /**
        * @class rs::record::device
        * @brief The rs::record::device extends rs::device to provide record capabilities. Commonly used for debug, testing and validation with known input.
        *
        * Accessing a camera using the record device captures the session into a file.
        * The record device writes to the file the device static information, current device and streams configuration, and streams data while streaming.
        * All the video-frames and motion-samples that the application receives while streaming, using the record device, are captured to the file.
        * In case the application causes frame drops, the dropped frames are not captured to the file.
        * Creating the rs::record::device and defining the target file location is done using rs::record::context.
        * The record device supports recording a single session of streaming. Hence, a single device and streams configuration is captured.
        */
        class device : public rs::device
        {
        public:
            /**
            * @brief Pause recording.
            *
            * Stop record to file without modifying the streaming state of the device.
            * The application still gets all camera streams data.
            * If the function is called before device start, recording doesn't start until resume_record is called.
            * The function can be called sequentially with resume_record multiple times.
            * If the function is called while the device state is already paused, the call is ignored.
            */
            void pause_record();

            /**
            * @brief Resume recording.
            *
            * Continue record to file without modifying the streaming state of the device.
            * The default recording state is on. This function should be called only after pause_record was called.
            * The function can be called sequentially with pause_record multiple times.
            * If the function is called while the device state is already recording, the call is ignored. .
            * Resume recording concatenates the captured streams data to the end of the same file.
            * The time gaps will appear to the application upon streaming from the file in playback.
            */
            void resume_record();

            /**
            * @brief Set the selected stream compression behavior.
            *
            * The function can be called only before record device start is called. The call is ignored while record device is in streaming state.
            * Setting the compression level adjusts the recorded file size - the higher the level, the smaller the file.
            * The default behavior is enabled compression with highest compression level for all streams.
            * Disable the compression is done by set compression_level to disabled
            * @param[in] stream  The stream for which the compression properties are requested.
            * @param[in] compression_level  The requsted compression level.
            * @return core::status
            * status_no_error               Successful execution.
            * status_invalid_argument       Compression level value is out of legal range.
            */
            core::status set_compression(rs::stream stream, compression_level compression_level);

            /** @brief Get the selected stream compression level.
            *
            * The function get the current compression level of the requested stream.
            * @param[in] stream  The stream for which the compression properties are requested.
            * @return compression_level The currnt compression level of the requested stream;
            */
            compression_level get_compression_level(rs::stream stream);
        };
    }
}
