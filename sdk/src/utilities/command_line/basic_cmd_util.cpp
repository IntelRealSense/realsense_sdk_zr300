// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <map>
#include "basic_cmd_util.h"
#include <cctype>
#include "rs_sdk_version.h"
#include "rs/playback/playback_context.h"
#include "rs/playback/playback_device.h"
#include "rs/utils/librealsense_conversion_utils.h"

using namespace rs::core;

namespace
{
    static std::map<stream_type,std::string> create_enabled_streams_map()
    {
        std::map<stream_type,std::string> rv;
        rv[stream_type::depth] = "-d -depth";
        rv[stream_type::color] = "-c -color";
        rv[stream_type::infrared] = "-i -infrared";
        rv[stream_type::infrared2] = "-i2 -infrared2";
        rv[stream_type::fisheye] = "-f -fisheye";
        return rv;
    }

    static std::map<stream_type,std::string> create_streams_config_map()
    {
        std::map<stream_type,std::string> rv;
        rv[stream_type::depth] = "-dconf";
        rv[stream_type::color] = "-cconf";
        rv[stream_type::infrared] = "-iconf";
        rv[stream_type::infrared2] = "-i2conf";
        rv[stream_type::fisheye] = "-fconf";
        return rv;
    }

    static std::map<stream_type,std::string> create_streams_pixel_format_map()
    {
        std::map<stream_type,std::string> rv;
        rv[stream_type::depth] = "-dpf";
        rv[stream_type::color] = "-cpf";
        rv[stream_type::infrared] = "-ipf";
        rv[stream_type::infrared2] = "-i2pf";
        rv[stream_type::fisheye] = "-fpf";
        return rv;
    }

    static std::map<stream_type,std::string> create_streams_compression_level_map()
    {
        std::map<stream_type,std::string> rv;
        rv[stream_type::depth] = "-dcl";
        rv[stream_type::color] = "-ccl";
        rv[stream_type::infrared] = "-icl";
        rv[stream_type::infrared2] = "-i2cl";
        rv[stream_type::fisheye] = "-fcl";
        return rv;
    }

    static std::map<std::string,pixel_format> create_formats_map()
    {
        std::map<std::string, pixel_format> rv;
        rv["z16"] = pixel_format::z16;
        rv["disp"] = pixel_format::disparity16;
        rv["xyz"] = pixel_format::xyz32f;
        rv["yuyv"] = pixel_format::yuyv;
        rv["rgb8"] = pixel_format::rgb8;
        rv["bgr8"] = pixel_format::bgr8;
        rv["rgba8"] = pixel_format::rgba8;
        rv["bgra8"] = pixel_format::bgra8;
        rv["y8"] = pixel_format::y8;
        rv["y16"] = pixel_format::y16;
        rv["raw8"] = pixel_format::raw8;
        rv["raw10"] = pixel_format::raw10;
        rv["raw16"] = pixel_format::raw16;
        return rv;
    }
}

namespace rs
{
    namespace utils
    {
        basic_cmd_util::basic_cmd_util(const bool add_basic_options)
        {
            if (add_basic_options)
            {
                auto enabled_stream_map = create_enabled_streams_map();
                auto streams_config_map = create_streams_config_map();

                add_option("-h --h -help --help -?", "show help");

                add_option("-m -motion", "enable motion events recording");

                add_option(enabled_stream_map[stream_type::depth], "enable depth stream");
                add_multy_args_option_safe(streams_config_map[stream_type::depth], "set depth profile - [<width>-<height>-<fps>]", 3, '-');
                add_single_arg_option("-dpf", "set depth stream pixel format", "z16", "z16");
                add_single_arg_option("-dcl", "set depth stream compression level", "d l m h", "h");

                add_option(enabled_stream_map[stream_type::color], "enable color stream");
                add_multy_args_option_safe(streams_config_map[stream_type::color], "set color stream profile - [<width>-<height>-<fps>]", 3, '-');
                add_single_arg_option("-cpf", "set color stream pixel format", "rgb8 rgba8 bgr8 bgra8 yuyv", "rgba8");
                add_single_arg_option("-ccl", "set color stream compression level", "d l m h", "h");

                add_option(enabled_stream_map[stream_type::infrared], "enable infrared stream");
                add_multy_args_option_safe(streams_config_map[stream_type::infrared], "set infrared stream profile - [<width>-<height>-<fps>]", 3, '-');
                add_single_arg_option("-ipf", "set infrared stream pixel format", "y8 y16", "y8");
                add_single_arg_option("-icl", "set infrared stream compression level", "d l m h", "h");

                add_option(enabled_stream_map[stream_type::infrared2], "enable infrared2 stream");
                add_multy_args_option_safe(streams_config_map[stream_type::infrared2], "set infrared2 stream profile - [<width>-<height>-<fps>]", 3, '-');
                add_single_arg_option("-i2pf", "set infrared2 stream pixel format", "y8 y16", "y8");
                add_single_arg_option("-i2cl", "set infrared2 stream compression level", "d l m h", "h");

                add_option(enabled_stream_map[stream_type::fisheye], "enable fisheye stream, enables motion tracking by default");
                add_multy_args_option_safe(streams_config_map[stream_type::fisheye], "set fisheye stream profile - [<width>-<height>-<fps>]", 3, '-');
                add_single_arg_option("-fpf", "set fisheye stream pixel format", "raw8", "raw8");
                add_single_arg_option("-fcl", "set fisheye stream compression level", "d l m h", "h");

                add_single_arg_option("-rec -record", "set recorder file path");
                add_single_arg_option("-pb -playback", "set playback file path");
                add_single_arg_option("-fi -file_info", "print file info");
                add_single_arg_option("-ct -capture_time", "set capture time");
                add_single_arg_option("-n", "set minimum number of frames to capture per stream");
                add_option("-r -render", "enable streaming display");
                add_option("-nrt -non_real_time", "playback in non real time mode");

                set_usage_example("-c -cconf 640-480-30 -cpf rgba8 -rec rec.rssdk -r\n\n"
                                  "The following command will configure the camera to\n"
                                  "capture color stream of VGA resolution at 30 frames\n"
                                  "per second in rgba8 pixel format.\n"
                                  "The stream will be renderd to screen and will be saved\n"
                                  "to rec.rssdk file.");
            }
        }

        bool basic_cmd_util::is_number(std::string str)
        {
            return !str.empty() && std::find_if(str.begin(),
            str.end(), [](char c) { return !std::isdigit(c); }) == str.end();
        }

        bool basic_cmd_util::get_profile_data(stream_type stream, stream_profile &profile)
        {
            try
            {
                auto streams_config_map = create_streams_config_map();
                auto pixel_formats = create_streams_pixel_format_map();
                auto pixel_formats_names = create_formats_map();
                cmd_option opt;
                bool sts = get_cmd_option(streams_config_map[stream], opt);
                if(!sts) return false;
                if(opt.m_option_args_values.size() != 3) return false;
                for(auto arg : opt.m_option_args_values) if(!is_number(arg))return false;
                profile.width = std::stoi(opt.m_option_args_values[0]);
                profile.height = std::stoi(opt.m_option_args_values[1]);
                profile.fps = std::stoi(opt.m_option_args_values[2]);
                sts = get_cmd_option(pixel_formats.at(stream), opt);
                profile.format = pixel_formats_names.at(sts ? opt.m_option_args_values[0] : opt.m_default_value);
                return true;
            }
            catch(...)
            {
                return false;
            }
        }
        std::vector<core::stream_type> basic_cmd_util::get_enabled_streams()
        {
            auto enabled_stream_map = create_enabled_streams_map();
            std::vector<core::stream_type> rv;
            rs::utils::cmd_option opt;
            if(get_cmd_option(enabled_stream_map[stream_type::depth], opt))
                rv.push_back(core::stream_type::depth);
            if(get_cmd_option(enabled_stream_map[stream_type::color], opt))
                rv.push_back(core::stream_type::color);
            if(get_cmd_option(enabled_stream_map[stream_type::infrared], opt))
                rv.push_back(core::stream_type::infrared);
            if(get_cmd_option(enabled_stream_map[stream_type::infrared2], opt))
                rv.push_back(core::stream_type::infrared2);
            if(get_cmd_option(enabled_stream_map[stream_type::fisheye], opt))
                rv.push_back(core::stream_type::fisheye);
            return rv;
        }

        int basic_cmd_util::get_stream_width(core::stream_type stream)
        {
            stream_profile sp;
            if(!get_profile_data(stream, sp))return 640;
            return sp.width;
        }

        int basic_cmd_util::get_stream_height(core::stream_type stream)
        {
            stream_profile sp;
            if(!get_profile_data(stream, sp))return 480;
            return sp.height;
        }

        int basic_cmd_util::get_stream_fps(core::stream_type stream)
        {
            stream_profile sp;
            if(!get_profile_data(stream, sp))return 30;
            return sp.fps;
        }

        rs::record::compression_level basic_cmd_util::get_compression_level(core::stream_type stream)
        {
            auto compression_level_map = create_streams_compression_level_map();
            cmd_option opt;
            bool sts = get_cmd_option(compression_level_map.at(stream), opt);
            std::string val = sts ? opt.m_option_args_values[0] : opt.m_default_value;
            if(val == "d") return rs::record::compression_level::disabled;
            if(val == "l") return rs::record::compression_level::low;
            if(val == "m") return rs::record::compression_level::medium;
            return rs::record::compression_level::high;
        }

        core::pixel_format basic_cmd_util::get_stream_pixel_format(core::stream_type stream)
        {
            auto pixel_formats = create_streams_pixel_format_map();
            auto pixel_formats_names = create_formats_map();
            cmd_option opt;
            bool sts = get_cmd_option(pixel_formats.at(stream), opt);
            return pixel_formats_names.at(sts ? opt.m_option_args_values[0] : opt.m_default_value);
        }

        bool basic_cmd_util::is_stream_profile_available(core::stream_type stream)
        {
            stream_profile sp;
            return get_profile_data(stream, sp);
        }

        bool basic_cmd_util::is_stream_pixel_format_available(core::stream_type stream)
        {
            auto pixel_formats = create_streams_pixel_format_map();
            cmd_option opt;
            return get_cmd_option(pixel_formats.at(stream), opt);
        }

        int basic_cmd_util::get_capture_time()
        {
            try
            {
                rs::utils::cmd_option opt;
                if(!get_cmd_option("-ct -capture_time", opt)) return 0;
                if(opt.m_option_args_values.size() == 0) return 0;
                return is_number(opt.m_option_args_values[0]) ? std::stoi(opt.m_option_args_values[0]) : -1;
            }
            catch(...)
            {
                return 0;
            }
        }

        size_t basic_cmd_util::get_number_of_frames()
        {
            try
            {
                rs::utils::cmd_option opt;
                if(!get_cmd_option("-n", opt)) return 0;
                if(opt.m_option_args_values.size() == 0) return 0;
                return is_number(opt.m_option_args_values[0]) ? std::stoi(opt.m_option_args_values[0]) : -1;
            }
            catch(...)
            {
                return 0;
            }
        }

        bool basic_cmd_util::is_real_time()
        {
            rs::utils::cmd_option opt;
            return !get_cmd_option("-nrt -non_real_time", opt);
        }

        bool basic_cmd_util::is_print_file_info()
        {
            rs::utils::cmd_option opt;
            return get_cmd_option("-fi -file_info", opt);
        }

        bool basic_cmd_util::is_rendering_enabled()
        {
            rs::utils::cmd_option opt;
            return get_cmd_option("-r -render", opt);
        }

        bool basic_cmd_util::is_motion_enabled()
        {
            auto enabled_stream_map = create_enabled_streams_map();
            rs::utils::cmd_option opt;
            return get_cmd_option(enabled_stream_map[stream_type::fisheye], opt) || get_cmd_option("-m -motion", opt);
        }

        streaming_mode basic_cmd_util::get_streaming_mode()
        {
            rs::utils::cmd_option opt;
            if(get_cmd_option("-rec -record", opt)) return streaming_mode::record;
            if(get_cmd_option("-pb -playback", opt)) return streaming_mode::playback;
            return streaming_mode::live;
        }

        std::string basic_cmd_util::get_file_path(streaming_mode sm)
        {
            rs::utils::cmd_option opt;
            switch(sm)
            {
                case streaming_mode::record:
                {
                    if(!get_cmd_option("-rec -record", opt)) return "";
                    if(opt.m_option_args_values.size() == 0) return "";
                    return opt.m_option_args_values[0];
                }
                case streaming_mode::playback:
                {
                    if(!get_cmd_option("-pb -playback", opt)) return "";
                    if(opt.m_option_args_values.size() == 0) return "";
                    return opt.m_option_args_values[0];
                }
                default: return "";
            }
        }

        std::string basic_cmd_util::get_file_info()
        {
            rs::utils::cmd_option opt;
            bool file_info_request = get_cmd_option("-fi -file_info", opt);

            if(file_info_request == false || opt.m_option_args_values.size() == 0)
                return "";

            auto file_path = opt.m_option_args_values[0];
            rs::playback::context context(file_path.c_str());

            if(context.get_device_count() == 0)
                return "";

            auto device = context.get_playback_device();
            if(device == nullptr)
                return "";

            std::stringstream ss;
            auto info = device->get_file_info();
            std::string abfv = device->supports(rs::camera_info::adapter_board_firmware_version) ?
                        device->get_info(rs::camera_info::adapter_board_firmware_version) : "not supported";
            std::string mmfv = device->supports(rs::camera_info::motion_module_firmware_version) ?
                        device->get_info(rs::camera_info::motion_module_firmware_version) : "not supported";

            ss <<
                "device name:                    " << device->get_info(rs::camera_info::device_name) << std::endl <<
                "serial number:                  " << device->get_info(rs::camera_info::serial_number) << std::endl <<
                "camera firmware version:        " << device->get_info(rs::camera_info::camera_firmware_version) << std::endl <<
                "adapter board firmware version: " << abfv << std::endl <<
                "motion module firmware version: " << mmfv << std::endl <<
                "sdk_version:                    " << info.sdk_version << std::endl <<
                "librealsense_version:           " << info.librealsense_version << std::endl <<
                "file type:                      " << (info.type == rs::playback::file_format::rs_rssdk_format ? "rssdk format (Windows)" : "linux format") << std::endl <<
                "file version:                   " << info.version << std::endl <<
                "file capture mode:              " << (info.capture_mode == rs::playback::capture_mode::synced ? "synced" : "asynced") << std::endl <<
                "streams:" << std::endl;

            for(uint32_t i = (uint32_t)rs::stream::depth; i <= (uint32_t)rs::stream::fisheye; i++)
            {
                rs::stream stream  = static_cast<rs::stream>(i);
                if(device->get_stream_mode_count(stream) == 0)
                    continue;
                int width, height, fps;
                rs::format format;
                device->get_stream_mode(stream, 0, width, height, format, fps);
                ss << "\t" << stream <<
                    " - width: " << device->get_stream_width(stream) <<
                    ", height: " << device->get_stream_height(stream) <<
                    ", fps: " << device->get_stream_framerate(stream) <<
                    ", pixel format: " << format <<
                    ", frame count: " << device->get_frame_count(stream) << std::endl;
            }
            return ss.str();
        }
    }
}
