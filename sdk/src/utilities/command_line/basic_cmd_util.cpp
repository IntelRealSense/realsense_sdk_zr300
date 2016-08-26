// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <map>
#include "basic_cmd_util.h"

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
        basic_cmd_util::basic_cmd_util()
        {
            auto enabled_stream_map = create_enabled_streams_map();
            auto streams_config_map = create_streams_config_map();

            add_option("-h --h -help --help -?", "show help");

            add_option("-m -motion", "enable motion events");

            add_option(enabled_stream_map[stream_type::depth], "enable depth stream");
            add_multy_args_option_safe(streams_config_map[stream_type::depth], "set depth profile - [<width>-<height>-<fps>]", 3, '-');
            add_single_arg_option("-dpf", "set depth streams pixel format", "z16", "z16");

            add_option(enabled_stream_map[stream_type::color], "enable color stream");
            add_multy_args_option_safe(streams_config_map[stream_type::color], "set color stream profile - [<width>-<height>-<fps>]", 3, '-');
            add_single_arg_option("-cpf", "set color stream pixel format", "rgb8 rgba8 bgr8 bgra8 yuyv", "rgb8");

            add_option(enabled_stream_map[stream_type::infrared], "enable infrared stream");
            add_multy_args_option_safe(streams_config_map[stream_type::infrared], "set infrared stream profile - [<width>-<height>-<fps>]", 3, '-');
            add_single_arg_option("-ipf", "set infrared streams pixel format", "y8 y16", "y8");

            add_option(enabled_stream_map[stream_type::infrared2], "enable infrared2 stream");
            add_multy_args_option_safe(streams_config_map[stream_type::infrared2], "set infrared2 stream profile - [<width>-<height>-<fps>]", 3, '-');
            add_single_arg_option("-i2pf", "set infrared2 streams pixel format", "y8 y16", "y8");

            add_option(enabled_stream_map[stream_type::fisheye], "enable fisheye stream");
            add_multy_args_option_safe(streams_config_map[stream_type::fisheye], "set fisheye stream profile - [<width>-<height>-<fps>]", 3, '-');
            add_single_arg_option("-fpf", "set fisheye stream pixel format", "raw8", "raw8");

            add_single_arg_option("-rec -record", "set recorder file path");
            add_single_arg_option("-pb -playback", "set playback file path");
            add_single_arg_option("-ct -capture_time", "set capture time");
            add_single_arg_option("-n", "set number of frames to capture");
            add_option("-r -render", "enable streaming display");

            set_usage_example("-c -cconf 640-480-30 -cpf rgba8 -rec rec.rssdk -r\n\n"
                              "The following command will configure the camera to\n"
                              "capture color stream of VGA resolution at 30 frames\n"
                              "per second in rgba8 pixel format.\n"
                              "The stream will be renderd to screen and will be saved\n"
                              "to rec.rssdk file.");
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
            if(!get_profile_data(stream, sp))return 0;
            return sp.width;
        }

        int basic_cmd_util::get_stream_height(core::stream_type stream)
        {
            stream_profile sp;
            if(!get_profile_data(stream, sp))return 0;
            return sp.height;
        }

        int basic_cmd_util::get_stream_fps(core::stream_type stream)
        {
            stream_profile sp;
            if(!get_profile_data(stream, sp))return 0;
            return sp.fps;
        }

        core::pixel_format basic_cmd_util::get_streanm_pixel_format(core::stream_type stream)
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

        bool basic_cmd_util::is_rendering_enabled()
        {
            rs::utils::cmd_option opt;
            return get_cmd_option("-r -render", opt);
        }

        bool basic_cmd_util::is_motion_enabled()
        {
            rs::utils::cmd_option opt;
            return get_cmd_option("-m -motion", opt);
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
    }
}
