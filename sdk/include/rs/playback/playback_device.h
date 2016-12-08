// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/** 
* \file playback_device.h
* @brief Describes the \c rs::playback::device class, \c rs::playback::capture_mode 
* and \c rs::playback::file_format enums, and \c rs::playback::file_info struct.
*/
  
#pragma once
#include <librealsense/rs.hpp>

#ifdef WIN32 
#ifdef realsense_playback_EXPORTS
#define  DLL_EXPORT __declspec(dllexport)
#else
#define  DLL_EXPORT __declspec(dllimport)
#endif /* realsense_playback_EXPORTS */
#else /* defined (WIN32) */
#define DLL_EXPORT
#endif

namespace rs
{
    namespace playback
    {
        /**
        * @brief Capture modes
        */
		enum capture_mode
        {
            synced  = 1,  /**<  Blocking read until an image from each enabled stream is available */
            asynced = 2   /**<  Images are indicated to the application through camera notifications */
        };

        /**
        * @brief File formats.
        */
		enum file_format
        {
            rs_rssdk_format = 0,  /**<  Windows/Android RSSDK format */
            rs_linux_format = 1   /**<  Linux SDK format */
        };

        /**
        * @brief Describes the record software stack versions and file configuration.
        */
        struct file_info
        {
            int32_t                         version;                     /**<  Linux SDK file version: defines the file format and available data */
            char                            sdk_version[32];             /**<  Linux SDK version that was used to capture the file */
            char                            librealsense_version[32];    /**<  librealsense version that was used to capture the file */
            playback::capture_mode          capture_mode;                /**<  Indicates how the frames were captured by the recording application */
            playback::file_format           type;                        /**<  Indicates the file format, which is derived from the software stack that recorded it: Windows/Android RSSDK or Linux SDK */
        };

        /**
        * @brief Extends librealsense \c rs::device to provide playback capabilities. Commonly used for debug, testing and validation with known input.
        *
        * The playback device reads the device static information, the captured device configuration, streams configuration, and streams data from the file.
        * The playback device can be configured to run as in real time mode, as a live camera, or non-real time mode, as a file camera.
        * Some of the captured data, such as frame metadata fields, reflects the actual behavior at the time of recording, and not the actual playback behavior.
        * Creating the \c rs::playback::device and defining the source file location is done using \c rs::playback::context.
        */
        class DLL_EXPORT device : public rs::device
        {
        public:
            /**
            * @brief Pause streaming and keep the current file read location.
            *
            * While the streaming is paused, the application can access all device queries. Image reading is blocked, and no new sample indication is triggered.
            * File streaming can be resumed after pause by calling device start.
            */
            void pause();

            /**
            * @brief Sets the current file read location by the requested stream type and index.
            *
            * The file read pointer is set according to the time-correlated set of all enabled streams in the playback session.
            * It selects the stream frame with the requested index, while the other streams frames are selected by the nearest capture time.
            * The file read pointer is set to the frame with latest capture time.
            * While the method executes, the streaming state is set to paused. After the method returns, the device returns to the original streaming state.
            * While the method executes, other device operations are unsupported, and have unexpected behavior.
            * @param[in] index Zero-based frame index to set the file read pointer
            * @param[in] stream  Stream type for which the frame is selected
            * @return bool
            * - true     Set succeeded and frame data is available
            * - false    Failed to set the file read pointer to the requested index
            */
            bool set_frame_by_index(int index, rs::stream stream);

            /**
            * @brief Sets the current file read location to the frame according to the requested timestamp.
            *
            * The file read pointer location is set according to the time correlated set of all enabled streams in the playback session.
            * It locates the first frame of any enabled stream, with capture time larger than the requested timestamp.
            * The other streams frames are selected by the nearest capture time.
            * The file read pointer will be set to the frame with latest capture time.
            * While the method executes, the streaming state is set to paused. After the method returns, the device returns to the original streaming state.
            * While the method executes, other device operations are unsupported, and have unexpected behavior.
            * @param[in] timestamp  Time, in milliseconds
            * @return
            * - true     Set succeeded and frame data is available
            * - false    Failed to set the file read pointer to the requested index
            */
            bool set_frame_by_timestamp(uint64_t timestamp);

            /**
            * @brief Gets the index of current frame.
            *
            * This method can be called only if capture and playback mode is sync, or after \c set_frame_by_index() or \c set_frame_by_timestamp() was called.
            * This method is not supported for async playback mode.
            * When the method is supported, it can be called in any streaming state.
            * @param[in] stream  Stream type for which the frame is queried
            * @return int Frame index
            */
            int get_frame_index(rs::stream stream);

            /**
            * @brief Sets the playback mode to real time or non real time
            *
            * The real time mode selection defines the rate of samples delivery to the application.
            * Real time mode imitates the real time behavior of the file record session, in which the frames are provided to the application according to the actual time it was captured.
            * In this mode frame drops might occur according to the playback runtime behavior - application or system latency can cause the device to skip recorded frames.
            * Non-real time mode ignores the actual capture time and delivers all frames without drops. The playback time and frame rate depends only on the application 
			* and system behavior.
            * The default mode is real time.
            * This mode is designed for a single consumer, as the next sample delivery is blocked by current sample processing, faster or slower than the original camera FPS.
            * @param[in] real time Requested state
            */
            void set_real_time(bool realtime);

            /**
            * @brief Indicates the playback real time mode
            *
            * For more details, see the \c rs::playback::device::set_real_time() method.
            * @return bool Real time state
            */
            bool is_real_time();

            /**
            * @brief Gets the total frame count of the requested stream captured in the file.
            *
            * @param[in] stream  Stream type for which the frame count is queried
            * @return int        Frame count
            */
            int get_frame_count(rs::stream stream);

            /**
            * @brief Gets the total frame count of the stream with the lowest frame count captured in the file.
            *
            * @return int Frame count
            */
            int get_frame_count();

            /**
            * @brief Provides information of the software stack, with which the file was captured, and the way it was captured.
            *
            * Those parameters may influence the way the file can be played.
            * @return core::file_info File info.
            */
            file_info get_file_info();
        };
    }
}


