// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <vector>
#include <map>
#include <memory>
#include "include/file_types.h"
#include "status.h"

namespace rs
{
    namespace playback
    {
        class disk_read_interface
        {
        public:
            disk_read_interface() {}
            virtual ~disk_read_interface(void) {}
            virtual core::status init() = 0;
            virtual void reset() = 0;
            virtual void resume() = 0;
            virtual void pause() = 0;
            virtual void enable_stream(rs_stream stream, bool state) = 0;
            virtual void enable_motions_callback(bool state) = 0;
            virtual bool is_motion_tracking_enabled() = 0;
            virtual const core::file_types::device_info& get_device_info() = 0;
            virtual std::map<rs_stream, core::file_types::stream_info> get_streams_infos() = 0;
            virtual std::vector<rs_capabilities> get_capabilities() = 0;
            virtual std::map<rs_option, double> get_properties() = 0;
            virtual void set_realtime(bool realtime) = 0;
            virtual std::map<rs_stream, std::shared_ptr<core::file_types::frame_sample>> set_frame_by_index(uint32_t index, rs_stream stream_type) = 0;
            virtual std::map<rs_stream, std::shared_ptr<core::file_types::frame_sample>> set_frame_by_time_stamp(uint64_t ts) = 0;
            virtual bool query_realtime() = 0;
            virtual uint32_t query_number_of_frames(rs_stream stream_type) = 0;
            virtual int32_t query_coordinate_system() = 0;
            virtual core::file_types::version query_sdk_version() = 0;
            virtual core::file_types::version query_librealsense_version() = 0;
            virtual bool is_stream_profile_available(rs_stream stream, int width, int height, rs_format format, int framerate) = 0;//TODO:[mk]consider moving to device
            virtual void set_callbak(std::function<void(std::shared_ptr<core::file_types::sample>)> handler) = 0;
            virtual void set_callbak(std::function<void()> handler) = 0;
        };
    }
}
