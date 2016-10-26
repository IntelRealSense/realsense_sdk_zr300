// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

// Frames Per Second Counting Sample
// This sample demonstrates an application usage of an fps counter class which implements frames per second counting.
// The class provides an easy way to count frames per second value both per second and as an overall average.

#include <memory>
#include <iostream>
#include <librealsense/rs.hpp>

#include "rs_sdk.h"

using namespace rs::core;
using namespace rs::utils;
using namespace std;

const int count_stream_fps(rs::device* device, int requested_from_stream_fps);

int main(int argc, char* argv[])
{
    rs::context context;
    if(context.get_device_count() == 0)
    {
        cerr << "no device detected" << endl;
        return -1;
    }
    rs::device* device = context.get_device(0);

    const int default_fps = 60;

    /* calculate fps for first stream */
    //color profile
    const int colorWidth = 640, colorHeight = 480, colorFps = default_fps;
    const rs::format colorFormat = rs::format::rgb8;
    device->enable_stream(rs::stream::color, colorWidth, colorHeight, colorFormat, colorFps);
    device->start();

    /* fps measurement */
    int average_fps = count_stream_fps(device, default_fps); // counting color stream fps
    std::cout << "Color stream: average fps = " << average_fps << std::endl;

    device->stop();
    device->disable_stream(rs::stream::color); // disable first tested stream in order to have more precise result

    /* calculate fps for second stream */
    //depth profile
    const int depthWidth = 640, depthHeight = 480, depthFps = default_fps;
    const rs::format depthFormat = rs::format::z16;
    device->enable_stream(rs::stream::depth, depthWidth, depthHeight, depthFormat, depthFps);
    device->start();

    /* fps measurement */
    average_fps = count_stream_fps(device, default_fps); // counting depth stream fps
    std::cout << "Depth stream: average fps = " << average_fps << std::endl;

    device->stop();

    return 0;
}

const int count_stream_fps(rs::device* device, int requested_from_stream_fps)
{
    fps_counter _fps_counter(requested_from_stream_fps); // create fps_counter
    int frames_to_stream = 300; // approx. 5 seconds for requested 60 fps
    while(device->is_streaming() && frames_to_stream--)
    {
        device->wait_for_frames(); // stream frames

        _fps_counter.tick(); // store time value

        if (frames_to_stream % 100 == 0) // print fps
            std::cout << "Last second fps = " << _fps_counter.current_fps() << std::endl;
    }
    return _fps_counter.total_average_fps(); // return total average fps
}
