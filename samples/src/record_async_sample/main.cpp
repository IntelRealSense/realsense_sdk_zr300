// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <memory>
#include <iostream>
#include <thread>
#include <librealsense/rs.hpp>

#include "rs_sdk.h"

using namespace rs::core;
using namespace std;

void enable_motion_tracking(rs::device * device)
{
    auto motion_callback = [](rs::motion_data motion_data)
    {
        //process motion data here
    };

    device->enable_motion_tracking(motion_callback);

    //set the camera to produce all streams timestamps from a single clock - the microcontroller's clock.
    //this option takes effect only if motion tracking is enabled and device->start() is called with rs::source::all_sources argument.
    device->set_option(rs::option::fisheye_strobe, 1);
}

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
        auto timestamp_domain_str = frame.get_frame_timestamp_domain() == rs::timestamp_domain::camera ? "CAMERA" : "MICROCONTROLLER";
        std::cout << "stream type: " << frame.get_stream_type() << ", frame time domain: " << timestamp_domain_str << ", frame timestamp: " << frame.get_timestamp() << std::endl;
    };

    //enable required streams and set the frame callbacks
    vector<rs::stream> streams = { rs::stream::color, rs::stream::depth, rs::stream::fisheye };

    for(auto stream : streams)
    {
        device->enable_stream(stream, rs::preset::best_quality);
        device->set_frame_callback(stream, frame_callback);
        std::cout << "stream type: " << stream << ", width: " << device->get_stream_width(stream) << ", height: " << device->get_stream_height(stream) << ", format: " << device->get_stream_format(stream) << ", fps: " << device->get_stream_framerate(stream) << std::endl;
    }

    //enable motion tracking, provides IMU events, mandatory for fisheye stream timestamp sync.
    enable_motion_tracking(device);

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
