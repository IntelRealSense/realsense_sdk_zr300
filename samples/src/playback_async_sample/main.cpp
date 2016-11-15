// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <memory>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <librealsense/rs.hpp>
#include "rs_sdk.h"

using namespace rs::core;
using namespace std;

int main(int argc, char* argv[]) try
{
    if (argc < 2)
    {
        cerr << "missing playback file argument" << endl;
        return -1;
    }

    const string input_file(argv[1]);

    //create a playback context with file to play
    rs::playback::context context(input_file.c_str());

    if(context.get_device_count() == 0)
    {
        cerr<<"failed to create playback device" << endl;
        return -1;
    }

    //get the playback device
    rs::playback::device* device = context.get_playback_device();

    std::mutex print_mutex;
    auto frame_callback = [&print_mutex](rs::frame frame)
    {
        std::lock_guard<std::mutex> guard(print_mutex);//this mutex is synchronising the prints, it is not mandatory
        std::cout << "stream type: " << frame.get_stream_type() << ", frame number - " << frame.get_frame_number() << std::endl;
    };

    auto motion_callback = [](rs::motion_data motion_data)
    {
        //process motion data here
    };

    //enable available streams and set the frame callbacks
    vector<rs::stream> streams = { rs::stream::color, rs::stream::depth, rs::stream::infrared, rs::stream::infrared2, rs::stream::fisheye };

    for(auto stream : streams)
    {
        if(device->get_stream_mode_count(stream) > 0)
        {
            int width, height, fps;
            rs::format format;
            int streaming_mode_index = 0;
            device->get_stream_mode(stream, streaming_mode_index, width, height, format, fps);
            device->enable_stream(stream, width, height, format, fps);
            device->set_frame_callback(stream, frame_callback);
        }
    }

    if(device->supports(rs::capabilities::motion_events))
        device->enable_motion_tracking(motion_callback);

    //stream until the end of filee
    device->start(rs::source::all_sources);
    while(device->is_streaming())
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    device->stop(rs::source::all_sources);

    return 0;
}

catch(rs::error e)
{
    std::cout << e.what() << std::endl;
    return -1;
}

