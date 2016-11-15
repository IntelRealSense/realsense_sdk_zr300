// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <memory>
#include <iostream>
#include <thread>
#include <librealsense/rs.hpp>

#include "rs_sdk.h"

using namespace rs::core;
using namespace std;

int main(int argc, char* argv[]) try
{
    if (argc < 2)
    {
        cerr << "missing record file argument" << endl;
        return -1;
    }

    const string output_file(argv[1]);

    //create a record enabled context with a given output file
    rs::record::context context(output_file.c_str());

    if(context.get_device_count() == 0)
    {
        cerr<<"no device detected" << endl;
        return -1;
    }

    //each device created from the record enabled context will write the streaming data to the given file
    rs::record::device* device = context.get_record_device(0);

    auto frame_callback = [](rs::frame frame)
    {
        std::cout << "stream type: " << frame.get_stream_type() << ", frame number - " << frame.get_frame_number() << std::endl;
    };

    auto motion_callback = [](rs::motion_data motion_data)
    {
        //process motion data here
    };

    //enable required streams and set the frame callbacks
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

    device->enable_motion_tracking(motion_callback);

    // the following scenario will start record for one second, then will pause the record (not the streaming)
    // for one second and resume recording for one more second
    device->start(rs::source::all_sources);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    device->pause_record();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    device->resume_record();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    device->stop(rs::source::all_sources);

    return 0;
}

catch(rs::error e)
{
    std::cout << e.what() << std::endl;
    return -1;
}
