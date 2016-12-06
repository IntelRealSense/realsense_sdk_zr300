// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "cmd_base.h"
#include "rs/core/types.h"
#include "rs/record/record_device.h"

#ifdef WIN32 
#ifdef realsense_cl_util_EXPORTS
#define  DLL_EXPORT __declspec(dllexport)
#else
#define  DLL_EXPORT __declspec(dllimport)
#endif /* realsense_cl_util_EXPORTS */
#else /* defined (WIN32) */
#define DLL_EXPORT
#endif

namespace rs
{
    namespace utils
    {
        enum streaming_mode
        {
            live,
            record,
            playback
        };

        class DLL_EXPORT basic_cmd_util : public cmd_base
        {
        public:
            basic_cmd_util(const bool add_basic_options = true);
            std::vector<core::stream_type> get_enabled_streams();
            int get_stream_width(core::stream_type stream);
            int get_stream_height(core::stream_type stream);
            int get_stream_fps(core::stream_type stream);
            rs::record::compression_level get_compression_level(core::stream_type stream);
            core::pixel_format get_stream_pixel_format(core::stream_type stream);
            bool is_stream_profile_available(core::stream_type stream);
            bool is_stream_pixel_format_available(core::stream_type stream);
            int get_capture_time();
            size_t get_number_of_frames();
            bool is_real_time();
            bool is_print_file_info();
            bool is_rendering_enabled();
            bool is_motion_enabled();
            streaming_mode get_streaming_mode();
            std::string get_file_path(streaming_mode sm);
            std::string get_file_info();
        private:
            struct stream_profile
            {
                int	width;
                int	height;
                core::pixel_format	format;
                int	fps;
            };
            bool is_number(std::string str);
            bool get_profile_data(rs::core::stream_type stream, stream_profile &profile);
        };
    }
}
