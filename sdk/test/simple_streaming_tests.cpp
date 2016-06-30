// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <limits.h>
#include "gtest/gtest.h"
#include "utilities/utilities.h"

//librealsense api
#include "librealsense/rs.hpp"
#include <map>

//opengl api
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

using namespace std;

GTEST_TEST(StreamingTests, basicColorStreaming)
{
    rs::context contex;
    ASSERT_NE(contex.get_device_count(), 0) << "No camera is connected";

    rs::device * device = contex.get_device(0);

    int colorWidth = 640, colorHeight = 480, colorFps = 30;
    const rs::format colorFormat = rs::format::rgb8;
    int depthWidth = 628, depthHeight = 468, depthFps = 30;
    const rs::format depthFormat = rs::format::z16;

    auto frame_count = 0;
    auto maxFramesCount = 100;

    device->enable_stream(rs::stream::color, colorWidth, colorHeight, colorFormat, colorFps);
    device->enable_stream(rs::stream::depth, depthWidth, depthHeight, depthFormat, depthFps);
    std::map<rs::stream, GLFWwindow *> windows;
    for(int32_t s = (int32_t)rs::stream::depth; s <= (int32_t)rs::stream::infrared2; ++s)
    {
        auto stream = (rs::stream)s;
        if(device->is_stream_enabled(stream))
        {
            glfwInit();
            auto width = device->get_stream_width(stream);
            auto height = device->get_stream_height(stream);
            windows[stream] = (glfwCreateWindow(width, height, "basic playback test", nullptr, nullptr));
        }
    }

    device->start();

    while(frame_count++ < maxFramesCount)
    {
        device->wait_for_frames();
        for(int32_t s = (int32_t)rs::stream::depth; s <= (int32_t)rs::stream::infrared2; ++s)
        {
            auto stream = (rs::stream)s;
            if(device->is_stream_enabled(stream))
            {
                glfwMakeContextCurrent(windows.at(stream));
                glutils::gl_render(windows.at(stream), device, (rs::stream)s);
            }
        }
    }

    for(int32_t s = (int32_t)rs::stream::depth; s <= (int32_t)rs::stream::infrared2; ++s)
    {
        auto stream = (rs::stream)s;
        if(device->is_stream_enabled(stream))
        {
            glutils::gl_close(windows.at(stream));
        }
    }
}
