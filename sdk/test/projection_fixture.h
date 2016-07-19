// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once

/* standard */
#include <string>
#include <utility>
#include <map>

/* gtest */
#include "gtest/gtest.h"

/* librealsense */
#include "librealsense/rs.hpp"

/* playback and record */
#include "rs/playback/playback_context.h"
#include "rs/playback/playback_device.h"
#include "rs/record/record_context.h"
#include "rs/core/types.h"

/* projection */
#include "rs/core/projection_interface.h"

/* logging */
#include "rs/utils/log_utils.h"

/* projection defines */
static const float NORM_DISTANCE     = 400.f;
static const float FAR_DISTANCE      = 40000.f;
static const int   MAX_DISTANCE      = 30000;
static const int   CUBE_VERTICES     = 8;

namespace projection_tests_util
{
    static const std::string file_name    = "/tmp/rstest.rssdk";
    static const rs::format  depth_format = rs::format::z16;
    static const rs::format  color_format = rs::format::rgb8;
    static const int total_frames = 100;
}

class projection_fixture : public testing::Test
{
public:
    projection_fixture() {}

protected:
    static void SetUpTestCase()
    {
        //create a record enabled context with a given output file
        rs::record::context m_context(projection_tests_util::file_name);

        ASSERT_NE(0, m_context.get_device_count()) << "no device detected";

        //each device created from the record enabled context will write the streaming data to the given file
        rs::device* m_device = m_context.get_device(0);

        m_device->enable_stream(rs::stream::depth, 320, 240, projection_tests_util::depth_format, 30);
        m_device->enable_stream(rs::stream::color, 320, 240, projection_tests_util::color_format, 30);
        m_device->start();
        int frame = projection_tests_util::total_frames;
        while(frame--)
        {
            m_device->wait_for_frames();
        }
        m_device->stop();
    }

    virtual void SetUp()
    {
        //create a playback context with a given input file
        m_context = std::unique_ptr<rs::playback::context>(new rs::playback::context(projection_tests_util::file_name));

        //create a playback device
        m_device = m_context->get_playback_device();
        ASSERT_NE(nullptr, m_device);

        /* enabling all available streams */
        std::vector<rs::stream> streams = { rs::stream::color, rs::stream::depth };
        for(auto iter = streams.begin(); iter != streams.end(); iter++)
        {
            if(m_device->get_stream_mode_count(*iter) > 0)
            {
                int width, height, fps;
                rs::format format;
                m_device->get_stream_mode(*iter, 0, width, height, format, fps);
                m_device->enable_stream(*iter, width, height, format, fps);
                m_formats.insert(std::make_pair((*iter), format));
            }
        }
        /* Enabling projection */
        m_color_intrin = m_device->get_stream_intrinsics(rs::stream::color);
        m_depth_intrin = m_device->get_stream_intrinsics(rs::stream::depth);
        m_extrinsics = m_device->get_extrinsics(rs::stream::depth, rs::stream::color);
        m_projection = std::unique_ptr<rs::core::projection>(rs::core::projection::create_instance(&m_color_intrin, &m_depth_intrin, &m_extrinsics));

        m_is_failed = false;
        m_sts = rs::core::status::status_no_error;
    }

    virtual void TearDown()
    {
        m_projection.release();
        if (m_is_failed)
        {
            std::cerr << std::endl << "something went wrong:" << std::endl;
            EXPECT_EQ(m_is_failed, false);
            std::cerr << "please, check the logs for any additional information" << std::endl;
        }
    }

    static void TearDownTestCase()
    {
        ::remove(projection_tests_util::file_name.c_str());
    }

    int32_t m_points_max;
    bool m_is_failed;
    const std::vector<float> m_distances = {NORM_DISTANCE, FAR_DISTANCE}; // distances scope definition
    std::map<rs::stream, rs::format> m_formats;
    float m_avg_err, m_max_err;
    std::unique_ptr<rs::core::projection> m_projection;
    rs::core::status m_sts;
    rs_intrinsics m_color_intrin;
    rs_intrinsics m_depth_intrin;
    rs_extrinsics m_extrinsics;
    rs::utils::log_util m_log_util;

    std::unique_ptr<rs::playback::context> m_context;
    rs::playback::device * m_device;
};
