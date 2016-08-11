// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <limits.h>
#include <map>
#include "gtest/gtest.h"
#include "utilities/utilities.h"

//librealsense api
#include "librealsense/rs.hpp"
#include "viewer/viewer.h"

using namespace std;

GTEST_TEST(StreamingTests, basic_streaming_sync)
{
    rs::core::context context;
    ASSERT_NE(context.get_device_count(), 0) << "No camera is connected";

    rs::device * device = context.get_device(0);

    int colorWidth = 640, colorHeight = 480, colorFps = 30;
    const rs::format colorFormat = rs::format::rgb8;
    int depthWidth = 628, depthHeight = 468, depthFps = 30;
    const rs::format depthFormat = rs::format::z16;

    auto frame_count = 0;
    auto maxFramesCount = 100;

    device->enable_stream(rs::stream::color, colorWidth, colorHeight, colorFormat, colorFps);
    device->enable_stream(rs::stream::depth, depthWidth, depthHeight, depthFormat, depthFps);

    std::shared_ptr<rs::utils::viewer> viewer = std::make_shared<rs::utils::viewer>(device, 320, "basic_streaming_sync");

    device->start();

    while(frame_count++ < maxFramesCount)
    {
        device->wait_for_frames();
        for(int32_t s = (int32_t)rs::stream::depth; s <= (int32_t)rs::stream::infrared2; ++s)
        {
            auto stream = (rs::stream)s;
            if(device->is_stream_enabled(stream))
            {
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

    auto run_time = 2;

    device->enable_stream(rs::stream::color, color_width, color_height, color_format, color_fps);
    device->enable_stream(rs::stream::depth, depth_width, depth_height, depth_format, depth_fps);


    auto viewer = std::make_shared<rs::utils::viewer>(device, 320, "basic_streaming_callbacks");

    auto callback = [viewer](rs::frame f)
    {
        auto stream = f.get_stream_type();
        viewer->show_frame(std::move(f));
    };

    device->set_frame_callback(rs::stream::color, callback);
    device->set_frame_callback(rs::stream::depth, callback);

    device->start();
    std::this_thread::sleep_for(std::chrono::seconds(run_time));
    device->stop();
}
