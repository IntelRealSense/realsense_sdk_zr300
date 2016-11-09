// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <memory>
#include <sstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <librealsense/rs.hpp>
#include "rs_core.h"
#include "rs/core/context_interface.h"
#include "rs/playback/playback_context.h"
#include "rs/record/record_context.h"
#include "rs/playback/playback_device.h"
#include "rs/record/record_device.h"
#include "basic_cmd_util.h"
#include "viewer.h"
#include "rs/utils/librealsense_conversion_utils.h"

using namespace std;
using namespace rs::utils;
using namespace rs::core;

basic_cmd_util g_cmd;
std::shared_ptr<viewer> g_renderer;
std::map<rs::stream,size_t> g_frame_count;

auto g_frame_callback = [](rs::frame frame)
{
    g_frame_count[frame.get_stream_type()]++;
    if(!g_cmd.is_rendering_enabled())return;
    g_renderer->show_frame(std::move(frame));
};

auto g_motion_callback = [](rs::motion_data motion){};

std::shared_ptr<context_interface> create_context(basic_cmd_util cl_util)
{
    switch(cl_util.get_streaming_mode())
    {
        case streaming_mode::live: return std::shared_ptr<context_interface>(new context());
        case streaming_mode::record: return std::shared_ptr<context_interface>(new rs::record::context(cl_util.get_file_path(streaming_mode::record).c_str()));
        case streaming_mode::playback: return std::shared_ptr<context_interface>(new rs::playback::context(cl_util.get_file_path(streaming_mode::playback).c_str()));
    }
    return nullptr;
}

std::string stream_type_to_string(rs::stream stream)
{
    switch(stream)
    {
        case rs::stream::depth: return "depth";
        case rs::stream::color: return "color";
        case rs::stream::infrared: return "infrared";
        case rs::stream::infrared2: return "infrared2";
        case rs::stream::fisheye: return "fisheye";
        default: return "";
    }
}

std::string pixel_format_to_string(rs::format format)
{
    switch(format)
    {
        case rs::format::rgb8: return "rgb8";
        case rs::format::rgba8: return "rgba8";
        case rs::format::bgr8: return "bgr8";
        case rs::format::bgra8: return "bgra8";
        case rs::format::yuyv: return "yuyv";
        case rs::format::raw8: return "raw8";
        case rs::format::raw10: return "raw10";
        case rs::format::raw16: return "raw16";
        case rs::format::y8: return "y8";
        case rs::format::y16: return "y16";
        case rs::format::z16: return "z16";
        case rs::format::any: return "any";
        default: return "";
    }
}

void configure_device(rs::device* device, basic_cmd_util cl_util, std::shared_ptr<viewer> &renderer)
{
    const int window_width = 640;
    const int window_height = 480;
    auto streams = cl_util.get_enabled_streams();
    std::cout << "enabled streams:" << std::endl;
    for(auto it = streams.begin(); it != streams.end(); ++it)
    {
        auto lrs_stream = convert_stream_type(*it);
        auto is_playback = cl_util.get_streaming_mode() == streaming_mode::playback;
        auto is_record = cl_util.get_streaming_mode() == streaming_mode::record;

        bool is_stream_profile_available = cl_util.is_stream_profile_available(*it);
        bool is_stream_pixel_format_available = cl_util.is_stream_pixel_format_available(*it);

        device->set_frame_callback(lrs_stream, g_frame_callback);

        if(is_playback || !(is_stream_profile_available || is_stream_pixel_format_available))
        {
            device->enable_stream(lrs_stream, rs::preset::best_quality);
        }
        else
        {
            device->enable_stream(lrs_stream,
                                  cl_util.get_stream_width(*it),
                                  cl_util.get_stream_height(*it),
                                  convert_pixel_format(cl_util.get_stream_pixel_format(*it)),
                                  cl_util.get_stream_fps(*it));
        }

        if(is_playback)
        {
            ((rs::playback::device*)device)->set_real_time(g_cmd.is_real_time());
        }

        if(is_record)
        {
            float cl = cl_util.get_compression_level(*it);
            bool enable = cl >= 0;
            ((rs::record::device*)device)->set_compression(lrs_stream, enable, cl);
        }

        std::cout << "\t" << stream_type_to_string(lrs_stream) <<
                     " - width:" << device->get_stream_width(lrs_stream) <<
                     ", height:" << device->get_stream_height(lrs_stream) <<
                     ", fps:" << device->get_stream_framerate(lrs_stream) <<
                     ", pixel format:" << pixel_format_to_string(device->get_stream_format(lrs_stream)) << std::endl;

        if(g_cmd.is_motion_enabled())
        {
            device->enable_motion_tracking(g_motion_callback);
        }
    }
    if(cl_util.is_rendering_enabled())
    {
        renderer = std::make_shared<viewer>(streams.size(), window_width, window_height, [device]()
        {
            rs::source source = g_cmd.is_motion_enabled() ? rs::source::all_sources : rs::source::video;
            device->stop(source);
            cout << "done capturing" << endl;
            exit(0);
        });
    }
}

int main(int argc, char* argv[])
{
    try
    {
        rs::utils::cmd_option opt;
        if(!g_cmd.parse(argc, argv))
        {
            g_cmd.get_cmd_option("-h --h -help --help -?", opt);
            exit(-1);
        }

        if(g_cmd.get_cmd_option("-h --h -help --help -?", opt))
        {
            std::cout << g_cmd.get_help();
            exit(0);
        }

        std::cout << g_cmd.get_selection();

        std::shared_ptr<context_interface> context = create_context(g_cmd);

        if(context->get_device_count() == 0)
        {
            throw std::runtime_error("no device detected");
        }

        rs::device * device = context->get_device(0);

        configure_device(device, g_cmd, g_renderer);

        rs::source source = g_cmd.is_motion_enabled() ? rs::source::all_sources : rs::source::video;

        device->start(source);

        auto start_time = std::chrono::high_resolution_clock::now();

        switch(g_cmd.get_streaming_mode())
        {
            case rs::utils::streaming_mode::playback:
            {
                auto capture_time = g_cmd.get_capture_time();
                while(device->is_streaming())
                {
                    if(capture_time)
                    {
                        auto now = std::chrono::high_resolution_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
                        if(duration >= capture_time)break;
                    }
                    std::this_thread::sleep_for (std::chrono::milliseconds(100));
                }
                break;
            }
            default:
            {
                if(g_cmd.get_number_of_frames())
                {
                    bool done = false;
                    auto frames = g_cmd.get_number_of_frames();
                    cout << "start capturing " << frames << " frames" << endl;
                    while(!done && device->is_streaming())
                    {
                        if(g_frame_count.size() == 0)continue;
                        done = true;
                        for(auto it = g_frame_count.begin(); it != g_frame_count.end(); ++it)
                        {
                            if(it->second < frames)
                                done = false;
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(15));
                    }
                    break;
                }
                if(g_cmd.get_capture_time())
                {
                    cout << "start capturing for " << to_string(g_cmd.get_capture_time()) << " second" << endl;
                    std::this_thread::sleep_for (std::chrono::seconds(g_cmd.get_capture_time()));
                }
                else
                {
                    cout << "start capturing" << endl;
                    int key = '0';
                    while (key != 'q')
                    {
                        key = getchar();
                    }
                }
                break;
            }
        }

        device->stop(source);

        cout << "done capturing" << endl;
        return 0;
    }
    catch(rs::error e)
    {
        cout << e.what() << endl;
        return -1;
    }
    catch(string e)
    {
        cout << e << endl;
        return -1;
    }
}
