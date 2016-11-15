// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <memory>
#include <iostream>
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

    const int number_of_frames = 200;
    const string output_file(argv[1]);

    //create a record enabled context with a given output file
    rs::record::context context(output_file.c_str());

    if(context.get_device_count() == 0)
    {
        cerr<<"no device detected" << endl;
        return -1;
    }

    //each device created from the record enabled context will write the streaming data to the given file
    rs::device* device = context.get_device(0);


    //enable required streams
    device->enable_stream(rs::stream::color, 640, 480, rs::format::rgba8, 30);
    device->enable_stream(rs::stream::depth, 640, 480, rs::format::z16, 30);
    device->enable_stream(rs::stream::fisheye, 640, 480, rs::format::raw8, 30);

    vector<rs::stream> streams = { rs::stream::color, rs::stream::depth, rs::stream::infrared, rs::stream::infrared2, rs::stream::fisheye };

    device->start();
    for(auto i = 0; i < number_of_frames; ++i)
    {
        //each available frame will be written to the output file
        device->wait_for_frames();
        for(auto stream : streams)
        {
            if(device->is_stream_enabled(stream))
                std::cout << "stream type: " << stream << ", frame number - " << device->get_frame_number(stream) << std::endl;
        }
    }
    device->stop();

    return 0;
}

catch(rs::error e)
{
    std::cout << e.what() << std::endl;
    return -1;
}
