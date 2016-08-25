// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <limits.h>
#include <map>
#include "gtest/gtest.h"
#include "utilities/utilities.h"
#include <thread>

//librealsense api
#include "librealsense/rs.hpp"

#include "rs_sdk.h"


GTEST_TEST(samples_time_sync_tests, basic_time_sync_test)
{
    rs::core::context context;
    ASSERT_NE(context.get_device_count(), 0) << "No camera is connected";

    rs::device * device = context.get_device(0);

    int color_width = 640, color_height = 480, color_fps = 30;
    const rs::format color_format = rs::format::rgb8;
    int depth_width = 628, depth_height = 468, depth_fps = 30;
    const rs::format depth_format = rs::format::z16;
    const rs::format ir_format = rs::format::y16;

    auto run_time = 5;

    device->enable_stream(rs::stream::color, color_width, color_height, color_format, color_fps);
    device->enable_stream(rs::stream::depth, depth_width, depth_height, depth_format, depth_fps);
    device->enable_stream(rs::stream::infrared, depth_width, depth_height, ir_format, depth_fps);

    int streams[static_cast<int>(rs::core::stream_type::max)] = {0};
    int motions[static_cast<int>(rs::core::motion_type::max)] = {0};

    // create arrays of fps valuse for streams and motions
    streams[static_cast<int>(rs::core::stream_type::color)] = color_fps;
    streams[static_cast<int>(rs::core::stream_type::depth)] = depth_fps;
    streams[static_cast<int>(rs::core::stream_type::infrared)] = depth_fps;

    bool keep_accepting = true;

    // create samples_time_sync object
    rs::utils::unique_ptr<rs::utils::samples_time_sync_interface> samples_sync = rs::utils::get_unique_ptr_with_releaser(rs::utils::samples_time_sync_interface::create_instance(streams, motions, device->get_name()));

    int frames_sent = 0;
    int sets_received = 0;

    auto callback = [&samples_sync, &frames_sent, &sets_received, &keep_accepting](rs::frame new_frame)
    {
        using namespace rs::core;
        using namespace rs::utils;
        rs::core::correlated_sample_set correlated_sample;
        auto scoped_sample_set = rs::utils::get_unique_ptr_with_releaser(&correlated_sample);

        if (!keep_accepting)
            return;

        auto  image = get_unique_ptr_with_releaser(image_interface::create_instance_from_librealsense_frame(new_frame,
                                                                           image_interface::flag::any,
                                                                           nullptr));

        image->add_ref();

        auto st = samples_sync->insert(image.get(), correlated_sample);
        frames_sent++;
        if (st)
        {
            sets_received++;


            auto equal = correlated_sample.images[static_cast<int>(rs::core::stream_type::color)]->query_time_stamp() ==
                         correlated_sample.images[static_cast<int>(rs::core::stream_type::depth)]->query_time_stamp();

            equal &= correlated_sample.images[static_cast<int>(rs::core::stream_type::infrared)]->query_time_stamp() ==
                    correlated_sample.images[static_cast<int>(rs::core::stream_type::depth)]->query_time_stamp();

            ASSERT_EQ(equal, true) << "Error - timestamp not equal";
        }


    };

    device->set_frame_callback(rs::stream::color, callback);
    device->set_frame_callback(rs::stream::depth, callback);
    device->set_frame_callback(rs::stream::infrared, callback);

    device->start(rs::source::all_sources);
    std::this_thread::sleep_for(std::chrono::seconds(run_time));

    keep_accepting = false;
    samples_sync->flush();
    device->stop(rs::source::all_sources);

    auto diff = (frames_sent/3)-sets_received;
    if (diff <0) diff=0-diff;
    ASSERT_EQ(diff<10, true) << "Sent: " << frames_sent << " Received: " << sets_received;
}
