// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/** 
* \file record_device.h
* @brief Describes the \c rs::record::device class.
*/
  
#pragma once
#include <librealsense/rs.hpp>
#include "rs/core/status.h"

#ifdef WIN32 
#ifdef realsense_record_EXPORTS
#define  DLL_EXPORT __declspec(dllexport)
#else
#define  DLL_EXPORT __declspec(dllimport)
#endif /* realsense_record_EXPORTS */
#else /* defined (WIN32) */
#define DLL_EXPORT
#endif

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
        * @brief Extends librealsense \c rs::device to provide record capabilities. Commonly used for debug, testing and validation with known input.
        *
        * Accessing a camera using the record device captures the session into a file.
        * The record device writes to the file the device static information, current device and streams configuration, and streams data while streaming.
        * All the video frames and motion samples that the application receives while streaming using the record device are captured to the file.
        * In case the application causes frame drops, the dropped frames are not captured to the file.
        * Creating the \c rs::record::device and defining the target file location is done using \c rs::record::context.
		*
        * The record device supports recording a single session of streaming. Hence, a single device and streams configuration is captured.
        */
        class DLL_EXPORT device : public rs::device
        {
        public:
            /**
            * @brief Pauses recording.
            *
            * Stops recording to file without modifying the streaming state of the device.
            * The application still gets all camera streams data.
            * If the method is called before device start, recording doesn't start until \c resume_record() is called.
            * The method can be called sequentially with \c resume_record() multiple times.
            * If the method is called while the device state is already paused, the call is ignored.
            */
            void pause_record();

            /**
            * @brief Resumes recording.
            *
            * Continues recording to file without modifying the streaming state of the device.
            * The default recording state is on. This method should be called only after \c pause_record() was called.
            * The method can be called sequentially with \c pause_record() multiple times.
            * If the method is called while the device state is already recording, the call is ignored.
			*
            * Resume recording concatenates the captured streams data to the end of the same file.
            * The time gaps will appear to the application upon streaming from the file in playback.
            */
            void resume_record();

            /**
            * @brief Sets the selected stream compression behavior.
            *
            * The method can be called only before record device start is called. The call is ignored while record device is in streaming state.
            * Setting the compression level adjusts the recorded file size: a higher level creates a smaller file and increases the CPU utilization.
            * The default compression level is high, if no other level setting is made by the user.
            * 
			* Disabling the compression is performed by setting \c compression_level to disabled
            * @param[in] stream  Stream for which the compression properties are requested
            * @param[in] compression_level  Requested compression level
            * @return status_no_error Successful execution.
            * @return status_invalid_argument Compression level value is out of legal range.
            */
            core::status set_compression(rs::stream stream, compression_level compression_level);

            /** @brief Get the selected stream compression level.
            *
            * The method returns the current compression level of the requested stream.
            * @param[in] stream Stream for which the compression properties are requested
            * @return compression_level Requested compression level
            */
            compression_level get_compression_level(rs::stream stream);
        };
    }
}
