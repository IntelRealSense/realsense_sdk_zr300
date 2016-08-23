// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <memory>

#include "gtest/gtest.h"
#include "file_types.h"
#include "rs/utils/fps_counter.h"

using namespace std;
using namespace rs::core;
using namespace rs::utils;
using namespace rs::core::file_types;

namespace fps_tests_setup
{
    const int max_frames_to_stream = 120; // frame limit for requested fps
    const int requested_fps = 60, threshold = 1;

    static const frame_info depth_info = {628, 468, (rs_format)rs::format::z16};
    static const frame_info color_info = {640, 480, (rs_format)rs::format::rgb8};
}

class fps_counter_tests : public testing::Test
{
protected:
    std::unique_ptr<rs::context> m_context;
    rs::device* m_device;

    virtual void SetUp()
    {
        m_context = std::unique_ptr<rs::context>(new rs::context());
        ASSERT_NE(m_context->get_device_count(), 0) << "no camera is connected";
        m_device = m_context->get_device(0);
        ASSERT_NE(m_device, nullptr);
    }
};

TEST_F(fps_counter_tests, fps_color)
{
    fps_counter _fps_counter(fps_tests_setup::requested_fps);

    const int colorFps = fps_tests_setup::requested_fps;
    m_device->enable_stream(rs::stream::color, fps_tests_setup::color_info.width, fps_tests_setup::color_info.height,
                          (rs::format)fps_tests_setup::color_info.format, colorFps);
    ASSERT_NE(m_device->is_stream_enabled(rs::stream::color), false);

    m_device->start();
    int frame_count = 0;
    while(frame_count++ < fps_tests_setup::max_frames_to_stream)
    {
        m_device->wait_for_frames();
        _fps_counter.tick();
    }
    const int current_fps = _fps_counter.current_fps();
    m_device->stop();
    ASSERT_NEAR(current_fps, fps_tests_setup::requested_fps, fps_tests_setup::threshold);
}

TEST_F(fps_counter_tests, fps_depth)
{
    fps_counter _fps_counter(fps_tests_setup::requested_fps);

    const int depthFps = fps_tests_setup::requested_fps;

    m_device->enable_stream(rs::stream::depth, fps_tests_setup::depth_info.width, fps_tests_setup::depth_info.height,
                          (rs::format)fps_tests_setup::depth_info.format, depthFps);
    ASSERT_NE(m_device->is_stream_enabled(rs::stream::depth), false);

    m_device->start();
    int frame_count = 0;
    while(frame_count++ < fps_tests_setup::max_frames_to_stream)
    {
        m_device->wait_for_frames();
        _fps_counter.tick();
    }
    const int current_fps = _fps_counter.current_fps();
    m_device->stop();
    ASSERT_NEAR(current_fps, fps_tests_setup::requested_fps, fps_tests_setup::threshold);
}

TEST_F(fps_counter_tests, comparison_fps)
{
    fps_counter _fps_counter(fps_tests_setup::requested_fps);

    const int colorFps = fps_tests_setup::requested_fps;
    m_device->enable_stream(rs::stream::color, fps_tests_setup::color_info.width, fps_tests_setup::color_info.height,
                          (rs::format)fps_tests_setup::color_info.format, colorFps);
    ASSERT_NE(m_device->is_stream_enabled(rs::stream::color), false);

    m_device->start();
    int frame_count = 0;
    while(frame_count++ < fps_tests_setup::max_frames_to_stream)
    {
        m_device->wait_for_frames();
        _fps_counter.tick();
    }
    const int average_current_diff = std::abs(_fps_counter.total_average_fps() - _fps_counter.current_fps());
    m_device->stop();
    ASSERT_NEAR(average_current_diff, 0, fps_tests_setup::threshold);
}

