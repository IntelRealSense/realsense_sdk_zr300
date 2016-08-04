// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <memory>
#include <sstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <librealsense/rs.hpp>
#include "rs_core.h"
#include "rs/playback/playback_context.h"
#include "rs/record/record_context.h"
#include "command_line/basic_cmd_util.h"
#include "viewer/viewer.h"
#include "rs/utils/librealsense_conversion_utils.h"


#define WINDOW_SIZE 320

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

std::shared_ptr<context> create_context(basic_cmd_util cl_util)
{
    switch(cl_util.get_streaming_mode())
    {
        case streaming_mode::live: return std::shared_ptr<context>(new context());
        case streaming_mode::record: return std::shared_ptr<context>(new rs::record::context(cl_util.get_file_path(streaming_mode::record)));
        case streaming_mode::playback: return std::shared_ptr<context>(new rs::playback::context(cl_util.get_file_path(streaming_mode::playback)));
    }
    return nullptr;
}

void configure_device(rs::device* device, basic_cmd_util cl_util, std::shared_ptr<viewer> &renderer)
{
    auto streams = cl_util.get_enabled_streams();
    for(auto it = streams.begin(); it != streams.end(); ++it)
    {
        auto lrs_stream = convert_stream_type(*it);
        if(!cl_util.is_stream_profile_available(*it))
        {
            device->enable_stream(lrs_stream, rs::preset::best_quality);
        }
        else
        {
            auto width = cl_util.get_stream_width(*it);
            auto height = cl_util.get_stream_height(*it);
            auto fps = cl_util.get_stream_fps(*it);
            auto format = convert_pixel_format(cl_util.get_streanm_pixel_format(*it));
            device->enable_stream(lrs_stream, width, height, format, fps);
        }
        device->set_frame_callback(lrs_stream, g_frame_callback);
    }
    if(cl_util.is_rendering_enabled())
    {
        renderer = std::make_shared<viewer>(device, WINDOW_SIZE);
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
            std::cout << g_cmd.get_help();
            exit(-1);
        }

        if(g_cmd.get_cmd_option("-h --h -help --help -?", opt))
        {
            std::cout << g_cmd.get_help();
            exit(0);
        }

        std::cout << g_cmd.get_selection();

        std::shared_ptr<context> context = create_context(g_cmd);

        if(context->get_device_count() == 0)
        {
            throw "no device detected";
        }

        rs::device* device = context->get_device(0);

        configure_device(device, g_cmd, g_renderer);

        rs::source source = g_cmd.is_motion_enabled() ? rs::source::all_sources : rs::source::video;

        device->start(source);

        switch(g_cmd.get_streaming_mode())
        {
            case rs::utils::streaming_mode::playback:
            {
                while(device->is_streaming())
                {
                    std::this_thread::sleep_for (std::chrono::seconds(1));
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
                        std::this_thread::sleep_for (std::chrono::milliseconds(15));
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
                    char key = '0';
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
