// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <memory>
#include <iostream>
#include <librealsense/rs.hpp>

#include "rs_sdk.h"
#include <unistd.h>

#include "rs/utils/cyclic_array.h"

using namespace rs::core;
using namespace std;

void frame_handler(rs::frame new_frame);

rs::utils::sync_utility* sync_util = nullptr;

int frame_counter=0;
int correlated_frame_counter=0;


int main(int argc, char* argv[])
{
    rs::context context;
    if(context.get_device_count() == 0)
    {
        cerr<<"no device detected" << endl;
        return -1;
    }

    rs::device* device = context.get_device(0);

    //color profile
    int colorWidth = 640, colorHeight = 480, colorFps = 30, color_pixel_size = 3;
    const rs::format colorFormat = rs::format::rgb8;

    //depth profile
    int depthWidth = 628, depthHeight = 468, depthFps = 30, depth_pixel_size = 2;
    const rs::format depthFormat = rs::format::z16;

    device->enable_stream(rs::stream::color, colorWidth, colorHeight, colorFormat, colorFps);
    device->enable_stream(rs::stream::depth, depthWidth, depthHeight, depthFormat, depthFps);

    device->set_frame_callback(rs::stream::color, frame_handler);
    device->set_frame_callback(rs::stream::depth, frame_handler);

    vector<pair<stream_type, unsigned int>> streams;
    vector<pair<motion_type, unsigned int>> motions;

    streams.push_back(make_pair(stream_type::color, 60));
    streams.push_back(make_pair(stream_type::depth, 60));
    sync_util = new rs::utils::sync_utility(streams, motions);

    device->start();

    sleep(5);

    delete sync_util;

    return 0;
}

void frame_handler(rs::frame new_frame)
{

    frame_counter++;

    image_info  info;
    info.width = new_frame.get_width();
    info.height = new_frame.get_height();
    info.format = rs::utils::convert_pixel_format(new_frame.get_format());
    info.pitch = (info.format ==  pixel_format::rgb8 ? 3 : 2) * info.width;

    std::cout << "Received "<<frame_counter << " frame of type " << (int)info.format << " with timestamp " << new_frame.get_frame_number() << endl;

    auto st_type  = (info.format ==  pixel_format::rgb8 ? stream_type::color : stream_type::depth );
    auto  image = new custom_image(&info,
                                   new_frame.get_data(),
                                   st_type,
                                   image_interface::flag::any,
                                   new_frame.get_timestamp(),
                                   new_frame.get_frame_number(),
                                   nullptr, nullptr);

    correlated_sample_set sample;

    rs::utils::smart_ptr<image_interface> image_ptr(image);

    auto st = sync_util->insert(image_ptr, sample);

    if (!st)
        return;

    std::cout << "Received correlated set "<< correlated_frame_counter++ << " of synced frames. Color TS: " << sample.images[static_cast<uint8_t>(stream_type::color)]->query_time_stamp() <<
              " Depth TS: " << sample.images[static_cast<uint8_t>(stream_type::depth)]->query_time_stamp() << endl;

    return;
}
