// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <stdio.h>
#include <map>
#include <fstream>
#include <thread>
#include <librealsense/rs.hpp>
#include "gtest/gtest.h"
#include "utilities/utilities.h"
#include "rs/record/record_device.h"
#include "rs/record/record_context.h"
#include "rs/playback/playback_device.h"
#include "rs/playback/playback_context.h"
#include "file_types.h"

using namespace std;
using namespace rs::core;
using namespace rs::core::file_types;

namespace setup
{
    static const std::string file_path = "rstest";
    static std::vector<rs::record::compression_level> compression_levels;
    static vector<rs::stream> streams;
}

class compression_fixture : public testing::Test
{
protected:
    std::unique_ptr<rs::record::context> m_record_context;
    rs::record::device * m_record_device;
    std::unique_ptr<rs::playback::context> m_playback_context;
    rs::playback::device * m_playback_device;

    static void TearDownTestCase()
    {
        ::remove(setup::file_path.c_str());
    }

    void create_record_device()
    {
        m_record_context = std::unique_ptr<rs::record::context>(new rs::record::context(setup::file_path.c_str()));
        ASSERT_NE(0, m_record_context->get_device_count()) << "no device detected";
        m_record_device = m_record_context->get_record_device(0);
        ASSERT_NE(nullptr, m_record_device);
    }

    void create_playback_device()
    {
        m_playback_context = std::unique_ptr<rs::playback::context>(new rs::playback::context(setup::file_path.c_str()));
        ASSERT_NE(0, m_playback_context->get_device_count()) << "no device detected";
        m_playback_device = m_playback_context->get_playback_device();
        ASSERT_NE(nullptr, m_playback_device);
    }

    virtual void SetUp()
    {

    }

    static void SetUpTestCase()
    {
        for(int compression = rs::record::compression_level::disabled; compression <= rs::record::compression_level::high; compression++)
            setup::compression_levels.push_back((rs::record::compression_level)compression);
        setup::streams = { rs::stream::color, rs::stream::depth, rs::stream::infrared, rs::stream::infrared2, rs::stream::fisheye };
    }
};

TEST_F(compression_fixture, get_set_get_compression_level)
{
    create_record_device();

    for(auto stream : setup::streams)
    {
        EXPECT_EQ(m_record_device->get_compression_level(stream), rs::record::compression_level::high);
    }

    for(auto stream : setup::streams)
    {
        for(auto compression : setup::compression_levels)
        {
            EXPECT_EQ(m_record_device->set_compression(stream, compression), status::status_no_error);
            EXPECT_EQ(m_record_device->get_compression_level(stream), compression);
        }
    }
}

TEST_F(compression_fixture, check_failures_on_illegal_compression_level_values)
{
    create_record_device();

    for(auto stream : setup::streams)
    {
        int compression = rs::record::compression_level::disabled - 1;
        status expected_sts = (compression >= rs::record::compression_level::disabled && compression <= rs::record::compression_level::high) ? status::status_no_error : status::status_invalid_argument;
        EXPECT_EQ(expected_sts, m_record_device->set_compression(stream, (rs::record::compression_level)compression));

        compression = rs::record::compression_level::disabled + 1;
        expected_sts = (compression >= rs::record::compression_level::disabled && compression <= rs::record::compression_level::high) ? status::status_no_error : status::status_invalid_argument;
        EXPECT_EQ(expected_sts, m_record_device->set_compression(stream, (rs::record::compression_level)compression));
    }
}

TEST_F(compression_fixture, DISABLED_decompressed_data_is_lossless_on_lossless_codec)
{
    std::map<rs::stream,std::pair<uint64_t,std::vector<uint8_t>>> stream_to_original_frame_data;
    std::map<rs::stream,std::pair<uint64_t,std::vector<uint8_t>>> stream_to_decompressed_frame_data;

    //record
    create_record_device();

    bool done = false;
    auto record_frame_callback = [&done, &stream_to_original_frame_data](rs::frame frame)
    {
        if(stream_to_original_frame_data.find(frame.get_stream_type()) == stream_to_original_frame_data.end())
        {
            auto size = frame.get_stride() * frame.get_height();
            //save data to frmaes map
            stream_to_original_frame_data[frame.get_stream_type()] = { frame.get_frame_number(), std::vector<uint8_t>((uint8_t*)frame.get_data(), (uint8_t*)frame.get_data() + size) };
        }
        if(stream_to_original_frame_data.size() < setup::streams.size())
            return;
        done = true;
    };

    for(auto stream : setup::streams)
    {
        m_record_device->enable_stream(stream, rs::preset::largest_image);
        m_record_device->set_frame_callback(stream, record_frame_callback);
        m_record_device->set_compression(stream, rs::record::compression_level::high);
    }

    m_record_device->start();
    while(m_record_device->is_streaming() && !done)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    m_record_device->stop();

    //playback
    create_playback_device();

    m_playback_device->set_real_time(false);

    done = false;
    auto playback_frame_callback = [&done, &stream_to_decompressed_frame_data, &stream_to_original_frame_data](rs::frame frame)
    {
        if(frame.get_frame_number() != stream_to_original_frame_data[frame.get_stream_type()].first)
            return;
        auto size = frame.get_stride() * frame.get_height();
        stream_to_decompressed_frame_data[frame.get_stream_type()] = { frame.get_frame_number(), std::vector<uint8_t>((uint8_t*)frame.get_data(), (uint8_t*)frame.get_data() + size) };
        if(stream_to_decompressed_frame_data.size() < stream_to_original_frame_data.size())
            return;
        done = true;
    };

    for(auto stream : setup::streams)
    {
        m_playback_device->enable_stream(stream, rs::preset::best_quality);
        m_playback_device->set_frame_callback(stream, playback_frame_callback);
    }

    m_playback_device->set_real_time(false);

    m_playback_device->start();
    while(m_playback_device->is_streaming() && !done)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    m_playback_device->stop();

    ASSERT_EQ(stream_to_original_frame_data.size(), stream_to_decompressed_frame_data.size());

    for(auto stream : setup::streams)
    {
        auto & org_data = stream_to_original_frame_data[stream].second;
        auto & dec_data = stream_to_decompressed_frame_data[stream].second;
        ASSERT_EQ(org_data.size(), dec_data.size());
        for(uint32_t i = 0; i < org_data.size(); i++)
        {
            EXPECT_EQ(org_data[i], dec_data[i]);
        }
    }
}

TEST_F(compression_fixture, DISABLED_check_higher_compression_level_generates_smaller_file_size)
{
    std::map<rs::record::compression_level,uint64_t> compressed_file_sizes;

    for(auto compression : setup::compression_levels)
    {
        create_record_device();

        bool done = false;
        uint32_t frame_count = 0;
        auto record_frame_callback = [&done, &frame_count, this](rs::frame frame)
        {
            frame_count++;
            if(frame_count >= 20)
            {
                m_record_device->pause_record();
                done = true;
            }
        };

        rs::stream stream = rs::stream::depth;

        m_record_device->enable_stream(stream, rs::preset::highest_framerate);
        m_record_device->set_frame_callback(stream, record_frame_callback);

        m_record_device->set_compression(stream, compression);

        m_record_device->start();
        while(m_record_device->is_streaming() && !done)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        m_record_device->stop();
        std::ifstream myfile (setup::file_path, ios::binary);
        myfile.seekg (0, ios::end);
        compressed_file_sizes[compression] = myfile.tellg();
    }

    uint64_t prev_size = 0;
    for(auto compression : setup::compression_levels)
    {
        if(prev_size > 0)
            EXPECT_GT(prev_size, compressed_file_sizes[compression]);
        prev_size = compressed_file_sizes[compression];
    }
}

TEST_F(compression_fixture, four_streams_maximal_frame_drop_of_10_percent)
{
    uint32_t frame_count = 200;
    for(auto compression : setup::compression_levels)
    {
        std::map<rs::stream,size_t> stream_to_frame_count;

        create_record_device();

        bool done = false;
        std::mutex frame_callback_mutex;

        auto record_frame_callback = [&done, &stream_to_frame_count, frame_count, &frame_callback_mutex, this](rs::frame frame)
        {
            std::lock_guard<std::mutex> guard(frame_callback_mutex);
            stream_to_frame_count[frame.get_stream_type()]++;
            for(auto frames : stream_to_frame_count)
            {
                if(frames.second < frame_count)
                    return;
            }
            m_record_device->pause_record();
            done = true;
        };

        auto streams = { rs::stream::depth, rs::stream::color, rs::stream::fisheye, rs::stream::infrared };

        for(auto stream : streams)
        {
            m_record_device->enable_stream(stream, rs::preset::highest_framerate);
            m_record_device->set_frame_callback(stream, record_frame_callback);
            m_record_device->set_compression(stream, compression);
        }

        m_record_device->start();
        while(m_record_device->is_streaming() && !done)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        m_record_device->stop();

        std::map<rs::stream,size_t> first_frame_numbers;
        std::map<rs::stream,size_t> last_frame_numbers;
        auto playback_frame_callback = [&first_frame_numbers, &last_frame_numbers, &frame_callback_mutex,this](rs::frame frame)
        {
            std::lock_guard<std::mutex> guard(frame_callback_mutex);

            last_frame_numbers[frame.get_stream_type()] = frame.get_frame_number();

            if(first_frame_numbers.find(frame.get_stream_type()) == first_frame_numbers.end())
                first_frame_numbers[frame.get_stream_type()] = frame.get_frame_number();
        };

        create_playback_device();

        for(auto stream : streams)
        {
            m_playback_device->enable_stream(stream, rs::preset::highest_framerate);
            m_playback_device->set_frame_callback(stream, playback_frame_callback);
        }

        m_playback_device->set_real_time(true);

        m_playback_device->start();
        while(m_playback_device->is_streaming());
        m_playback_device->stop();

        for(auto stream : streams)
        {
            ASSERT_NE(last_frame_numbers[stream], first_frame_numbers[stream]);
            double first = (double)first_frame_numbers[stream];
            double last = (double)last_frame_numbers[stream];
            double count = (double)m_playback_device->get_frame_count(stream);
            int fps = m_playback_device->get_stream_framerate(stream);
            int frame_size = m_playback_device->get_stream_width(stream) * m_playback_device->get_stream_height(stream);
            //std::cout << "compression - " << compression << ", count - " << count << ", size - " << frame_size << ", fps - " << fps << ", first - " << first << ", last - " << last << ", percent - " << count / (last - first + 1.0) << std::endl;
            EXPECT_GT(count / (last - first + 1), 0.9);//expect no more than 10% frame drop
        }
    }
}


