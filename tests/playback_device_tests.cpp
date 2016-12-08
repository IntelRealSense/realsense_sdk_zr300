// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <stdio.h>
#include <map>
#include <chrono>
#include <thread>
#include <mutex>
#include "gtest/gtest.h"
#include "rs/playback/playback_device.h"
#include "rs/playback/playback_context.h"
#include "rs/record/record_context.h"
#include "librealsense/rs.hpp"
#include "file_types.h"
#include "rs/utils/librealsense_conversion_utils.h"
#include "image/image_utils.h"
#include "viewer.h"
#include "utilities/utilities.h"
#include "include/rs_sdk_version.h"

using namespace std;
using namespace rs::core;
using namespace rs::core::file_types;

namespace setup
{
    static const int frames = 200;

    static const frame_info depth_info = {628, 468, (rs_format)rs::format::z16, 640};
    static const frame_info color_info = {640, 480, (rs_format)rs::format::rgb8, 640};
    static const frame_info ir_info = {628, 468, (rs_format)rs::format::y16, 640};
    static const frame_info fisheye_info = {640, 480, (rs_format)rs::format::raw8, 640};

    static const stream_profile depth_stream_profile = {depth_info, 30};
    static const stream_profile color_stream_profile = {color_info, 30};
    static const stream_profile ir_stream_profile = {ir_info, 30};
    static const stream_profile fisheye_stream_profile = {fisheye_info, 30};

    static const std::string file_wait_for_frames = "rstest_wait_for_frames.rssdk";
    static const std::string file_callbacks = "rstest_callbacks.rssdk";

    static std::map<rs::camera_info, std::string> supported_camera_info;
    static std::vector<rs::camera_info> unsupported_camera_info;
    static std::vector<rs::option> supported_options;
    static std::vector<rs::extrinsics> stream_extrinsics;
    static std::map<rs::stream, rs::extrinsics> motion_extrinsics;
    static rs::motion_intrinsics motion_intrinsics;
    static std::map<rs::stream, stream_profile> profiles;
    static rs::core::device_info dinfo;
    static std::vector<rs::motion_data> motion_events_sync;
    static std::vector<rs::timestamp_data> time_stamps_events_sync;
    static std::vector<uint64_t> motion_time_stamps_sync;
    static uint64_t first_motion_sample_delay_sync = 0;
    static std::vector<uint64_t> motion_time_stamps_async;
    static uint64_t first_motion_sample_delay_async = 0;
    static std::vector<rs::motion_data> motion_events_async;
    static std::vector<rs::timestamp_data> time_stamps_events_async;
    static std::vector<rs::timestamp_domain> time_stamps_domain;
}

namespace playback_tests_util
{
    int enable_available_streams(rs::device* device)
    {
        int stream_count = 0;
        for(int32_t s = (int32_t)rs::stream::depth; s <= (int32_t)rs::stream::fisheye; ++s)
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

    static void record_callback_no_motion(rs::device* device)
    {
        std::map<rs::stream,uint32_t> frame_count;

        std::mutex mutex;

        for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
        {
            rs::stream stream = it->first;
            device->set_frame_callback(stream, [stream,&frame_count, &mutex](rs::frame entry)
            {
                std::lock_guard<std::mutex> guard(mutex);
                setup::time_stamps_domain.push_back(entry.get_frame_timestamp_domain());
                frame_count[stream]++;
            });
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
        std::chrono::high_resolution_clock::time_point begin;
        auto motion_callback = [&begin](rs::motion_data entry)
        {
            auto now = std::chrono::high_resolution_clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::microseconds>(now - begin).count();
            if(setup::motion_time_stamps_async.size() == 0)
                setup::first_motion_sample_delay_async = diff;
            setup::motion_time_stamps_async.push_back(diff);
            setup::motion_events_async.push_back(entry);

        };
        auto timestamp_callback = [](rs::timestamp_data entry)
        {
            setup::time_stamps_events_async.push_back(entry);
        };
        std::map<rs::stream,uint32_t> frame_count;

        device->enable_motion_tracking(motion_callback, timestamp_callback);

        std::mutex mutex;

        for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
        {
            rs::stream stream = it->first;
            device->set_frame_callback(stream, [stream,&frame_count,&mutex](rs::frame entry)
            {
                std::lock_guard<std::mutex> guard(mutex);
                setup::time_stamps_domain.push_back(entry.get_frame_timestamp_domain());
                frame_count[stream]++;
            });
            try
            {
                auto ext = device->get_motion_extrinsics_from(stream);
                setup::motion_extrinsics[stream] = ext;            }
            catch(...)
            {
            }
        }

        try
        {
            setup::motion_intrinsics = device->get_motion_intrinsics();
        }
        catch(...)
        {
            rs::motion_intrinsics intr = {};
            setup::motion_intrinsics = intr;
        }
        begin = std::chrono::high_resolution_clock::now();

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

    static void record_wait_for_frames_with_motion(rs::device* device)
    {
        std::chrono::high_resolution_clock::time_point begin;
        auto motion_callback = [&begin](rs::motion_data entry)
        {
            auto now = std::chrono::high_resolution_clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::microseconds>(now - begin).count();
            if(setup::motion_time_stamps_sync.size() == 0)
                setup::first_motion_sample_delay_sync = diff;
            setup::motion_time_stamps_sync.push_back(diff);
            setup::motion_events_sync.push_back(entry);
        };
        auto timestamp_callback = [](rs::timestamp_data entry)
        {
            setup::time_stamps_events_sync.push_back(entry);
        };

        device->enable_motion_tracking(motion_callback, timestamp_callback);
        int frames = setup::frames;

        for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
        {
            rs::stream stream = it->first;
            try
            {
                auto ext = device->get_motion_extrinsics_from(stream);
                setup::motion_extrinsics[stream] = ext;
            }
            catch(...)
            {

            }
        }

        try
        {
            setup::motion_intrinsics = device->get_motion_intrinsics();
        }
        catch(...)
        {
            rs::motion_intrinsics intr = {};
            setup::motion_intrinsics = intr;
        }

        begin = std::chrono::high_resolution_clock::now();

        device->start(rs::source::all_sources);
        ASSERT_TRUE(device->is_motion_tracking_active());

        while(frames--)
        {
            device->wait_for_frames();
        }
        device->stop(rs::source::all_sources);

        device->disable_motion_tracking();
    }

    static void record(std::string file_path)
    {
        //create a record enabled context with a given output file
        rs::record::context context(file_path.c_str());

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
        int ci;
        for(ci = (int)rs::camera_info::device_name; ci <= (int)rs::camera_info::third_lens_nominal_baseline; ci++)
        {
            rs::camera_info cam_info = static_cast<rs::camera_info>(ci);
            if(device->supports(cam_info))
            {
                setup::supported_camera_info[cam_info] = device->get_info(cam_info);
            }else
            {
                 setup::unsupported_camera_info.push_back(cam_info);
            }
        }
        EXPECT_TRUE(ci == rs_camera_info::RS_CAMERA_INFO_COUNT);
        setup::profiles[rs::stream::depth] = setup::depth_stream_profile;
        setup::profiles[rs::stream::color] = setup::color_stream_profile;
        setup::profiles[rs::stream::infrared] = setup::ir_stream_profile;
        setup::profiles[rs::stream::fisheye] = setup::fisheye_stream_profile;

        for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
        {
            rs::stream stream = it->first;
            stream_profile sp = it->second;
            device->enable_stream(stream, sp.info.width, sp.info.height, (rs::format)sp.info.format, sp.frame_rate);
        }

        if(setup::stream_extrinsics.size() == 0)
        {
            for(auto it1 = setup::profiles.begin(); it1 != setup::profiles.end(); ++it1)
            {
                for(auto it2 = setup::profiles.begin(); it2 != setup::profiles.end(); ++it2)
                {
                    setup::stream_extrinsics.push_back(device->get_extrinsics(it1->first, it2->first));
                }
            }
        }

        device->set_option(rs::option::fisheye_strobe, 1);

        if(file_path == setup::file_wait_for_frames)
        {
            if(device->supports(rs::capabilities::motion_events))
                playback_tests_util::record_wait_for_frames_with_motion(device);
            else
                playback_tests_util::record(device);
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
        playback_tests_util::record(setup::file_callbacks);
        playback_tests_util::record(setup::file_wait_for_frames);
    }

    static void TearDownTestCase()
    {
        ::remove(setup::file_callbacks.c_str());
        ::remove(setup::file_wait_for_frames.c_str());
    }

    virtual void SetUp()
    {
        //create a playback context with a given input file
        context = std::unique_ptr<rs::playback::context>(new rs::playback::context(GetParam().c_str()));

        //create a playback device
        device = context->get_playback_device();
        ASSERT_NE(nullptr, device);
    }
};

TEST_P(playback_streaming_fixture, get_file_info)
{
    rs::playback::file_info file_info = device->get_file_info();
    std::stringstream lrs_version;
    lrs_version << RS_API_MAJOR_VERSION << "." << RS_API_MINOR_VERSION << "." << RS_API_PATCH_VERSION;
    EXPECT_EQ(0, lrs_version.str().compare(file_info.librealsense_version));
    std::stringstream sdk_version;
    sdk_version << SDK_VER_MAJOR << "." << SDK_VER_MINOR << "." << SDK_VER_PATCH;
    EXPECT_EQ(0, sdk_version.str().compare(file_info.sdk_version));
    EXPECT_EQ(2, file_info.version);
    EXPECT_EQ(rs::playback::file_format::rs_linux_format, file_info.type);
    if(0 == strcmp(GetParam().c_str(), setup::file_callbacks.c_str()))
    {
        EXPECT_EQ(rs::playback::capture_mode::asynced, file_info.capture_mode);
    }
    if(0 == strcmp(GetParam().c_str(), setup::file_wait_for_frames.c_str()))
    {
        EXPECT_EQ(rs::playback::capture_mode::synced, file_info.capture_mode);
    }
}

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
    auto stream_count = playback_tests_util::enable_available_streams(device);
    std::vector<rs::extrinsics> pb_extrinsics;
    for(auto it1 = setup::profiles.begin(); it1 != setup::profiles.end(); ++it1)
    {
        for(auto it2 = setup::profiles.begin(); it2 != setup::profiles.end(); ++it2)
        {
            pb_extrinsics.push_back(device->get_extrinsics(it1->first, it2->first));
        }
    }
    ASSERT_EQ(setup::stream_extrinsics.size(), pb_extrinsics.size());
    ASSERT_GT(setup::stream_extrinsics.size(), 0u);
    for(uint32_t i = 0; i < pb_extrinsics.size(); i++)
    {
        auto rec_ext = setup::stream_extrinsics[i];
        auto pb_ext = pb_extrinsics[i];
        for(int j = 0; j < 9; j++)
        {
            EXPECT_NEAR(rec_ext.rotation[j], pb_ext.rotation[j], 1e-6);
        }
        for(int j = 0; j < 3; j++)
        {
            EXPECT_NEAR(rec_ext.translation[j], pb_ext.translation[j], 1e-6);
        }
    }
}

TEST_P(playback_streaming_fixture, get_motion_extrinsics_from)
{
    auto stream_count = playback_tests_util::enable_available_streams(device);

    for(auto it = setup::motion_extrinsics.begin(); it != setup::motion_extrinsics.end(); ++it)
    {
        rs::stream stream = it->first;
        rs::extrinsics ext =  device->get_motion_extrinsics_from(stream);
        for(int i = 0; i < 9; i++)
        {
            EXPECT_EQ(setup::motion_extrinsics[stream].rotation[i], ext.rotation[i]);
        }
        for(int i = 0; i < 3; i++)
        {
            EXPECT_EQ(setup::motion_extrinsics[stream].translation[i], ext.translation[i]);
        }
    }
}

TEST_P(playback_streaming_fixture, get_motion_intrinsics)
{
    auto stream_count = playback_tests_util::enable_available_streams(device);
    rs::motion_intrinsics motion_intrinsics =  device->get_motion_intrinsics();
    auto size = sizeof(setup::motion_intrinsics.acc.bias_variances) / sizeof(setup::motion_intrinsics.acc.bias_variances[0]);
    for(size_t i = 0; i < size; i++)
    {
        EXPECT_NEAR(setup::motion_intrinsics.acc.bias_variances[i], motion_intrinsics.acc.bias_variances[i], 0.0001);
        EXPECT_NEAR(setup::motion_intrinsics.gyro.bias_variances[i], motion_intrinsics.gyro.bias_variances[i], 0.0001);
    }
    size = sizeof(setup::motion_intrinsics.acc.noise_variances) / sizeof(setup::motion_intrinsics.acc.noise_variances[0]);
    for(size_t i = 0; i < size; i++)
    {
        EXPECT_NEAR(setup::motion_intrinsics.acc.noise_variances[i], motion_intrinsics.acc.noise_variances[i], 0.0001);
        EXPECT_NEAR(setup::motion_intrinsics.gyro.noise_variances[i], motion_intrinsics.gyro.noise_variances[i], 0.0001);
    }
    size = sizeof(setup::motion_intrinsics.acc.data) / sizeof(setup::motion_intrinsics.acc.data[0]);
    for(size_t i = 0; i < size; i++)
    {
        auto inter_size = sizeof(setup::motion_intrinsics.acc.data[0]) / sizeof(setup::motion_intrinsics.acc.data[0][0]);
        for(size_t j = 0; j < inter_size; j++)
        {
            EXPECT_NEAR(setup::motion_intrinsics.acc.data[i][j], motion_intrinsics.acc.data[i][j], 0.0001);
            EXPECT_NEAR(setup::motion_intrinsics.gyro.data[i][j], motion_intrinsics.gyro.data[i][j], 0.0001);
        }
    }
}

TEST_P(playback_streaming_fixture, get_depth_scale)
{
    EXPECT_NEAR(0.001, device->get_depth_scale(), 1e-6);
}

TEST_P(playback_streaming_fixture, supports_option)
{
    for(auto op : setup::supported_options)
    {
        EXPECT_TRUE(device->supports_option(op)) << "Device should support option " << op;
    }
}

TEST_P(playback_streaming_fixture, supports_camera_info)
{
    for(const auto& cam_info_pair : setup::supported_camera_info)
    {
        rs::camera_info ci = cam_info_pair.first;
        const char* recorded_info = cam_info_pair.second.c_str();
        bool supports_cam_info = device->supports(ci);
        EXPECT_TRUE(supports_cam_info) << "camera_info of type " << static_cast<int32_t>(ci) << "is not supported";
        if(supports_cam_info)
        {
            const char* playback_info = device->get_info(ci);
            EXPECT_STREQ(playback_info, recorded_info)
                    << "Different info for camera_info of type " << static_cast<int32_t>(ci);
        }
    }
    for(rs::camera_info unsupported_id : setup::unsupported_camera_info)
    {
        EXPECT_FALSE(device->supports(unsupported_id));
        ASSERT_THROW(device->get_info(unsupported_id), std::runtime_error);
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
    auto stream_count = playback_tests_util::enable_available_streams(device);
}

TEST_P(playback_streaming_fixture, disable_stream)
{
    auto stream_count = playback_tests_util::enable_available_streams(device);
    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        auto stream = it->first;
        device->disable_stream(stream);
        EXPECT_FALSE(device->is_stream_enabled(stream));
    }
}

TEST_P(playback_streaming_fixture, get_stream_width)
{
    auto stream_count = playback_tests_util::enable_available_streams(device);
    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        auto stream = it->first;
        stream_profile sp = it->second;
        EXPECT_EQ(sp.info.width, device->get_stream_width(stream));
    }
}

TEST_P(playback_streaming_fixture, get_stream_height)
{
    auto stream_count = playback_tests_util::enable_available_streams(device);
    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        auto stream = it->first;
        stream_profile sp = it->second;
        EXPECT_EQ(sp.info.height, device->get_stream_height(stream));
    }
}

TEST_P(playback_streaming_fixture, get_stream_format)
{
    auto stream_count = playback_tests_util::enable_available_streams(device);
    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        auto stream = it->first;
        stream_profile sp = it->second;
        EXPECT_EQ((rs::format)sp.info.format, device->get_stream_format(stream));
    }
}

TEST_P(playback_streaming_fixture, start_stop_stress)
{
    auto stream_count = playback_tests_util::enable_available_streams(device);
    for(int i = 0; i < 100; i++)
    {
        device->start();
        EXPECT_TRUE(device->is_streaming());
        device->pause();
        EXPECT_FALSE(device->is_streaming());
        device->start();
        EXPECT_TRUE(device->is_streaming());
        device->stop();
        EXPECT_FALSE(device->is_streaming());
        device->start();
        EXPECT_TRUE(device->is_streaming());
        device->pause();
        EXPECT_FALSE(device->is_streaming());
        device->stop();
        EXPECT_FALSE(device->is_streaming());
    }
}

TEST_P(playback_streaming_fixture, stop)
{
    //prevent from runnimg async file with wait for frames
    rs::playback::file_info file_info = device->get_file_info();
    if(file_info.capture_mode == rs::playback::capture_mode::asynced) return;

    auto stream_count = playback_tests_util::enable_available_streams(device);
    auto it = setup::profiles.begin();
    ASSERT_NE(it, setup::profiles.end());
    rs::stream stream = it->first;
    device->start();
    EXPECT_TRUE(device->is_streaming());
    std::this_thread::sleep_for (std::chrono::milliseconds(1000));
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
    auto stream_count = playback_tests_util::enable_available_streams(device);
    device->start();
    EXPECT_TRUE(device->is_streaming());
    device->stop();
    EXPECT_FALSE(device->is_streaming());
    device->start();
    EXPECT_TRUE(device->is_streaming());
    device->pause();
    EXPECT_FALSE(device->is_streaming());
    device->start();
    EXPECT_TRUE(device->is_streaming());
    device->stop();
}

TEST_P(playback_streaming_fixture, poll_for_frames)
{
    //prevent from runnimg async file with wait for frames
    rs::playback::file_info file_info = device->get_file_info();
    if(file_info.capture_mode == rs::playback::capture_mode::asynced) return;

    auto stream_count = playback_tests_util::enable_available_streams(device);
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
    auto stream_count = playback_tests_util::enable_available_streams(device);
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
    device->set_frame_by_index(static_cast<int>(mid_index), stream);
    auto mid_time = device->get_frame_timestamp(stream);
    auto last_index = device->get_frame_count() - 1;
    device->set_frame_by_index(last_index, stream);
    auto last_time = device->get_frame_timestamp(stream);
    auto max_error = 0.1 * expected_fps;
    double frame_count = last_index - mid_index;
    double duration_in_second = (double)(last_time - mid_time) * 0.001;
    auto actual_fps = frame_count / duration_in_second;
    EXPECT_NEAR(expected_fps, actual_fps, max_error);
}

TEST_P(playback_streaming_fixture, get_frame_data)
{
    auto stream_count = playback_tests_util::enable_available_streams(device);
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
    //prevent from runnimg async file with wait for frames
    rs::playback::file_info file_info = device->get_file_info();
    if(file_info.capture_mode == rs::playback::capture_mode::asynced) return;

    auto stream_count = playback_tests_util::enable_available_streams(device);

    device->set_real_time(false);
    EXPECT_FALSE(device->is_real_time());
    auto it = setup::profiles.begin();
    unsigned long long prev = 0;
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
    //prevent from runnimg async file with wait for frames
    rs::playback::file_info file_info = device->get_file_info();
    if(file_info.capture_mode == rs::playback::capture_mode::asynced) return;

    auto stream_count = playback_tests_util::enable_available_streams(device);
    ASSERT_NE(0, stream_count);
    rs::stream stream = rs::stream::color;
    device->enable_stream(stream, rs::preset::best_quality);
    device->start();
    std::this_thread::sleep_for (std::chrono::milliseconds(300));
    device->wait_for_frames();
    device->pause();
    auto first = device->get_frame_index(stream);
    std::this_thread::sleep_for (std::chrono::milliseconds(500));
    device->start();
    device->wait_for_frames();
    auto second = device->get_frame_index(stream);
    EXPECT_NEAR(first, second, 2);
}

TEST_P(playback_streaming_fixture, resume)
{
    //prevent from runnimg async file with wait for frames
    rs::playback::file_info file_info = device->get_file_info();
    if(file_info.capture_mode == rs::playback::capture_mode::asynced) return;

    auto stream_count = playback_tests_util::enable_available_streams(device);
    ASSERT_NE(0, stream_count);
    rs::stream stream = rs::stream::color;
    device->start();
    std::this_thread::sleep_for (std::chrono::milliseconds(200));
    device->wait_for_frames();
    device->pause();
    auto first = device->get_frame_timestamp(stream);
    device->start();
    std::this_thread::sleep_for (std::chrono::milliseconds(200));
    device->wait_for_frames();
    auto second = device->get_frame_timestamp(stream);
    EXPECT_GT(second, first);
}

TEST_P(playback_streaming_fixture, set_frame_by_index)
{
    auto stream_count = playback_tests_util::enable_available_streams(device);
    ASSERT_NE(0, stream_count);
    rs::stream stream = rs::stream::color;
    auto index = device->get_frame_count() - 1;
    device->set_frame_by_index(index, stream);
    EXPECT_EQ(index, device->get_frame_index(stream));
}

TEST_P(playback_streaming_fixture, DISABLED_set_frame_by_timestamp)
{
    auto stream_count = playback_tests_util::enable_available_streams(device);
    ASSERT_NE(0, stream_count);
    rs::stream stream = rs::stream::color;
    auto first_index = 100;
    device->set_frame_by_index(first_index, stream);
    auto ts1 = device->get_frame_timestamp(stream);
    device->set_frame_by_timestamp(static_cast<uint64_t>(ts1 + 100.0));
    auto second_index = device->get_frame_index(stream);
    EXPECT_GT(second_index, first_index);
}

TEST_P(playback_streaming_fixture, DISABLED_set_real_time)
{
    //prevent from runnimg async file with wait for frames
    rs::playback::file_info file_info = device->get_file_info();
    if(file_info.capture_mode == rs::playback::capture_mode::asynced) return;

    auto stream_count = playback_tests_util::enable_available_streams(device);
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
    auto stream_count = playback_tests_util::enable_available_streams(device);
    ASSERT_NE(0, stream_count);
    rs::stream stream = rs::stream::color;
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

    std::shared_ptr<rs::utils::viewer> viewer = std::make_shared<rs::utils::viewer>(stream_count, 320, 240, nullptr, "playback_set_frames");

    auto frame_count = device->get_frame_count();
    int counter = 0;
    while(counter < frame_count)
    {
        if(!device->set_frame_by_index(counter++, rs::stream::depth))break;
        for(int32_t s = (int32_t)rs::stream::depth; s <= (int32_t)rs::stream::fisheye; ++s)
        {
            auto stream = (rs::stream)s;
            if(device->is_stream_enabled(stream))
            {
                if(device->get_frame_data(stream) == nullptr)continue;
                auto image = test_utils::create_image(device, stream);
                viewer->show_image(image);
            }
        }
    }
}

TEST_P(playback_streaming_fixture, basic_playback)
{
    //prevent from runnimg async file with wait for frames
    rs::playback::file_info file_info = device->get_file_info();
    if(file_info.capture_mode == rs::playback::capture_mode::asynced) return;

    auto stream_count = playback_tests_util::enable_available_streams(device);

    std::shared_ptr<rs::utils::viewer> viewer = std::make_shared<rs::utils::viewer>(stream_count, 320, 240, nullptr, "basic_playback");

    device->start();
    auto counter = 0;
    while(device->is_streaming())
    {
        device->wait_for_frames();
        for(int32_t s = (int32_t)rs::stream::depth; s <= (int32_t)rs::stream::fisheye; ++s)
        {
            auto stream = (rs::stream)s;
            if(device->is_stream_enabled(stream))
            {
                auto image = test_utils::create_image(device, stream);
                viewer->show_image(image);
            }
        }
        counter++;
    }
}

TEST_P(playback_streaming_fixture, motions_callback)
{
    if(!device->supports(rs::capabilities::motion_events))return;
    std::chrono::high_resolution_clock::time_point begin;

    int sleep_time = 200;
    std::vector<rs::motion_data> pb_motion_events;
    std::vector<uint64_t> pb_motion_time_stamps;
    std::vector<rs::timestamp_data> pb_time_stamp_events;
    uint64_t pb_first_motion_sample_delay = 0;
    auto motion_callback = [&pb_motion_events, &begin, &pb_motion_time_stamps, &pb_first_motion_sample_delay](rs::motion_data entry)
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::microseconds>(now - begin).count();
        if(pb_motion_time_stamps.size() == 0)
            pb_first_motion_sample_delay = diff;
        pb_motion_time_stamps.push_back(diff);
        pb_motion_events.push_back(entry);
    };

    auto timestamp_callback = [&pb_time_stamp_events](rs::timestamp_data entry)
    {
        pb_time_stamp_events.push_back(entry);
    };

    device->enable_motion_tracking(motion_callback, timestamp_callback);
    begin = std::chrono::high_resolution_clock::now();
    device->start(rs::source::all_sources);
    while(device->is_streaming())
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
    device->stop(rs::source::all_sources);

    //prevent from runnimg async file with wait for frames
    rs::playback::file_info file_info = device->get_file_info();
    auto motion_events = file_info.capture_mode == rs::playback::capture_mode::asynced ? setup::motion_events_async : setup::motion_events_sync;
    auto time_stamps_events = file_info.capture_mode == rs::playback::capture_mode::asynced ? setup::time_stamps_events_async : setup::time_stamps_events_sync;

    EXPECT_NEAR(static_cast<double>(motion_events.size()), static_cast<double>(pb_motion_events.size()), static_cast<double>(motion_events.size()) * 0.01);
    EXPECT_NEAR(static_cast<double>(time_stamps_events.size()), static_cast<double>(pb_time_stamp_events.size()), static_cast<double>(time_stamps_events.size()) * 0.01);
}

TEST_P(playback_streaming_fixture, frames_callback)
{
    auto stream_count = playback_tests_util::enable_available_streams(device);

    std::mutex mutex;

    std::map<rs::stream,unsigned int> frame_counter;
    int warmup = 2;
    int run_time = setup::frames / max(setup::color_stream_profile.frame_rate, setup::depth_stream_profile.frame_rate) - warmup;
    auto callback = [&frame_counter, &mutex](rs::frame f)
    {
        auto stream = f.get_stream_type();
        std::lock_guard<std::mutex> guard(mutex);
        frame_counter[stream]++;
    };

    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        auto stream = it->first;
        frame_counter[stream] = 0;
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
        auto max_excepted_error = actual_fps * 0.1;
        EXPECT_GT(actual_fps, fps - max_excepted_error);
    }
}

TEST_P(playback_streaming_fixture, playback_and_render_callbak)
{
    auto stream_count = playback_tests_util::enable_available_streams(device);

    std::shared_ptr<rs::utils::viewer> viewer = std::make_shared<rs::utils::viewer>(stream_count, 320, 240, nullptr, "playback_and_render_callbak");

    auto callback = [viewer](rs::frame f)
    {
        auto stream = f.get_stream_type();
        auto image = rs::utils::get_shared_ptr_with_releaser(rs::core::image_interface::create_instance_from_librealsense_frame(f, rs::core::image_interface::flag::any));
        viewer->show_image(image);
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
}

TEST_P(playback_streaming_fixture, frame_time_domain)
{
    //prevent from runnimg async file with wait for frames
    rs::playback::file_info file_info = device->get_file_info();
    if(file_info.capture_mode == rs::playback::capture_mode::synced) return;

    auto stream_count = playback_tests_util::enable_available_streams(device);

    std::vector<rs::timestamp_domain> time_stamps_domain;

    auto callback = [&time_stamps_domain](rs::frame f)
    {
        time_stamps_domain.push_back(f.get_frame_timestamp_domain());
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

    ASSERT_EQ(setup::time_stamps_domain.size(), time_stamps_domain.size());
    for(uint32_t i = 0; i < time_stamps_domain.size(); i++)
    {
        EXPECT_EQ(setup::time_stamps_domain[i], time_stamps_domain[i]);
    }
}

TEST_P(playback_streaming_fixture, get_frame_metadata)
{
    //This test does not cover backwards compatability for record\playback.
    //When we have the option to play from a remote ftp we can add such test

    auto stream_count = playback_tests_util::enable_available_streams(device);
    std::map<rs::stream, bool> callbacksReceived;

    for(auto it = setup::profiles.begin(); it != setup::profiles.end(); ++it)
    {
        rs::stream stream = it->first;
        callbacksReceived[stream] = false;

        device->set_frame_callback(stream, [stream, &callbacksReceived](rs::frame f)
        {
            callbacksReceived[stream] = true;
            ASSERT_TRUE(f.supports_frame_metadata(rs_frame_metadata::RS_FRAME_METADATA_ACTUAL_EXPOSURE));
            uint32_t metadata = 0;
            try
            {       for(; metadata < rs_frame_metadata::RS_FRAME_METADATA_COUNT; metadata++)
                    {
                        f.get_frame_metadata(static_cast<rs_frame_metadata>(metadata));
                    }
            }catch(...)
            {
                FAIL() << "Got an exception while getting metadata " << metadata << " from frame";
            }
        });
    }

    device->set_real_time(false);
    device->start();
    while(device->is_streaming())
        std::this_thread::sleep_for(std::chrono::seconds(1));
    device->stop();
    for(auto streamReceived : callbacksReceived)
    {
        EXPECT_TRUE(streamReceived.second) << "No callbacks received during the test for stream type " << streamReceived.first;
    }
}

INSTANTIATE_TEST_CASE_P(playback_tests, playback_streaming_fixture, ::testing::Values(
                            setup::file_callbacks,
                            setup::file_wait_for_frames
                        ));
