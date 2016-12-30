// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <limits.h>
#include <map>
#include "gtest/gtest.h"
#include "utilities/utilities.h"

//librealsense api
#include "librealsense/rs.hpp"
#include "viewer.h"

using namespace std;


GTEST_TEST(StreamingTests, device_details)
{
    rs::core::context context;
    ASSERT_NE(context.get_device_count(), 0) << "No camera is connected";

    rs::device * device = context.get_device(0);
    std::cout << "Device Name : " << device->get_info(rs::camera_info::device_name) << std::endl;
    std::cout << "Serial number : " << device->get_info(rs::camera_info::serial_number) << std::endl;
    std::cout << "Camera Firmware Version : " << device->get_info(rs::camera_info::camera_firmware_version) << std::endl;
    std::cout << "Adapter Board Firmware Version : " << device->get_info(rs::camera_info::adapter_board_firmware_version) << std::endl;
    std::cout << "Motion Module Firmware Version : " << device->get_info(rs::camera_info::motion_module_firmware_version) << std::endl;
}

GTEST_TEST(StreamingTests, basic_streaming_sync)
{
    rs::core::context context;
    ASSERT_NE(context.get_device_count(), 0) << "No camera is connected";

    rs::device * device = context.get_device(0);

    int color_width = 640, color_height = 480, color_fps = 30;
    const rs::format colorFormat = rs::format::rgb8;
    int depth_width = 628, depth_height = 468, depth_fps = 30;
    const rs::format depth_format = rs::format::z16;
    const rs::format ir_format = rs::format::y16;

    auto frame_count = 0;
    auto maxFramesCount = 100;

    device->enable_stream(rs::stream::color, color_width, color_height, colorFormat, color_fps);
    device->enable_stream(rs::stream::depth, depth_width, depth_height, depth_format, depth_fps);
    device->enable_stream(rs::stream::infrared, depth_width, depth_height, ir_format, depth_fps);

    std::shared_ptr<rs::utils::viewer> viewer = std::make_shared<rs::utils::viewer>(3, 320, 240, nullptr, "basic_streaming_sync");

    device->start();

    while(frame_count++ < maxFramesCount)
    {
        device->wait_for_frames();
        for(int32_t s = (int32_t)rs::stream::depth; s <= (int32_t)rs::stream::infrared2; ++s)
        {
            auto stream = (rs::stream)s;
            if(device->is_stream_enabled(stream))
            {
//                size_t zero = 0;
//                EXPECT_GT(device->get_frame_number(stream), zero) << ", stream - " << stream;
                auto image = test_utils::create_image(device, stream);
                viewer->show_image(image);
            }
        }
    }
}

GTEST_TEST(StreamingTests, basic_streaming_callbacks)
{
    rs::core::context context;
    ASSERT_NE(context.get_device_count(), 0) << "No camera is connected";

    rs::device * device = context.get_device(0);

    int color_width = 640, color_height = 480, color_fps = 30;
    const rs::format color_format = rs::format::rgb8;
    int depth_width = 628, depth_height = 468, depth_fps = 30;
    const rs::format depth_format = rs::format::z16;
    const rs::format ir_format = rs::format::y16;

    auto run_time = 2;

    device->enable_stream(rs::stream::color, color_width, color_height, color_format, color_fps);
    device->enable_stream(rs::stream::depth, depth_width, depth_height, depth_format, depth_fps);
    device->enable_stream(rs::stream::infrared, depth_width, depth_height, ir_format, depth_fps);

    auto viewer = std::make_shared<rs::utils::viewer>(3, 320, 240, nullptr, "basic_streaming_callbacks");

    auto callback = [viewer](rs::frame f)
    {
//        size_t zero = 0;
//        EXPECT_GT(f.get_frame_number(), zero) << ", stream - " << f.get_stream_type();
        ASSERT_TRUE(f.supports_frame_metadata(rs_frame_metadata::RS_FRAME_METADATA_ACTUAL_EXPOSURE));
        auto image = rs::utils::get_shared_ptr_with_releaser(rs::core::image_interface::create_instance_from_librealsense_frame(f, rs::core::image_interface::flag::any));
        viewer->show_image(image);
    };

    device->set_frame_callback(rs::stream::color, callback);
    device->set_frame_callback(rs::stream::depth, callback);
    device->set_frame_callback(rs::stream::infrared, callback);

    device->start();
    std::this_thread::sleep_for(std::chrono::seconds(run_time));
    device->stop();
}


GTEST_TEST(StreamingTests, motions_callback)
{
    rs::core::context context;
    ASSERT_NE(context.get_device_count(), 0) << "No camera is connected";

    rs::device * device = context.get_device(0);

    if(!device->supports(rs::capabilities::motion_events))return;
    int color_width = 640, color_height = 480, color_fps = 30;
    const rs::format color_format = rs::format::rgb8;
    int depth_width = 628, depth_height = 468, depth_fps = 30;
    const rs::format depth_format = rs::format::z16;
    const rs::format ir_format = rs::format::y16;

    device->enable_stream(rs::stream::color, color_width, color_height, color_format, color_fps);
    device->enable_stream(rs::stream::depth, depth_width, depth_height, depth_format, depth_fps);
    device->enable_stream(rs::stream::infrared, depth_width, depth_height, ir_format, depth_fps);
    int run_time = 3;
    bool motion_trigerd = false;
    bool timestamp_trigerd = false;
    auto motion_callback = [&motion_trigerd](rs::motion_data entry)
    {
        motion_trigerd = true;
    };

    auto timestamp_callback = [&timestamp_trigerd](rs::timestamp_data entry)
    {
        timestamp_trigerd = true;
    };

    device->enable_motion_tracking(motion_callback, timestamp_callback);

    device->start(rs::source::all_sources);
    std::this_thread::sleep_for(std::chrono::seconds(run_time));
    device->stop(rs::source::all_sources);

    EXPECT_TRUE(motion_trigerd);
    //EXPECT_TRUE(timestamp_trigerd);check expected behaviour!!!
}
