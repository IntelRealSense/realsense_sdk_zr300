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
            synced  = 1,  /**<  blocking read until an image from each enabled stream is available */
            asynced = 2   /**<  images are indicated to the application through camera notifications */
        };

        enum file_format
        {
            rs_rssdk_format = 0,  /**<  file structure is of Windows / Android RSSDK format */
            rs_linux_format = 1   /**<  file structure is of Linux SDK format */
        };

        /**
        * @struct rs::playback::file_info
        * @brief Describes the record software stack versions and file configuration.
        */
        struct file_info
        {
            int32_t                         version;                     /**<  Linux SDK file version - defines the file format and available data */
            char                            sdk_version[32];             /**<  Linux SDK version that was used to capture the file */
            char                            librealsense_version[32];    /**<  librealsense version that was used to capture the file */
            playback::capture_mode          capture_mode;                /**<  indicates how the frames were captured by the recording application */
            playback::file_format           type;                        /**<  indicates on the file format, which is derived from the SW stack that recorded it - Windows/Android RSSDK or Linux SDK */
        };

        /**
        * @class rs::playback::device
        * @brief rs::playback::device extends rs::device to provide playback capabilities. Commonly used for debug, testing and validation with known input.
        *
        * The playback device reads the device static information, the captured device configuration, streams configuration and streams data from the file.
        * The playback device can be configured to run as in real time mode, as a live camera, or non real time mode, as a file camera.
        * Creating the rs::playback::device and defining the source file location is done using rs::playback::context.
        */
        class device : public rs::device
        {
        public:
            /**
            * @brief Pause streaming and keep the current file read location.
            *
            * While the streaming is in paused state the application can access all device queries. Images read is blocked, and no new samples indication is triggered.
            * File streaming can be resumed after pause by calling device start.
            */
            void pause();

            /**
            * @brief Set the current file read location by the requested stream type and index.
            *
            * The file read pointer is set according to the time correlated set of all enabled streams in the playback session.
            * It selects the stream frame with the requested index, while the other streams frames are selected by the nearest capture time.
            * The file read pointer is set to the frame with latest capture time.
            * While the function executes, the streaming state is set to paused. After the function returns, the device returns to the original streaming state.
            * While the function executes, other device operations are unsupported, and have unexpected behavior.
            * @param[in] index  A zero based frame index to set the file read pointer.
            * @param[in] stream  The stream type for which the frame is selected.
            * @return bool
            * true     Set succeed and frames data is available.
            * false    Failed to set the file read pointer to the requested index.
            */
            bool set_frame_by_index(int index, rs::stream stream);

            /**
            * @brief Set the current file read location to the frame according to the requested time stamp.
            *
            * The file read pointer location is set according to the time correlated set of all enabled streams in the playback session.
            * It locates the first frame of any enabled stream, with capture time larger than the requested time stamp.
            * The other streams frames are selected by the nearest capture time.
            * The file read pointer will be set to the frame with latest capture time.
            * While the function executes, the streaming state is set to paused. After the function returns, the device returns to the original streaming state.
            * While the function executes, other device operations are unsupported, and have unexpected behavior.
            * @param[in] timestamp  The time in millisecond to seek according to.
            * @return bool
            * true     Set succeed and frames data is available.
            * false    Failed to set the file read pointer to the requested time.
            */
            bool set_frame_by_timestamp(uint64_t timestamp);

            /**
            * @brief Gets the index of current frame.
            *
            * This function can be called only if capture andplayback mode is sync, or after set_frame_by_index / set_frame_by_timestamp was called.
            * This function is not supported for async playback mode.
            * When the function is supported, it can be called in any streaming state.
            * @param[in] stream  The stream type for which the frame is queried.
            * @return int      The frame index.
            */
            int get_frame_index(rs::stream stream);

            /**
            * @brief Sets the playback mode to real time or non real time
            *
            * The real time mode selection defines the rate of samples delivery to the application.
            * Real time mode imitates the real time behavior of the file record session, in which the frames are provided to the application according to the actual time it was captured.
            * In this mode frame drops might occur according to the playback runtime behavior - application or system latency can cause the device to skip recorded frames.
            * Non-real time mode ignores the actual capture time, and delivers all frames without drops. The playback time and frame rate depends only on the application and system behavior.
            * The default mode is real time.
            * This mode is designed for a single consumer, as the next sample delivery is blocked by current sample processing, faster or slower than original camera FPS.
            * @param[in] real time  The requested state.
            */
            void set_real_time(bool realtime);

            /**
            * @brief Indicates the playback real time mode
            *
            * For more details see rs::playback::device::set_real_time method.
            * @return bool     Real time state.
            */
            bool is_real_time();

            /**
            * @brief Gets the total frame count of the requested stream captured in the file.
            *
            * @param[in] stream  The stream type for which the frame count is queried.
            * @return int        Frame count.
            */
            int get_frame_count(rs::stream stream);

            /**
            * @brief Gets the total frame count of the stream with the lowest frame count captured in the file.
            *
            * @return int      Frame count.
            */
            int get_frame_count();

            /**
            * @brief Provides information of the software stack, with which the file was captured, and the way it was captured.
            *
            * Those parameters may influence the way the file can be played.
            * @return core::file_info      File info.
            */
            file_info get_file_info();
        };
    }
}


