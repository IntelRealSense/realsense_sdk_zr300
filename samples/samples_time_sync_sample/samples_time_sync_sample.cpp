// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <memory>
#include <iostream>
#include <librealsense/rs.hpp>

#include "rs_sdk.h"
#include <unistd.h>
#include <thread>
#include <memory>

#include "rs/utils/cyclic_array.h"
#include "rs/utils/smart_ptr_helpers.h"

using namespace rs::core;
using namespace rs::utils;
using namespace std;

void frame_handler(rs::frame new_frame);

void motion_handler(rs::motion_data data);


rs::utils::unique_ptr<samples_time_sync_interface> samples_sync = nullptr;


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
    int colorWidth = 640, colorHeight = 480, colorFps = 60;
    const rs::format colorFormat = rs::format::rgb8;

    //depth profile
    int depthWidth = 628, depthHeight = 468, depthFps = 60;
    const rs::format depthFormat = rs::format::z16;

    // Enable three color/depth streams
    device->enable_stream(rs::stream::color, colorWidth, colorHeight, colorFormat, colorFps);
    device->enable_stream(rs::stream::depth, depthWidth, depthHeight, depthFormat, depthFps);
    device->enable_stream(rs::stream::fisheye, colorWidth, colorHeight, rs::format::raw8, 30);

    // set callbacks for color/depth streams
    device->set_frame_callback(rs::stream::color, frame_handler);
    device->set_frame_callback(rs::stream::depth, frame_handler);
    device->set_frame_callback(rs::stream::fisheye, frame_handler);

    int streams[static_cast<int>(stream_type::max)] = {0};
    int motions[static_cast<int>(motion_type::max)] = {0};

    // create arrays of fps valuse for streams and motions
    streams[static_cast<int>(stream_type::color)] = 60;
    streams[static_cast<int>(stream_type::depth)] = 60;
    streams[static_cast<int>(stream_type::fisheye)] = 30;

    motions[static_cast<int>(motion_type::accel)] = 200;
    motions[static_cast<int>(motion_type::gyro)] = 200;

    // create samples_time_sync object
    samples_sync = rs::utils::get_unique_ptr_with_releaser(
                         rs::utils::samples_time_sync_interface::create_instance(streams, motions, device->get_name()));

    // enable motion in device
    device->enable_motion_tracking(motion_handler);

    // add strobe for fisheye - in order to get correlated samples with depth stream
    device->set_option(rs::option::fisheye_strobe, 1);

    device->start(rs::source::all_sources);

    std::this_thread::sleep_for(std::chrono::seconds(3));

    device->stop(rs::source::all_sources);

    samples_sync->flush();

    return 0;
}


//process_sample own the correlated_sample_set, it will release the samples when it will be out of scope
void process_sample(correlated_sample_set * sample)
{
    auto scoped_samples = get_unique_ptr_with_releaser(sample);

    static int correlated_frame_counter=0;

    std::cout << "Received correlated set "<< correlated_frame_counter++ << " of synced frames. Color TS: " << scoped_samples->images[static_cast<uint8_t>(stream_type::color)]->query_time_stamp() <<
              " Depth TS: " << scoped_samples->images[static_cast<uint8_t>(stream_type::depth)]->query_time_stamp() <<
              " Gyro TS: "<< scoped_samples->motion_samples[(int)motion_type::gyro].timestamp <<
              " Accel TS: "<< scoped_samples->motion_samples[(int)motion_type::accel].timestamp <<
              " Fisheye TS: " << scoped_samples->images[static_cast<uint8_t>(stream_type::fisheye)]->query_time_stamp() << endl;
}

void frame_handler(rs::frame new_frame)
{

    if (!samples_sync)
        return;

    // create an image from received data
    image_info  info;
    info.width = new_frame.get_width();
    info.height = new_frame.get_height();
    info.format = rs::utils::convert_pixel_format(new_frame.get_format());
    info.pitch = new_frame.get_stride();

    auto st_type  = rs::utils::convert_stream_type(new_frame.get_stream_type());

    std::cout << "Received video frame " << new_frame.get_frame_number() << " , type " << static_cast<int>(st_type) << " with timestamp " << new_frame.get_timestamp()
              << " TS Domain: " << static_cast<int>(new_frame.get_frame_timestamp_domain()) << endl;

    auto  image = get_unique_ptr_with_releaser(image_interface::create_instance_from_raw_data(
                                                                       &info,
                                                                       {new_frame.get_data(), nullptr},
                                                                       st_type,
                                                                       image_interface::flag::any,
                                                                       new_frame.get_timestamp(),
                                                                       new_frame.get_frame_number(),
                                                                       nullptr));


    //create a container for correlated sample set
    correlated_sample_set sample = {};

    //image unique ptr will decrement the ref count at the end of the scope, using add ref when inserting it to the samples sync utility
    image->add_ref();

    // push the image to the sample time sync
    auto st = samples_sync->insert(image.get(), sample);

    // time sync may return correlated sample set - check the status
    if (!st)
        return;

    // correlated sample set found - print it
    process_sample(&sample);
}


void motion_handler(rs::motion_data data)
{
    motion_sample new_sample;

    if (!samples_sync)
        return;

    // create a motion sample from data
    int i=0;
    while (i<3)
    {
        new_sample.data[i] = data.axes[i];
        i++;
    }

    new_sample.timestamp = data.timestamp_data.timestamp;
    new_sample.type = (motion_type)(data.timestamp_data.source_id == RS_EVENT_IMU_ACCEL ? 1 : 2);

    //create a container for correlated sample set
    correlated_sample_set sample = {};

    // push the motion to the sample time sync
    auto st = samples_sync->insert(new_sample, sample);

    // time sync may return correlated sample set - check the status
    if (!st)
        return;

    // correlated sample set found - print it
    process_sample(&sample);
}
