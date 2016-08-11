// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <memory>
#include <iostream>
#include <librealsense/rs.hpp>

#include "rs_sdk.h"

using namespace rs::core;
using namespace std;

int main(int argc, char* argv[])
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

    device->enable_stream(rs::stream::depth, 480, 360, rs::format::z16,  60);
    device->enable_stream(rs::stream::color, 640, 480, rs::format::rgb8, 60);

    device->start();
    for(auto i = 0; i < number_of_frames; ++i)
    {
        //each available frame will be written to the output file
        device->wait_for_frames();
    }
    device->stop();

    return 0;
}
