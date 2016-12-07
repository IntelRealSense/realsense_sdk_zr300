// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense/rs.hpp>
#include <librealsense/rscore.hpp>
#include "rs/playback/playback_device.h"

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
        class DLL_EXPORT device_interface : public rs_device
        {
        public:
            virtual ~device_interface() {}
            virtual bool init() = 0;
            virtual bool is_real_time() = 0;
            virtual void pause() = 0;
            virtual void resume() = 0;
            virtual bool set_frame_by_index(int index, rs_stream stream) = 0;
            virtual bool set_frame_by_timestamp(uint64_t timestamp) = 0;
            virtual void set_real_time(bool realtime) = 0;
            virtual int get_frame_index(rs_stream stream) = 0;
            virtual int get_frame_count(rs_stream stream) = 0;
            virtual int get_frame_count() = 0;
            virtual playback::file_info get_file_info() = 0;
        };
    }
}
