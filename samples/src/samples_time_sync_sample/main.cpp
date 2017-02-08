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
std::mutex sync_mutex;

int main(int argc, char* argv[]) try
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
    streams[static_cast<int>(stream_type::color)] = colorFps;
    streams[static_cast<int>(stream_type::depth)] = depthFps;
    streams[static_cast<int>(stream_type::fisheye)] = 30;

    motions[static_cast<int>(motion_type::accel)] = 200;
    motions[static_cast<int>(motion_type::gyro)] = 200;

    // create samples_time_sync object
    samples_sync = rs::utils::get_unique_ptr_with_releaser(
                         rs::utils::samples_time_sync_interface::create_instance(streams, motions, device->get_name()));

    // enable motion in device
    device->enable_motion_tracking(motion_handler);

    // set the camera to produce all streams timestamps from a single clock - the microcontroller's clock.
    // this option takes effect only if motion tracking is enabled and device->start() is called with rs::source::all_sources argument.
    device->set_option(rs::option::fisheye_strobe, 1);

    device->start(rs::source::all_sources);

    std::this_thread::sleep_for(std::chrono::seconds(3));

    {
        std::lock_guard<mutex> lock(sync_mutex);
        samples_sync->flush();
        samples_sync.reset();
    }

    device->stop(rs::source::all_sources);

    return 0;
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


// Note: In this sample processing correlated frames is done on libRealSense callback thread
//       It is recommended not to execute any heavy processing on callback's thread. This may cause frame drop.
void process_sample(const correlated_sample_set& sample)
{
    static int correlated_frame_counter=0;

    std::cout << "Received correlated set "<< correlated_frame_counter++ << " of synced frames. Color TS: " << sample[stream_type::color]->query_time_stamp() <<
              " Depth TS: " << sample[stream_type::depth]->query_time_stamp() <<
              " Gyro TS: "<< sample[motion_type::gyro].timestamp <<
              " Accel TS: "<< sample[motion_type::accel].timestamp <<
              " Fisheye TS: " << sample[stream_type::fisheye]->query_time_stamp() << endl;

    sample[stream_type::fisheye]->release();
    sample[stream_type::depth]->release();
    sample[stream_type::color]->release();
}

void frame_handler(rs::frame new_frame)
{

    std::lock_guard<std::mutex> lock(sync_mutex);

    if (!samples_sync)
        return;

    auto st_type  = rs::utils::convert_stream_type(new_frame.get_stream_type());

    std::cout << "Received video frame " << new_frame.get_frame_number() << " , type " << static_cast<int>(st_type) << " with timestamp " << new_frame.get_timestamp()
              << " TS Domain: " << static_cast<int>(new_frame.get_frame_timestamp_domain()) << endl;

    auto  image = get_unique_ptr_with_releaser(image_interface::create_instance_from_librealsense_frame(
                                                                       new_frame,
                                                                       image_interface::flag::any
                                                                       ));


    //create a container for correlated sample set
    correlated_sample_set sample = {};

    // push the image to the sample time sync
    auto st = samples_sync->insert(image.get(), sample);

    // time sync may return correlated sample set - check the status
    if (!st)
        return;

    // correlated sample set found - print it
    process_sample(sample);
}


void motion_handler(rs::motion_data data)
{
    motion_sample new_sample;

    std::lock_guard<std::mutex> lock(sync_mutex);

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
    new_sample.frame_number = data.timestamp_data.frame_number;

    //create a container for correlated sample set
    correlated_sample_set sample = {};

    // push the motion to the sample time sync
    auto st = samples_sync->insert(new_sample, sample);

    // time sync may return correlated sample set - check the status
    if (!st)
        return;

    // correlated sample set found - print it
    process_sample(sample);
}
