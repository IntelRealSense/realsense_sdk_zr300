// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <memory>
#include <iostream>
#include <vector>
#include <thread>
#include <librealsense/rs.hpp>
#include "rs_sdk.h"

using namespace rs::core;
using namespace std;

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        cerr << "missing playback file argument" << endl;
        return -1;
    }

    const string output_file(argv[1]);

    //create a playback context with file to play
    rs::playback::context context(output_file.c_str());

    if(context.get_device_count() == 0)
    {
        cerr<<"failed to create playback device" << endl;
        return -1;
    }

    //get the playback device
    rs::playback::device* device = context.get_playback_device();

    auto frame_callback = [](rs::frame frame)
    {
        std::cout << "stream type: " << frame.get_stream_type() << ", frame number - " << frame.get_frame_number() << std::endl;
    };

    auto motion_callback = [](rs::motion_data motion_data)
    {
        //process motion data here
    };

    auto timestamp_callback = [](rs::timestamp_data timestamp_data)
    {
        //process timestamp data here
    };

    //enable available streams and set the frame callbacks
    for(int32_t s = (int32_t)rs::stream::depth; s <= (int32_t)rs::stream::fisheye; ++s)
    {
        int width, height, fps;
        rs::format format = rs::format::any;
        auto stream = (rs::stream)s;
        if(device->get_stream_mode_count(stream) == 0)continue;
        device->get_stream_mode(stream, 0, width, height, format, fps);
        device->enable_stream(stream, width, height, format, fps);
        device->set_frame_callback(stream, frame_callback);
    }

    if(device->supports(rs::capabilities::motion_events))
        device->enable_motion_tracking(motion_callback, timestamp_callback);

    // the following scenario will start playback for one second, then will pause the playback (not the streaming)
    // for one second and resume playbacking for one more second
    device->start(rs::source::all_sources);
    while(device->is_streaming())
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    device->stop(rs::source::all_sources);

    return 0;
}

