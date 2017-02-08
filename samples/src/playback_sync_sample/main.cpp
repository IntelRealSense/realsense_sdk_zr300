// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <memory>
#include <iostream>
#include <vector>
#include <librealsense/rs.hpp>
#include "rs_sdk.h"
#include "unistd.h"

using namespace rs::core;
using namespace std;

int main(int argc, char* argv[]) try
{
    if (argc < 2)
    {
        cerr << "missing playback file argument" << endl;
        return -1;
    }
    if (access(argv[1], F_OK) == -1)
    {
        cerr << "playback file does not exists" << endl;
        return -1;
    }
    const string input_file(argv[1]);

    //create a playback enabled context with a given output file
    rs::playback::context context(input_file.c_str());

    //get device count, in playback context there should be a single device.
    //in case device count is 0 there is probably problem with file location or permissions
    int device_count = context.get_device_count();
    if (device_count == 0)
    {
        cerr << "failed to open playback file" << endl;
        return -1;
    }

    //create a playback enabled device
    rs::device* device = context.get_device(0);

    //enable the recorded streams
    vector<rs::stream> streams = { rs::stream::color, rs::stream::depth, rs::stream::infrared, rs::stream::infrared2, rs::stream::fisheye };

    for(auto stream : streams)
    {
        if(device->get_stream_mode_count(stream) > 0)
        {
            device->enable_stream(stream, rs::preset::best_quality);
            std::cout << "stream type: " << stream << ", width: " << device->get_stream_width(stream) << ", height: " << device->get_stream_height(stream) << ", format: " << device->get_stream_format(stream) << ", fps: " << device->get_stream_framerate(stream) << std::endl;
        }
    }

    device->start();

    //if theres no more frames the playback device will report that its not streaming
    while(device->is_streaming())
    {
        device->wait_for_frames();
        for(auto stream : streams)
        {
            if(device->is_stream_enabled(stream))
                std::cout << "stream type: " << stream << ", timestamp: " << device->get_frame_timestamp(stream) << std::endl;
            auto frame_data = device->get_frame_data(stream);

            //use the recorded frame...
        }
    }
    device->stop();

    return 0;
}
catch(const rs::error& e)
{
    std::cout << e.what() << std::endl;
    return 1;
}
catch (const rs::core::exception& e)
{
    std::cerr << "what(): " << e.what() << std::endl;
    std::cerr << "function(): " << e.function() << std::endl;
    return 1;
}
catch (const std::exception& e)
{
    std::cerr << "what(): " << e.what() << std::endl;
    return 1;
}
