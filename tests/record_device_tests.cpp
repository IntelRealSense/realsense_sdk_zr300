// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <stdio.h>
#include <map>
#include <librealsense/rs.hpp>
#include "gtest/gtest.h"
#include "utilities/utilities.h"
#include "rs/record/record_device.h"
#include "rs/record/record_context.h"
#include "file_types.h"
#include "viewer/viewer.h"

using namespace std;
using namespace rs::core;
using namespace rs::core::file_types;

namespace setup
{
    static const int frames = 100;

    static const frame_info depth_info = {628, 468, (rs_format)rs::format::z16, 640};
    static const frame_info infrared_info = {628, 468, (rs_format)rs::format::y16, 640};
    static const frame_info color_info = {640, 480, (rs_format)rs::format::rgb8, 640};

    static const stream_profile depth_stream_profile = {depth_info, 30};
    static const stream_profile color_stream_profile = {color_info, 30};
    static const stream_profile infrared_stream_profile = {infrared_info, 30};

    static const std::string file_path = "/tmp/rstest.rssdk";

    static std::map<rs::stream, stream_profile> profiles;
}

class record_fixture : public testing::Test
{
protected:
    std::unique_ptr<rs::record::context> m_context;
    std::shared_ptr<rs::utils::viewer> m_viewer;
    rs::record::device * m_device;

    static void TearDownTestCase()
    {
        ::remove(setup::file_path.c_str());
    }

    virtual void SetUp()
    {
        //create a record enabled context with a given output file
        m_context = std::unique_ptr<rs::record::context>(new rs::record::context(setup::file_path.c_str()));

        ASSERT_NE(0, m_context->get_device_count()) << "no device detected";

        //each device created from the record enabled context will write the streaming data to the given file
        m_device = m_context->get_record_device(0);

        ASSERT_NE(nullptr, m_device);

        setup::profiles[rs::stream::depth] = setup::depth_stream_profile;
        setup::profiles[rs::stream::color] = setup::color_stream_profile;
        setup::profiles[rs::stream::infrared] = setup::infrared_stream_profile;
    }
};

TEST_F(record_fixture, wait_for_frames)
{
    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        rs::stream stream = it->first;
        stream_profile sp = it->second;
        m_device->enable_stream(stream, sp.info.width, sp.info.height, (rs::format)sp.info.format, sp.frame_rate);
    }

    m_device->start();
    auto frame_count = 0;
    while(frame_count++ < setup::frames)
    {
        m_device->wait_for_frames();
        for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
        {
            rs::stream stream = it->first;
            EXPECT_NE(nullptr, m_device->get_frame_data(stream));
        }
    }
    m_device->stop();
}

TEST_F(record_fixture, frames_callback)
{
    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        rs::stream stream = it->first;
        stream_profile sp = it->second;
        m_device->enable_stream(stream, sp.info.width, sp.info.height, (rs::format)sp.info.format, sp.frame_rate);
    }

    std::map<rs::stream,int> frame_counter;
    int warmup_time = 3;
    int run_time = 2;
    // Set callbacks prior to calling start()
    auto depth_callback = [&frame_counter](rs::frame f)
    {
        auto stream = rs::stream::depth;
        frame_counter[stream]++;
    };
    auto color_callback = [&frame_counter](rs::frame f)
    {
        auto stream = rs::stream::color;
        frame_counter[stream]++;
    };
    auto infrared_callback = [&frame_counter](rs::frame f)
    {
        auto stream = rs::stream::infrared;
        frame_counter[stream]++;
    };

    m_device->set_frame_callback(rs::stream::depth, depth_callback);
    m_device->set_frame_callback(rs::stream::color, color_callback);
    m_device->set_frame_callback(rs::stream::infrared, infrared_callback);

    // Between start() and stop(), you will receive calls to the specified callbacks from background threads
    m_device->start();
    std::this_thread::sleep_for(std::chrono::seconds(warmup_time));
    frame_counter.clear();
    std::this_thread::sleep_for(std::chrono::seconds(run_time));
    // NOTE: No need to call wait_for_frames() or poll_for_frames(), though they should still work as designed
    m_device->stop();

    for(auto it = frame_counter.begin(); it != frame_counter.end(); ++it)
    {
        auto stream = it->first;
        auto fps = m_device->get_stream_framerate(stream);
        auto actual_fps = it->second / run_time;
        auto max_excepted_error = actual_fps * 0.05;
        EXPECT_NEAR(fps, actual_fps, max_excepted_error);
    }
}

TEST_F(record_fixture, motions_callback)
{
    if(!m_device->supports(rs::capabilities::motion_events))return;
    int run_time = 2;
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

    m_device->enable_motion_tracking(motion_callback, timestamp_callback);

    m_device->start(rs::source::motion_data);
    std::this_thread::sleep_for(std::chrono::seconds(run_time));
    m_device->stop(rs::source::motion_data);

    EXPECT_TRUE(motion_trigerd);
    //EXPECT_TRUE(timestamp_trigerd);check expected behaviour!!!
}

TEST_F(record_fixture, all_sources_callback)
{
    if(!m_device->supports(rs::capabilities::motion_events))return;

    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        rs::stream stream = it->first;
        stream_profile sp = it->second;
        m_device->enable_stream(stream, sp.info.width, sp.info.height, (rs::format)sp.info.format, sp.frame_rate);
    }

    std::map<rs::stream,int> frame_counter;
    int warmup_time = 3;
    int run_time = 2;
    bool motion_trigerd = false;
    bool timestamp_trigerd = false;

    auto depth_callback = [&frame_counter](rs::frame f)
    {
        auto stream = rs::stream::depth;
        frame_counter[stream]++;
    };
    auto color_callback = [&frame_counter](rs::frame f)
    {
        auto stream = rs::stream::color;
        frame_counter[stream]++;
    };
    auto infrared_callback = [&frame_counter](rs::frame f)
    {
        auto stream = rs::stream::infrared;
        frame_counter[stream]++;
    };
    auto motion_callback = [&motion_trigerd](rs::motion_data entry)
    {
        motion_trigerd = true;
    };

    auto timestamp_callback = [&timestamp_trigerd](rs::timestamp_data entry)
    {
        timestamp_trigerd = true;
    };

    m_device->set_frame_callback(rs::stream::depth, depth_callback);
    m_device->set_frame_callback(rs::stream::color, color_callback);
    m_device->set_frame_callback(rs::stream::infrared, infrared_callback);
    m_device->enable_motion_tracking(motion_callback, timestamp_callback);

    m_device->start(rs::source::all_sources);
    std::this_thread::sleep_for(std::chrono::seconds(warmup_time));
    frame_counter.clear();
    std::this_thread::sleep_for(std::chrono::seconds(run_time));
    m_device->stop(rs::source::all_sources);

    EXPECT_TRUE(motion_trigerd);
    EXPECT_TRUE(timestamp_trigerd);
    for(auto it = frame_counter.begin(); it != frame_counter.end(); ++it)
    {
        auto stream = it->first;
        auto fps = m_device->get_stream_framerate(stream);
        auto actual_fps = it->second / run_time;
        auto max_excepted_error = actual_fps * 0.05;
        EXPECT_NEAR(fps, actual_fps, max_excepted_error);
    }
}

TEST_F(record_fixture, record_and_render)
{
    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        rs::stream stream = it->first;
        stream_profile sp = it->second;
        m_device->enable_stream(stream, sp.info.width, sp.info.height, (rs::format)sp.info.format, sp.frame_rate);
    }

    m_viewer = std::make_shared<rs::utils::viewer>(m_device, 320, "record_and_render");
    std::map<rs::stream,int> frame_counter;
    int run_time = 3;    // Set callbacks prior to calling start()
    auto callback = [&frame_counter, this](rs::frame frame)
    {
        frame_counter[frame.get_stream_type()]++;
        m_viewer->show_frame(std::move(frame));
    };

    m_device->set_frame_callback(rs::stream::depth, callback);
    m_device->set_frame_callback(rs::stream::color, callback);
    m_device->set_frame_callback(rs::stream::infrared, callback);

    m_device->start();
    std::this_thread::sleep_for(std::chrono::seconds(run_time));
    m_device->stop();
}
