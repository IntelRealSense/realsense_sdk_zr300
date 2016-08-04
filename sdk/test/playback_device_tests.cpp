// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <stdio.h>
#include <map>
#include "gtest/gtest.h"
#include "utilities/utilities.h"
#include "rs/playback/playback_device.h"
#include "rs/playback/playback_context.h"
#include "rs/record/record_context.h"
#include "librealsense/rs.hpp"
#include "file_types.h"
#include "rs/utils/librealsense_conversion_utils.h"
#include "image/image_utils.h"

using namespace std;
using namespace rs::core;
using namespace rs::core::file_types;

namespace setup
{
    static const int frames = 200;

    static const frame_info depth_info = {628, 468, (rs_format)rs::format::z16, 640};
    static const frame_info color_info = {640, 480, (rs_format)rs::format::rgb8, 640};

    static const stream_profile depth_stream_profile = {depth_info, 30};
    static const stream_profile color_stream_profile = {color_info, 30};

    static const std::string file_wait_for_frames = "/tmp/rstest_wait_for_frames.rssdk";
    static const std::string file_callbacks = "/tmp/rstest_callbacks.rssdk";

    static std::vector<rs::option> supported_options;
    static std::map<rs::stream, stream_profile> profiles;
    static rs::core::device_info dinfo;
}

namespace playback_tests_util
{
    int enable_available_streams(rs::device* device)
    {
        int stream_count = 0;
        for(int32_t s = (int32_t)rs::stream::depth; s <= (int32_t)rs::stream::infrared2; ++s)
        {
            int width, height, fps;
            rs::format format = rs::format::any;
            auto stream = (rs::stream)s;
            if(device->get_stream_mode_count(stream) == 0)continue;
            device->get_stream_mode(stream, 0, width, height, format, fps);
            if(format != rs::format::any)
            {
                stream_count++;
                device->enable_stream(stream, width, height, format, fps);
            }
        }
        return stream_count;
    }
    static void enable_streams(rs::device* device, const std::map<rs::stream, stream_profile>& profiles)
    {
        for(auto it = profiles.begin(); it != profiles.end(); ++it)
        {
            auto stream = it->first;
            int width, height, fps;
            rs::format format;
            device->get_stream_mode(stream, 0, width, height, format, fps);
            device->enable_stream(stream, width, height, format, fps);
            EXPECT_TRUE(device->is_stream_enabled(stream));
        }
    }

    static void record_callback_no_motion(rs::device* device)
    {
        std::map<rs::stream,uint32_t> frame_count;

        for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
        {
            rs::stream stream = it->first;
            device->set_frame_callback(stream, [stream,&frame_count](rs::frame entry) {frame_count[stream]++;});
        }

        device->start();
        uint32_t streaming = 0;
        while(streaming < setup::profiles.size())
        {
            streaming = 0;
            for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
            {
                rs::stream stream = it->first;
                if(frame_count[stream] < setup::frames) continue;
                streaming++;
            }
            std::this_thread::sleep_for (std::chrono::milliseconds(5));
        }
        device->stop();
    }

    static void record_callback_with_motion(rs::device* device)
    {
        auto motion_callback = [](rs::motion_data entry) {};
        auto timestamp_callback = [](rs::timestamp_data entry) {};
        std::map<rs::stream,uint32_t> frame_count;

        device->enable_motion_tracking(motion_callback, timestamp_callback);
        for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
        {
            rs::stream stream = it->first;
            device->set_frame_callback(stream, [stream,&frame_count](rs::frame entry) {frame_count[stream]++;});
        }

        device->start(rs::source::all_sources);
        ASSERT_TRUE(device->is_motion_tracking_active());

        uint32_t streaming = 0;
        while(streaming < setup::profiles.size())
        {
            streaming = 0;
            for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
            {
                rs::stream stream = it->first;
                if(frame_count[stream] < setup::frames) continue;
                streaming++;
            }
            std::this_thread::sleep_for (std::chrono::milliseconds(5));
        }
        device->stop(rs::source::all_sources);

        device->disable_motion_tracking();
    }

    static void record_wait_for_frames_no_motion(rs::device* device)
    {
        int frames = setup::frames;

        device->start();

        while(frames--)
        {
            device->wait_for_frames();
        }
        device->stop();
    }

    static void record_wait_for_frames_with_motion(rs::device* device)
    {
        auto motion_callback = [](rs::motion_data entry) {};
        auto timestamp_callback = [](rs::timestamp_data entry) {};

        device->enable_motion_tracking(motion_callback, timestamp_callback);
        int frames = setup::frames;

        device->start(rs::source::all_sources);
        ASSERT_TRUE(device->is_motion_tracking_active());

        while(frames--)
        {
            device->wait_for_frames();
        }
        device->stop(rs::source::all_sources);

        device->disable_motion_tracking();
    }

    static void record_wait_for_frames_no_motion(std::string file_path)
    {
        //create a record enabled context with a given output file
        rs::record::context context(file_path);

        ASSERT_NE(0, context.get_device_count()) << "no device detected";
        //each device created from the record enabled context will write the streaming data to the given file
        rs::device* device = context.get_device(0);

        memset(&setup::dinfo, 0, sizeof(setup::dinfo));
        strcpy(setup::dinfo.name, device->get_name());
        strcpy(setup::dinfo.serial, device->get_serial());
        strcpy(setup::dinfo.firmware, device->get_firmware_version());

        for(int32_t option = (int32_t)rs::option::color_backlight_compensation; option <= (int32_t)rs::option::r200_depth_control_lr_threshold; option++)
        {
            if(device->supports_option((rs::option)option))
                setup::supported_options.push_back((rs::option)option);
        }

        setup::profiles[rs::stream::depth] = setup::depth_stream_profile;
        setup::profiles[rs::stream::color] = setup::color_stream_profile;

        for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
        {
            rs::stream stream = it->first;
            stream_profile sp = it->second;
            device->enable_stream(stream, sp.info.width, sp.info.height, (rs::format)sp.info.format, sp.frame_rate);
        }
        if(file_path == setup::file_wait_for_frames)
        {
            if(device->supports(rs::capabilities::motion_events))
                playback_tests_util::record_wait_for_frames_with_motion(device);
            else
                playback_tests_util::record_wait_for_frames_no_motion(device);
        }
        if(file_path == setup::file_callbacks)
        {
            if(device->supports(rs::capabilities::motion_events))
                playback_tests_util::record_callback_with_motion(device);
            else
                playback_tests_util::record_callback_no_motion(device);
        }
    }
}

class playback_streaming_fixture : public testing::TestWithParam<std::string>
{
protected:
    std::unique_ptr<rs::playback::context> context;
    rs::playback::device * device;
public:
    static void SetUpTestCase()
    {
        playback_tests_util::record_wait_for_frames_no_motion(setup::file_callbacks);
        playback_tests_util::record_wait_for_frames_no_motion(setup::file_wait_for_frames);
    }

    static void TearDownTestCase()
    {
        ::remove(setup::file_callbacks.c_str());
        ::remove(setup::file_wait_for_frames.c_str());
    }

    virtual void SetUp()
    {
        //create a playback context with a given input file
        context = std::unique_ptr<rs::playback::context>(new rs::playback::context(GetParam()));

        //create a playback device
        device = context->get_playback_device();
        ASSERT_NE(nullptr, device);
    }
};

TEST_P(playback_streaming_fixture, get_name)
{
    EXPECT_EQ(strcmp(device->get_name(), setup::dinfo.name), 0);
}

TEST_P(playback_streaming_fixture, get_serial)
{
    EXPECT_EQ(strcmp(device->get_serial(), setup::dinfo.serial), 0);
}

TEST_P(playback_streaming_fixture, get_firmware_version)
{
    EXPECT_EQ(strcmp(device->get_firmware_version(), setup::dinfo.firmware), 0);
}

TEST_P(playback_streaming_fixture, get_extrinsics)
{
    device->enable_stream(rs::stream::color, rs::preset::best_quality);
    device->enable_stream(rs::stream::depth, rs::preset::best_quality);
    auto ext1 = device->get_extrinsics(rs::stream::color, rs::stream::depth);
    auto ext2 = device->get_extrinsics(rs::stream::depth, rs::stream::color);
    EXPECT_GT(ext1.translation[0], 0);
    EXPECT_LT(ext2.translation[0], 0);
    EXPECT_NEAR(ext1.translation[0], -ext2.translation[0], 0.001);
}

TEST_P(playback_streaming_fixture, get_depth_scale)
{
    EXPECT_NEAR(0.001, device->get_depth_scale(), 1e-6);
}

TEST_P(playback_streaming_fixture, supports_option)
{
    for(auto op : setup::supported_options)
    {
        EXPECT_TRUE(device->supports_option(op));
    }
}

TEST_P(playback_streaming_fixture, get_stream_mode_count)
{
    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        EXPECT_EQ(1, device->get_stream_mode_count(it->first));
    }
}

TEST_P(playback_streaming_fixture, get_stream_mode)
{
    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        auto stream = it->first;
        stream_profile sp = it->second;
        int width, height, fps;
        rs::format format;
        device->get_stream_mode(stream, 0, width, height, format, fps);
        EXPECT_EQ(sp.info.width, width);
        EXPECT_EQ(sp.info.height, height);
        EXPECT_EQ((rs::format)sp.info.format, format);
        EXPECT_EQ(sp.frame_rate, fps);
    }
}

TEST_P(playback_streaming_fixture, enable_stream)
{
    playback_tests_util::enable_streams(device, setup::profiles);
}

TEST_P(playback_streaming_fixture, disable_stream)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        auto stream = it->first;
        device->disable_stream(stream);
        EXPECT_FALSE(device->is_stream_enabled(stream));
    }
}

TEST_P(playback_streaming_fixture, get_stream_width)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        auto stream = it->first;
        stream_profile sp = it->second;
        EXPECT_EQ(sp.info.width, device->get_stream_width(stream));
    }
}

TEST_P(playback_streaming_fixture, get_stream_height)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        auto stream = it->first;
        stream_profile sp = it->second;
        EXPECT_EQ(sp.info.height, device->get_stream_height(stream));
    }
}

TEST_P(playback_streaming_fixture, get_stream_format)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        auto stream = it->first;
        stream_profile sp = it->second;
        EXPECT_EQ((rs::format)sp.info.format, device->get_stream_format(stream));
    }
}

TEST_P(playback_streaming_fixture, start_stop_stress)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    for(int i = 0; i < 100; i++)
    {
        device->start();
        EXPECT_TRUE(device->is_streaming());
        device->pause();
        EXPECT_FALSE(device->is_streaming());
        device->resume();
        EXPECT_TRUE(device->is_streaming());
        device->stop();
        EXPECT_FALSE(device->is_streaming());
        device->resume();
        EXPECT_TRUE(device->is_streaming());
        device->pause();
        EXPECT_FALSE(device->is_streaming());
        device->stop();
        EXPECT_FALSE(device->is_streaming());
    }
}

TEST_P(playback_streaming_fixture, stop)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    auto it = setup::profiles.begin();
    ASSERT_NE(it, setup::profiles.end());
    rs::stream stream = it->first;
    device->start();
    EXPECT_TRUE(device->is_streaming());
    std::this_thread::sleep_for (std::chrono::milliseconds(800));
    device->wait_for_frames();
    auto first = device->get_frame_index(stream);
    device->stop();
    EXPECT_FALSE(device->is_streaming());
    device->start();
    EXPECT_TRUE(device->is_streaming());
    std::this_thread::sleep_for (std::chrono::milliseconds(700));
    device->wait_for_frames();
    auto second = device->get_frame_index(stream);
    device->stop();
    EXPECT_GT(first, second);
}

TEST_P(playback_streaming_fixture, is_streaming)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    device->start();
    EXPECT_TRUE(device->is_streaming());
    device->stop();
    EXPECT_FALSE(device->is_streaming());
    device->start();
    EXPECT_TRUE(device->is_streaming());
    device->pause();
    EXPECT_FALSE(device->is_streaming());
    device->resume();
    EXPECT_TRUE(device->is_streaming());
    device->stop();
}

TEST_P(playback_streaming_fixture, poll_for_frames)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    auto it = setup::profiles.begin();
    ASSERT_NE(it, setup::profiles.end());
    rs::stream stream = it->first;
    EXPECT_FALSE(device->poll_for_frames());
    device->start();
    while(!device->poll_for_frames() && device->is_streaming())
        std::this_thread::sleep_for (std::chrono::milliseconds(5));
    auto first = device->get_frame_index(stream);
    std::this_thread::sleep_for (std::chrono::milliseconds(200));
    while(!device->poll_for_frames() && device->is_streaming())
        std::this_thread::sleep_for (std::chrono::milliseconds(5));
    auto second = device->get_frame_index(stream);
    EXPECT_GT(second, first);
    device->stop();
}

TEST_P(playback_streaming_fixture, get_frame_timestamp)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    auto expected_fps = 0;
    rs::stream stream = rs::stream::color;
    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        stream_profile sp = it->second;
        if(sp.frame_rate > expected_fps)
        {
            expected_fps = sp.frame_rate;
            stream= it->first;
        }
    }
    auto mid_index = device->get_frame_count() / 2.0;
    device->set_frame_by_index(mid_index, stream);
    auto mid_time = device->get_frame_timestamp(stream);
    auto last_index = device->get_frame_count() - 1;
    device->set_frame_by_index(last_index, stream);
    auto last_time = device->get_frame_timestamp(stream);
    auto max_error = 0.05 * expected_fps;
    double frame_count = last_index - mid_index;
    double duration_in_second = (double)(last_time - mid_time) * 0.001;
    auto actual_fps = frame_count / duration_in_second;
    EXPECT_NEAR(expected_fps, actual_fps, max_error);
}

TEST_P(playback_streaming_fixture, get_frame_data)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        auto stream = it->first;
        device->set_frame_by_index(0, stream);
        EXPECT_NE(nullptr, device->get_frame_data(stream));
    }
}

TEST_P(playback_streaming_fixture, is_real_time)
{
    device->set_real_time(false);
    EXPECT_FALSE(device->is_real_time());
    device->set_real_time(true);
    EXPECT_TRUE(device->is_real_time());
}

TEST_P(playback_streaming_fixture, non_real_time_playback)
{
    playback_tests_util::enable_streams(device, setup::profiles);

    device->set_real_time(false);
    EXPECT_FALSE(device->is_real_time());
    auto it = setup::profiles.begin();
    int prev = 0;
    device->start();
    for(int i = 0; i < 10; i++)
    {
       device->wait_for_frames();
       std::this_thread::sleep_for (std::chrono::milliseconds(100));
       auto frame_number = device->get_frame_number(it->first);
       if(prev != 0)
       {
           EXPECT_EQ(prev + 1, frame_number);
       }
       prev = frame_number;
    }
    device->stop();
}

TEST_P(playback_streaming_fixture, pause)
{
    auto it = setup::profiles.begin();
    ASSERT_NE(it, setup::profiles.end());
    rs::stream stream = it->first;
    device->enable_stream(stream, rs::preset::best_quality);
    device->start();
    std::this_thread::sleep_for (std::chrono::milliseconds(300));
    device->wait_for_frames();
    device->pause();
    auto first = device->get_frame_index(stream);
    std::this_thread::sleep_for (std::chrono::milliseconds(500));
    device->resume();
    device->wait_for_frames();
    auto second = device->get_frame_index(stream);
    EXPECT_NEAR(first, second, 2);
}

TEST_P(playback_streaming_fixture, resume)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    auto it = setup::profiles.begin();
    ASSERT_NE(it, setup::profiles.end());
    rs::stream stream = it->first;
    device->start();
    std::this_thread::sleep_for (std::chrono::milliseconds(200));
    device->wait_for_frames();
    device->pause();
    auto first = device->get_frame_timestamp(stream);
    device->resume();
    std::this_thread::sleep_for (std::chrono::milliseconds(200));
    device->wait_for_frames();
    auto second = device->get_frame_timestamp(stream);
    EXPECT_GT(second, first);
}

TEST_P(playback_streaming_fixture, set_frame_by_index)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    auto it = setup::profiles.begin();
    ASSERT_NE(it, setup::profiles.end());
    rs::stream stream = it->first;
    auto index = device->get_frame_count() - 1;
    device->set_frame_by_index(index, stream);
    EXPECT_EQ(index, device->get_frame_index(stream));
}

TEST_P(playback_streaming_fixture, DISABLED_set_frame_by_timestamp)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    auto it = setup::profiles.begin();
    ASSERT_NE(it, setup::profiles.end());
    rs::stream stream = it->first;
    auto first_index = 100;
    device->set_frame_by_index(first_index, stream);
    auto ts1 = device->get_frame_timestamp(stream);
    device->set_frame_by_timestamp(ts1 + 100);
    auto second_index = device->get_frame_index(stream);
    EXPECT_GT(second_index, first_index);
}

TEST_P(playback_streaming_fixture, set_real_time)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    auto t1 = std::chrono::system_clock::now();
    device->set_real_time(true);
    device->start();
    while(device->is_streaming())
    {
        device->wait_for_frames();
    }
    device->stop();
    auto t2 = std::chrono::system_clock::now();
    device->set_real_time(false);
    device->start();
    while(device->is_streaming())
    {
        device->wait_for_frames();
    }
    device->stop();
    auto t3 = std::chrono::system_clock::now();
    auto diff1 = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    auto diff2 = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count();
    EXPECT_GT(diff1, diff2 * 10);
}

TEST_P(playback_streaming_fixture, get_frame_index)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    auto it = setup::profiles.begin();
    ASSERT_NE(it, setup::profiles.end());
    rs::stream stream = it->first;
    auto index = device->get_frame_count() - 1;
    device->set_frame_by_index(index, stream);
    EXPECT_EQ(index, device->get_frame_index(stream));
}

TEST_P(playback_streaming_fixture, get_frame_count)
{
    auto stream_count = playback_tests_util::enable_available_streams(device);

    std::map<rs::stream,int> frame_counter;
    auto callback = [&frame_counter](rs::frame f)
    {
        auto stream = f.get_stream_type();
        frame_counter[stream]++;
    };

    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        auto stream = it->first;
        device->set_frame_callback(stream, callback);
    }
    device->set_real_time(false);
    device->start();
    while(device->is_streaming())
        std::this_thread::sleep_for(std::chrono::seconds(1));
    device->stop();

    for(auto it = frame_counter.begin(); it != frame_counter.end(); ++it)
    {
        auto stream = it->first;
        auto expected_frame_count = device->get_frame_count(stream);
        auto actual_frame_count = it->second;
        EXPECT_EQ(expected_frame_count, actual_frame_count);
    }
}

TEST_P(playback_streaming_fixture, playback_set_frames)
{
    auto stream_count = playback_tests_util::enable_available_streams(device);

    std::map<rs::stream, GLFWwindow *> windows;
    for(int32_t s = (int32_t)rs::stream::depth; s <= (int32_t)rs::stream::infrared2; ++s)
    {
        auto stream = (rs::stream)s;
        if(device->is_stream_enabled(stream))
        {
            glfwInit();
            windows[stream] = (glfwCreateWindow(device->get_stream_width(stream), device->get_stream_height(stream), "basic playback test", nullptr, nullptr));
        }
    }

    auto frame_count = device->get_frame_count();
    int counter = 0;
    while(counter< frame_count)
    {
        device->set_frame_by_index(counter++, rs::stream::color);
        for(int32_t s = (int32_t)rs::stream::depth; s <= (int32_t)rs::stream::infrared2; ++s)
        {
            auto stream = (rs::stream)s;
            if(device->is_stream_enabled(stream))
            {
                glfwMakeContextCurrent(windows.at(stream));
                glutils::gl_render(windows.at(stream), device, (rs::stream)s);
            }
        }
        //std::this_thread::sleep_for (std::chrono::milliseconds(33));
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

TEST_P(playback_streaming_fixture, basic_playback)
{
    auto stream_count = playback_tests_util::enable_available_streams(device);

    std::map<rs::stream, GLFWwindow *> windows;
    for(int32_t s = (int32_t)rs::stream::depth; s <= (int32_t)rs::stream::infrared2; ++s)
    {
        auto stream = (rs::stream)s;
        if(device->is_stream_enabled(stream))
        {
            glfwInit();
            windows[stream] = (glfwCreateWindow(device->get_stream_width(stream), device->get_stream_height(stream), "basic playback test", nullptr, nullptr));
        }
    }

    device->start();
    auto counter = 0;
    while(device->is_streaming())
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
        counter++;
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

TEST_P(playback_streaming_fixture, motions_callback)
{
    if(!device->supports(rs::capabilities::motion_events))return;
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

TEST_P(playback_streaming_fixture, frames_callback)
{
    auto stream_count = playback_tests_util::enable_available_streams(device);

    std::map<rs::stream,unsigned int> frame_counter;
    int warmup = 2;
    int run_time = setup::frames / max(setup::color_stream_profile.frame_rate, setup::depth_stream_profile.frame_rate) - warmup;
    auto callback = [&frame_counter](rs::frame f)
    {
        auto stream = f.get_stream_type();
        frame_counter[stream]++;
    };

    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        auto stream = it->first;
        device->set_frame_callback(stream, callback);
    }

    device->start();
    std::this_thread::sleep_for(std::chrono::seconds(warmup));
    frame_counter.clear();
    std::this_thread::sleep_for(std::chrono::seconds(run_time));
    device->stop();

    EXPECT_GT((int)frame_counter.size(),(int)0);

    for(auto it = frame_counter.begin(); it != frame_counter.end(); ++it)
    {
        auto stream = it->first;
        auto fps = device->get_stream_framerate(stream);
        auto actual_fps = it->second / run_time;
        auto max_excepted_error = actual_fps * 0.05;
        EXPECT_GT(actual_fps, fps - max_excepted_error);
    }
}

TEST_P(playback_streaming_fixture, playback_and_render_callbak)
{
    auto stream_count = playback_tests_util::enable_available_streams(device);
    std::map<rs::stream, GLFWwindow *> windows;
    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        auto stream = it->first;
        if(device->is_stream_enabled(stream))
        {
            glfwInit();
            windows[stream] = (glfwCreateWindow(device->get_stream_width(stream), device->get_stream_height(stream), "basic record test", nullptr, nullptr));
        }
    }

    std::map<rs::stream,int> frame_counter;
    auto callback = [&frame_counter, windows](rs::frame f)
    {
        auto stream = f.get_stream_type();
        glfwMakeContextCurrent(windows.at(stream));
        glutils::gl_render(windows.at(stream), f);
        frame_counter[stream]++;
    };

    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        auto stream = it->first;
        device->set_frame_callback(stream, callback);
    }

    device->start();
    while(device->is_streaming())
        std::this_thread::sleep_for(std::chrono::seconds(1));
    device->stop();

    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        auto stream = it->first;
        if(device->is_stream_enabled(stream))
        {
            glutils::gl_close(windows.at(stream));
        }
    }
}

INSTANTIATE_TEST_CASE_P(playback_tests, playback_streaming_fixture, ::testing::Values(
                            setup::file_callbacks,
                            setup::file_wait_for_frames
                        ));
