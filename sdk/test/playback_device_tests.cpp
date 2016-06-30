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

    static const stream_profile depth_stream_profile = {depth_info, 60};
    static const stream_profile color_stream_profile = {color_info, 30};

    static const std::string file_path = "/tmp/rstest.rssdk";

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

    static void record(rs::device* device)
    {
        int frames = setup::frames;

        device->start();

        while(frames--)
        {
            device->wait_for_frames();
        }
        device->stop();
    }

    static void record_with_motions(rs::device* device)
    {
        try
        {
            auto motion_callback = [](rs::motion_data entry) {};
            auto timestamp_callback = [](rs::timestamp_data entry) {};

            device->enable_motion_tracking(motion_callback, timestamp_callback);

            int frames = setup::frames;

            device->start(rs::source::all_sources);

            while(frames--)
            {
                device->wait_for_frames();
            }
            device->stop(rs::source::all_sources);

            device->disable_motion_tracking();
        }
        catch(const std::exception &ex)
        {
            cerr<<"TEST ERROR : "<< ex.what()<<endl;
        }
        catch(...)
        {
            cerr<<"TEST ERROR : unknown exception in playback test util"<<endl;
        }
    }
}

class playback_fixture : public testing::Test
{
protected:
    std::unique_ptr<rs::playback::context> context;
    rs::playback::device * device;

    static void SetUpTestCase()
    {
        //create a record enabled context with a given output file
        rs::record::context context(setup::file_path);

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

        if(device->supports(rs::capabilities::motion_events))
            playback_tests_util::record_with_motions(device);
        else
            playback_tests_util::record(device);
    }

    static void TearDownTestCase()
    {
        ::remove(setup::file_path.c_str());
    }

    virtual void SetUp()
    {
        //create a playback context with a given input file
        context = std::unique_ptr<rs::playback::context>(new rs::playback::context(setup::file_path));

        //create a playback device
        device = context->get_playback_device();
        ASSERT_NE(nullptr, device);
    }
};

TEST_F(playback_fixture, get_name)
{
    EXPECT_EQ(strcmp(device->get_name(), setup::dinfo.name), 0);
}

TEST_F(playback_fixture, get_serial)
{
    EXPECT_EQ(strcmp(device->get_serial(), setup::dinfo.serial), 0);
}

TEST_F(playback_fixture, get_firmware_version)
{
    EXPECT_EQ(strcmp(device->get_firmware_version(), setup::dinfo.firmware), 0);
}

TEST_F(playback_fixture, get_extrinsics)
{

}

TEST_F(playback_fixture, get_depth_scale)
{
    EXPECT_NEAR(0.001, device->get_depth_scale(), 1e-6);
}

TEST_F(playback_fixture, supports_option)
{
    for(auto op : setup::supported_options)
    {
        EXPECT_TRUE(device->supports_option(op));
    }
}

TEST_F(playback_fixture, get_stream_mode_count)
{
    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        EXPECT_EQ(1, device->get_stream_mode_count(it->first));
    }
}

TEST_F(playback_fixture, get_stream_mode)
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

TEST_F(playback_fixture, enable_stream)
{
    playback_tests_util::enable_streams(device, setup::profiles);
}

TEST_F(playback_fixture, disable_stream)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        auto stream = it->first;
        device->disable_stream(stream);
        EXPECT_FALSE(device->is_stream_enabled(stream));
    }
}

TEST_F(playback_fixture, get_stream_width)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        auto stream = it->first;
        stream_profile sp = it->second;
        EXPECT_EQ(sp.info.width, device->get_stream_width(stream));
    }
}

TEST_F(playback_fixture, get_stream_height)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        auto stream = it->first;
        stream_profile sp = it->second;
        EXPECT_EQ(sp.info.height, device->get_stream_height(stream));
    }
}

TEST_F(playback_fixture, get_stream_format)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        auto stream = it->first;
        stream_profile sp = it->second;
        EXPECT_EQ((rs::format)sp.info.format, device->get_stream_format(stream));
    }
}

TEST_F(playback_fixture, start)
{

}

TEST_F(playback_fixture, stop)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    rs::stream stream = setup::profiles.begin()->first;
    device->start();
    EXPECT_TRUE(device->is_streaming());
    std::this_thread::sleep_for (std::chrono::milliseconds(300));
    device->wait_for_frames();
    EXPECT_GT(device->get_frame_index(stream) ,0);
    device->stop();
    EXPECT_FALSE(device->is_streaming());
    EXPECT_EQ(0, device->get_frame_index(stream));
}

TEST_F(playback_fixture, is_streaming)
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

TEST_F(playback_fixture, poll_for_frames)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    rs::stream stream = setup::profiles.begin()->first;
    EXPECT_FALSE(device->poll_for_frames());
    device->start();
    while(!device->poll_for_frames());
    auto first = device->get_frame_index(stream);
    std::this_thread::sleep_for (std::chrono::milliseconds(200));
    while(!device->poll_for_frames());
    auto second = device->get_frame_index(stream);
    EXPECT_GT(second, first);
    device->stop();
}

TEST_F(playback_fixture, wait_for_frames_safe)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    rs::stream stream = setup::profiles.begin()->first;
    device->start();
    rs::frameset frames = device->wait_for_frames_safe();
    rs::frame frame = frames[stream];
    auto first = frame.get_timestamp();
    std::this_thread::sleep_for (std::chrono::milliseconds(100));
    frames = device->wait_for_frames_safe();
    frame = frames[stream];
    auto second = frame.get_timestamp();
    EXPECT_GT(second, first);
    device->stop();
}

TEST_F(playback_fixture, poll_for_frames_safe)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    rs::stream stream = setup::profiles.begin()->first;
    rs::frameset frames;
    EXPECT_FALSE(device->poll_for_frames_safe(frames));
    device->start();
    while(!device->poll_for_frames_safe(frames));
    rs::frame frame = frames[stream];
    auto first = frame.get_timestamp();
    std::this_thread::sleep_for (std::chrono::milliseconds(200));
    while(!device->poll_for_frames_safe(frames));
    frame = frames[stream];
    auto second = frame.get_timestamp();
    EXPECT_GT(second, first);
    device->stop();
}

TEST_F(playback_fixture, get_frame_timestamp)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    auto max_fps = 0;
    rs::stream stream = rs::stream::color;
    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        stream_profile sp = it->second;
        if(sp.frame_rate > max_fps)
        {
            max_fps = sp.frame_rate;
            stream= it->first;
        }
    }
    stream_profile sp = setup::profiles[stream];
    auto index = device->get_frame_count() - 1;
    device->set_frame_by_index(index, stream);
    auto expected_time = 1000.0 / (double)sp.frame_rate * index;
    auto actual_time = device->get_frame_timestamp(stream);
    auto max_error = 0.05 * expected_time;
    EXPECT_NEAR(expected_time, actual_time, max_error);
}

TEST_F(playback_fixture, get_frame_data)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        auto stream = it->first;
        device->set_frame_by_index(0, stream);
        EXPECT_NE(nullptr, device->get_frame_data(stream));
    }
}

TEST_F(playback_fixture, is_real_time)
{
    device->set_real_time(false);
    EXPECT_FALSE(device->is_real_time());
    device->set_real_time(true);
    EXPECT_TRUE(device->is_real_time());
}

TEST_F(playback_fixture, pause)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    rs::stream stream = setup::profiles.begin()->first;
    device->start();
    std::this_thread::sleep_for (std::chrono::milliseconds(100));
    device->wait_for_frames();
    device->pause();
    auto first = device->get_frame_index(stream);
    std::this_thread::sleep_for (std::chrono::milliseconds(200));
    device->resume();
    device->wait_for_frames();
    auto second = device->get_frame_index(stream);
    EXPECT_NEAR(first, second, 3);
}

TEST_F(playback_fixture, resume)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    auto it = setup::profiles.begin();
    auto stream = it->first;
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

TEST_F(playback_fixture, set_frame_by_index)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    rs::stream stream = setup::profiles.begin()->first;
    auto index = device->get_frame_count() - 1;
    device->set_frame_by_index(index, stream);
    EXPECT_EQ(index, device->get_frame_index(stream));
}

TEST_F(playback_fixture, set_frame_by_timestamp)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    rs::stream stream = setup::profiles.begin()->first;
    auto first_index = 10;
    device->set_frame_by_index(first_index, stream);
    auto ts1 = device->get_frame_timestamp(stream);
    device->set_frame_by_timestamp(ts1 + 1000);
    auto second_index = device->get_frame_index(stream);
    EXPECT_GT(second_index, first_index);
}

TEST_F(playback_fixture, set_real_time)
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

TEST_F(playback_fixture, get_frame_index)
{
    playback_tests_util::enable_streams(device, setup::profiles);
    rs::stream stream = setup::profiles.begin()->first;
    auto index = device->get_frame_count() - 1;
    device->set_frame_by_index(index, stream);
    EXPECT_EQ(index, device->get_frame_index(stream));
}

TEST_F(playback_fixture, DISABLED_get_frame_count)
{
    playback_tests_util::enable_available_streams(device);
    auto frame_count = device->get_frame_count();
    int counter = 0;
    device->start();
    while(device->is_streaming())
    {
        device->wait_for_frames();
        counter++;
    }
    EXPECT_EQ(frame_count, counter);
}

TEST_F(playback_fixture, playback_set_frames)
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

TEST_F(playback_fixture, basic_playback)
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

TEST_F(playback_fixture, get_set_current_index)
{
    auto stream_count = playback_tests_util::enable_available_streams(device);
    auto max_fps = 0;
    rs::stream stream = rs::stream::color;
    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        stream_profile sp = it->second;
        if(sp.frame_rate > max_fps)
        {
            max_fps = sp.frame_rate;
            stream= it->first;
        }
    }
    auto frame_count = device->get_frame_count();
    int first_frame_index = 0.1 * frame_count;
    int second_frame_index = 0.9 * frame_count;
    auto fps = device->get_stream_framerate(stream);
    device->set_frame_by_index(first_frame_index, stream);
    auto first_timestamp = device->get_frame_timestamp(stream);
    device->set_frame_by_index(second_frame_index, stream);
    auto second_timestamp = device->get_frame_timestamp(stream);
    auto expected_time_diff = 1000 / fps * (second_frame_index - first_frame_index);
    auto actual_diff = second_timestamp - first_timestamp;
    auto max_accepted_error = (second_frame_index - first_frame_index) * 0.05 * 1000 / fps; //accept error of up to 3%
    EXPECT_NEAR(expected_time_diff, actual_diff, max_accepted_error);
}

TEST_F(playback_fixture, motions_callback)
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

TEST_F(playback_fixture, frames_callback)
{
    auto stream_count = playback_tests_util::enable_available_streams(device);

    std::map<rs::stream,int> frame_counter;
    int warmup = 2;
    int run_time = setup::frames / max(setup::color_stream_profile.frame_rate, setup::depth_stream_profile.frame_rate) - warmup;
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

    device->set_frame_callback(rs::stream::depth, depth_callback);
    device->set_frame_callback(rs::stream::color, color_callback);

    device->start();
    std::this_thread::sleep_for(std::chrono::seconds(warmup));
    frame_counter.clear();
    std::this_thread::sleep_for(std::chrono::seconds(run_time));
    device->stop();

    EXPECT_GT(frame_counter.size(),0);

    for(auto it = frame_counter.begin(); it != frame_counter.end(); ++it)
    {
        auto stream = it->first;
        auto fps = device->get_stream_framerate(stream);
        auto actual_fps = it->second / run_time;
        auto max_excepted_error = actual_fps * 0.05;
        EXPECT_GT(actual_fps, fps - max_excepted_error);
    }
}

TEST_F(playback_fixture, DISABLED_playback_and_render_callbak)
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
    int run_time = 3;
    auto depth_callback = [&frame_counter, windows](rs::frame f)
    {
        auto stream = rs::stream::depth;
        glfwMakeContextCurrent(windows.at(stream));
        glutils::gl_render(windows.at(stream), f);
        frame_counter[stream]++;
    };
    auto color_callback = [&frame_counter,windows](rs::frame f)
    {
        auto stream = rs::stream::color;
        glfwMakeContextCurrent(windows.at(stream));
        glutils::gl_render(windows.at(stream), f);
        frame_counter[stream]++;
    };

    device->set_frame_callback(rs::stream::depth, depth_callback);
    device->set_frame_callback(rs::stream::color, color_callback);

    device->start();
    std::this_thread::sleep_for(std::chrono::seconds(run_time));
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
