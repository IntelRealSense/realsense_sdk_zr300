// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense/rs.hpp>

namespace rs
{
    namespace playback
    {
        enum capture_mode
        {
            synced  = 1, /* blocking read of all streams */
            asynced = 2  /* frames received from camera notifications */
        };

        enum file_format
        {
            rs_rssdk_format = 0, /* file structure is of RSSDK format */
            rs_linux_format = 1  /* file structure is of Linux SDK format */
        };

        /**
        * @struct rs::playback::file_info
        * @brief Describes the capture environment and configurations of a file.
        */
        struct file_info
        {
            int32_t                         version;                    /* Linux SDK file version */
            char                            sdk_version[32];            /* Linux SDK version that was used to capture the file */
            char                            librealsense_version[32];   /* librealsense version that was used to capture the file */
            playback::capture_mode          capture_mode;               /* indicates on the way that the frames were capture by */
            playback::file_format           type;                       /* indicates on the file format, whether the file was captured using RSSDK or Linux SDK */
        };

        /**
        * @class rs::playback::device
        * @brief The rs::playback::device extends rs::device to provide playback capabilities. Commonly used for debug, testing and validation with known input.
        *
        * The playback device reads from the file the device static information, the captured device and streams configuration, and streams data.
        * The playback device can be configured to either to imitate a real time behavior in which the frames are provided to the application according to the time it was captured.
        * In the real time mode frame drop might occur in dependency of the application behavior.
        * In non-real time mode, frames are not dropped, the frames are all provided on a single reading thread and the playback rate is determined only by the application behavior.
        * Creating the rs::playback::device and defining the source file location is done using rs::playback::context.
        * The playback device supports playing a single session of streaming.
        */
        class device : public rs::device
        {
        public:
            /**
            * @brief Indicates the played frame rate - with or without real time frame rate delays.
            *
            * For more details see rs::playback::device::set_real_time method.
            * @return bool     Real time state.
            */
            bool is_real_time();

            /**
            * @brief Pause streaming and leave the file read pointer on its current position.
            *
            * Resume streaming after pause is done by calling rs::playback::device::start().
            */
            void pause();

            /**
            * @brief Set the current frames by a given stream type and index.
            *
            * Other streams will be set by the nearest capture time.
            * The file read pointer will be set to the frame with latest capture time.
            * @param[in] index  A zero based frame index to set the file read pointer.
            * @param[in] stream  The stream type for which the frame is selected.
            * @return bool
            * true     Set succeed and frames data is available.
            * false    Failed to set the file read pointer to the requested index.
            */
            bool set_frame_by_index(int index, rs::stream stream);

            /**
            * @brief Locate the frame after a given time stamp.
            *
            * Other streams will be set by the nearest capture time.
            * The file read pointer will be set to the frame with latest capture time.
            * @param[in] timestamp  The time in millisecond to seek according to.
            * @return bool
            * true     Set succeed and frames data is available.
            * false    Failed to set the file read pointer to the requested time.
            */
            bool set_frame_by_timestamp(uint64_t timestamp);

            /**
            * @brief Sets the state of the real time flag.
            *
            * Enabled by default.
            * Non real time mode delivers all samples without any sample drop, according to the application sample processing latency.
            * This mode is designed for a single consumer, as the next sample delivery is blocked by current sample processing, faster or slower than original camera FPS.
            * @param[in] real time  The requested state.
            */
            void set_real_time(bool realtime);

            /**
            * @brief Gets the index of current frame.
            *
            * @param[in] stream  The stream type for which the frame is queried.
            * @return int      The frame index.
            */
            int get_frame_index(rs::stream stream);

            /**
            * @brief Gets the total count of captured frames in the file.
            *
            * @param[in] stream  The stream type for which the frame count is queried.
            * @return int      Frame count.
            */
            int get_frame_count(rs::stream stream);

            /**
            * @brief Gets the total frame count of the stream with the lowest frame count.
            *
            * @return int      Frame count.
            */
            int get_frame_count();

            /**
            * @brief Provide information of the software that the file was captured with and the way it was captured.
            *
            * Those parameters may influence the way the file can be played.
            * @return core::file_info      File info.
            */
            file_info get_file_info();
        };
    }
}


