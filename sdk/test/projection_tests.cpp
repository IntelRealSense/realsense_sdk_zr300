// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "projection_fixture.h"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <locale>
#include "rs/utils/librealsense_conversion_utils.h"
#include "rs/core/custom_image.h"
#include "image/image_utils.h"

using namespace rs::core;
using namespace rs::utils;

static rs::core::point3dF32 world3dSrc[CUBE_VERTICES];
static const rs::core::point3dF32 cube100mm[CUBE_VERTICES] =
{
    {  0.f,   0.f,   0.f},
    {100.f,   0.f,   0.f},
    {  0.f, 100.f,   0.f},
    {100.f, 100.f,   0.f},
    {  0.f,   0.f, 100.f},
    {100.f,   0.f, 100.f},
    {  0.f, 100.f, 100.f},
    {100.f, 100.f, 100.f},
};

__inline rs::core::point3dF32 *cube100mmWorldTr(float trX, float trY, float trZ)
{
    for( int n=0; n<CUBE_VERTICES; n++ )
    {
        world3dSrc[n].x = cube100mm[n].x + trX;
        world3dSrc[n].y = cube100mm[n].y + trY;
        world3dSrc[n].z = cube100mm[n].z + trZ;
    }
    return &world3dSrc[0];
}

__inline float distance3d(rs::core::point3dF32 v1, rs::core::point3dF32 v2)
{
    return sqrtf( (v1.x - v2.x)*(v1.x - v2.x) +
                  (v1.y - v2.y)*(v1.y - v2.y) +
                  (v1.z - v2.z)*(v1.z - v2.z) );
}

__inline float distancePixels(rs::core::pointF32 v1, rs::core::pointF32 v2)
{
    return std::max(fabs(v1.x - v2.x), fabs(v1.y - v2.y));
}
static const wchar_t* rsformatToString(rs::format format)
{
    switch(format)
    {
        case rs::format::any:                       return L"UNKNOWN";
        case rs::format::bgra8:                     return L"COLOR_BGR32";
        case rs::format::rgba8:                     return L"COLOR_RGB32";
        case rs::format::bgr8:                      return L"COLOR_BGR24";
        case rs::format::rgb8:                      return L"COLOR_RGB24";
        case projection_tests_util::depth_format:   return L"DEPTH";
        default:                                    return L"Incorrect Pixel Format";
    };
}

__inline rs::core::sizeI32 imSize(int32_t w, int32_t h) {rs::core::sizeI32 sz = {w, h}; return sz;}


/*
    Test:
        camera_to_color_to_camera

    Target:
        Checks compatibility of ProjectCameraToColor and ProjectColorToCamera

    Scope:
        1. All available '.rssdk' files from PROJECTION folder with different aspect ratios and serialized projection data
        2. Normal and far m_distances from object to Camera

    Description:
        A Cube with a size 100mm in Camera (World) coordinates projects to Color image and after that projects back to Camera.
        An expectation is to have projected back Cube coordinates near the source Cube in the World.
        An error gets as distance in mm of source Camera (World) coordinates and projected back to Camera through a Color Image.

    Pass Criteria:
        Test passes if average error and maximal error less than threshold.
*/
TEST_F(projection_fixture, camera_to_color_to_camera)
{
    m_avg_err = 2.f;
    m_max_err = 2.f;
    rs::core::point3dF32 pos3dDst[CUBE_VERTICES], pos_ijSrc[CUBE_VERTICES];
    rs::core::pointF32 pos_ijDst[CUBE_VERTICES];
    for(int32_t dd = 0; dd < (int32_t)m_distances.size(); dd++)
    {
        rs::core::point3dF32 *pos3dSrc = cube100mmWorldTr( 0.f, 0.f, m_distances[dd] );
        m_sts = m_projection->project_camera_to_color(CUBE_VERTICES, pos3dSrc, pos_ijDst);
        if( m_sts == status_param_unsupported ) continue;
        else ASSERT_EQ(m_sts, status_no_error);
        for(int32_t n=0; n<CUBE_VERTICES; n++ )
        {
            pos_ijSrc[n].x = pos_ijDst[n].x, pos_ijSrc[n].y = pos_ijDst[n].y;
            pos_ijSrc[n].z = pos3dSrc[n].z;
        }
        m_sts = m_projection->project_color_to_camera(CUBE_VERTICES, pos_ijSrc, pos3dDst);
        if( m_sts == status_param_unsupported ) continue;
        else ASSERT_EQ(m_sts, status_no_error);
        /* Check distance between source Camera coordinates and projected back to Camera coordinates */
        float avg = 0.f, max = 0.f;
        for(int32_t n = 0; n < CUBE_VERTICES; n++ )
        {
            float v = distance3d( pos3dSrc[n], pos3dDst[n]);
            if( max < v ) max = v;
            avg += v;
        }
        avg /= CUBE_VERTICES;
        EXPECT_LE(avg, m_avg_err);
        EXPECT_LE(max, m_max_err);
        if(avg > m_avg_err || max > m_max_err)
        {
            std::basic_ostringstream<wchar_t> stream;
            stream << L"FAIL: " << rsformatToString(m_formats.at(rs::stream::color)) << " " << m_color_intrin.width << "x" << m_color_intrin.height << "; "
                   << "File: " << projection_tests_util::file_name.c_str() << " distance[mm]=" << m_distances[dd] << "; m_avg_error[mm]=" << avg << "; m_max_error[mm]=" << max;
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, stream.str().c_str(), __FILE__, __LINE__, "camera_to_color_to_camera");
            m_is_failed = true;
        }
    }
}


/*
    Test:
        camera_to_depth_to_camera

    Target:
        Checks compatibility of ProjectCameraToDepth and ProjectDepthToCamera

    Scope:
        1. All available '.rssdk' files from PROJECTION folder with different aspect ratios and serialized projection data
        2. Normal and far m_distances from object to Camera

    Description:
        A Cube with a size 100mm in Camera (World) corrdinates projects to Depth image and after that projects back to Camera.
        An expectation is to have projected back Cube coordinates near the source Cube in the World.
        An error gets as distance in mm of source Camera (World) coordinates and projected back to Camera through a Depth Image.

    Pass Criteria:
        Test passes if average error and maximal error less than threshold.

*/
TEST_F(projection_fixture, camera_to_depth_to_camera)
{
    m_avg_err = 2.f;
    m_max_err = 2.2f;

    rs::core::point3dF32 pos3dDst[CUBE_VERTICES], pos_ijSrc[CUBE_VERTICES];
    rs::core::pointF32 pos_ijDst[CUBE_VERTICES];
    for(int32_t dd = 0; dd < (int32_t)m_distances.size(); dd++)
    {
        rs::core::point3dF32 *pos3dSrc = cube100mmWorldTr( 0.f, 0.f, m_distances[dd] );
        m_sts = m_projection->project_camera_to_depth(CUBE_VERTICES, pos3dSrc, pos_ijDst);
        if( m_sts == status_param_unsupported ) continue;
        else ASSERT_EQ(m_sts, status_no_error);
        for(int32_t n=0; n<CUBE_VERTICES; n++ )
        {
            pos_ijSrc[n].x = pos_ijDst[n].x, pos_ijSrc[n].y = pos_ijDst[n].y;
            pos_ijSrc[n].z = pos3dSrc[n].z;
        }
        m_sts = m_projection->project_depth_to_camera(CUBE_VERTICES, pos_ijSrc, pos3dDst);
        if( m_sts == status_param_unsupported ) continue;
        else ASSERT_EQ(m_sts, status_no_error);
        /* Check distance between source Camera coordinates and projected back to Camera coordinates */
        float avg = 0.f, max = 0.f;
        for(int32_t n = 0; n < CUBE_VERTICES; n++ )
        {
            float v = distance3d( pos3dSrc[n], pos3dDst[n]);
            if( max < v ) max = v;
            avg += v;
        }
        avg /= CUBE_VERTICES;
        EXPECT_LE(avg, m_avg_err);
        EXPECT_LE(max, m_max_err);
        if( avg > m_avg_err || max > m_max_err)
        {
            std::basic_ostringstream<wchar_t> stream;
            stream << L"FAIL: " << rsformatToString(m_formats.at(rs::stream::depth)) << " " << m_depth_intrin.width << "x" << m_depth_intrin.height << "; "
                   << "File: " << projection_tests_util::file_name.c_str() << " distance[mm]=" << m_distances[dd] << "; m_avg_error[mm]=" << avg << "; m_max_error[mm]=" << max;
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, stream.str().c_str(), __FILE__, __LINE__, "camera_to_depth_to_camera");
            m_is_failed = true;
        }
    }
}


/*
    Test:
        color_to_camera_to_color

    Target:
        Checks compatibility of ProjectColorToCamera and ProjectCameraToColor

    Scope:
        All available '.rssdk' files from PROJECTION folder with different aspect ratios and serialized projection data

    Description:
        A  set of points from a different places of Color image in pixel coordinates project to Camera and after that projects back to Color image.
        An expectation is to have projected back Color image pixel coordinates near the source Color pixel coordinates.
        An error gets as distance in pixels of source Color coordinates and projected back to Color through a Camera Coordinate System.

    Pass Criteria:
        Test passes if average error and maximal error less than threshold.

*/
TEST_F(projection_fixture, color_to_camera_to_color)
{
    m_avg_err = 0.0002f;
    m_max_err = 0.0005f;

    for(int32_t dd = 0; dd < (int32_t)m_distances.size(); dd++)
    {
        rs::core::point3dF32 pos_uvzSrc[9] =
        {
            {m_color_intrin.width/2.f, m_color_intrin.height/2.f, m_distances[dd]},
            {5.f, 5.f, m_distances[dd]},
            {(m_color_intrin.width-5.f), 5.f, m_distances[dd]},
            {5.f, (m_color_intrin.height-5.f), m_distances[dd]},
            {(m_color_intrin.width-5.f), (m_color_intrin.height-5.f), m_distances[dd]},
            {100.f, 100.f, m_distances[dd]},
            {(m_color_intrin.width-100.f), 100.f, m_distances[dd]},
            {100.f, (m_color_intrin.height-100.f), m_distances[dd]},
            {(m_color_intrin.width-100.f), (m_color_intrin.height-100.f), m_distances[dd]},
        };
        const int npoints = sizeof(pos_uvzSrc)/sizeof(pos_uvzSrc[0]);
        rs::core::point3dF32 pos3dDst[npoints];
        rs::core::pointF32 pos_uvzDst[npoints];
        m_sts = m_projection->project_color_to_camera(npoints, pos_uvzSrc, pos3dDst);
        if( m_sts == status_param_unsupported ) continue;
        else ASSERT_EQ(m_sts, status_no_error);
        m_sts = m_projection->project_camera_to_color(npoints, pos3dDst, pos_uvzDst);
        if( m_sts == status_param_unsupported ) continue;
        else ASSERT_EQ(m_sts, status_no_error);
        /* Check coordinates */
        float avg = 0.f, max = 0.f;
        for(int32_t n = 0; n < npoints; n++ )
        {
            float v = sqrtf( (pos_uvzSrc[n].x - pos_uvzDst[n].x)*(pos_uvzSrc[n].x - pos_uvzDst[n].x) +
                             (pos_uvzSrc[n].y - pos_uvzDst[n].y)*(pos_uvzSrc[n].y - pos_uvzDst[n].y) );
            if( max < v ) max = v;
            avg += v;
        }
        avg /= npoints;
        EXPECT_LE(avg, m_avg_err);
        EXPECT_LE(max, m_max_err);
        if(avg > m_avg_err || max > m_max_err)
        {
            std::basic_ostringstream<wchar_t> stream;
            stream << L"FAIL: " << rsformatToString(m_formats.at(rs::stream::color)) << " " << m_color_intrin.width << "x" << m_color_intrin.height << "; "
                   << "File: " << projection_tests_util::file_name.c_str() << " distance[mm]=" << m_distances[dd] << "; m_avg_error[pxls]=" << avg << "; m_max_error[pxls]=" << max;
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, stream.str().c_str(), __FILE__, __LINE__, "color_to_camera_to_color");
            m_is_failed = true;
        }
    }
}


/*
    Test:
        depth_to_camera_to_depth

    Target:
        Checks compatibility of ProjectDepthToCamera and ProjectCameraToDepth

    Scope:
        All available '.rssdk' files from PROJECTION folder with different aspect ratios and serialized projection data

    Description:
        A  set of points from a different places of Depth image in pixel coordinates project to Camera and after that projects back to Depth image.
        An expectation is to have projected back Depth image pixel coordinates near the source Depth pixel coordinates.
        An error gets as distance in pixels of source Depth coordinates and projected back to Depth through a Camera Coordinate System.

    Pass Criteria:
        Test passes if average error and maximal error less than threshold

*/
TEST_F(projection_fixture, depth_to_camera_to_depth)
{
    m_avg_err = 3.f;
    m_max_err = 7.f;

    for(int32_t dd = 0; dd < (int32_t)m_distances.size(); dd++)
    {
        rs::core::point3dF32 pos_uvzSrc[9] =
        {
            {m_color_intrin.width/2.f, m_color_intrin.height/2.f, m_distances[dd]},
            {5.f, 5.f, m_distances[dd]},
            {(m_color_intrin.width-5.f), 5.f, m_distances[dd]},
            {5.f, (m_color_intrin.height-5.f), m_distances[dd]},
            {(m_color_intrin.width-5.f), (m_color_intrin.height-5.f), m_distances[dd]},
            {100.f, 100.f, m_distances[dd]},
            {(m_color_intrin.width-100.f), 100.f, m_distances[dd]},
            {100.f, (m_color_intrin.height-100.f), m_distances[dd]},
            {(m_color_intrin.width-100.f), (m_color_intrin.height-100.f), m_distances[dd]},
        };
        const int npoints = sizeof(pos_uvzSrc)/sizeof(pos_uvzSrc[0]);
        rs::core::point3dF32 pos3dDst[npoints];
        rs::core::pointF32 pos_uvzDst[npoints];
        m_sts = m_projection->project_depth_to_camera(npoints, pos_uvzSrc, pos3dDst);
        if( m_sts == status_param_unsupported ) continue;
        else ASSERT_EQ(m_sts, status_no_error);
        m_sts = m_projection->project_camera_to_depth(npoints, pos3dDst, pos_uvzDst);
        if( m_sts == status_param_unsupported ) continue;
        else ASSERT_EQ(m_sts, status_no_error);
        /* Check coordinates */
        float avg = 0.f, max = 0.f;
        for(int32_t n = 0; n < npoints; n++ )
        {
            float v = sqrtf( (pos_uvzSrc[n].x - pos_uvzDst[n].x)*(pos_uvzSrc[n].x - pos_uvzDst[n].x) +
                             (pos_uvzSrc[n].y - pos_uvzDst[n].y)*(pos_uvzSrc[n].y - pos_uvzDst[n].y) );
            if( max < v ) max = v;
            avg += v;
        }
        avg /= npoints;
        EXPECT_LE(avg, m_avg_err);
        EXPECT_LE(max, m_max_err);
        if( avg > m_avg_err || max > m_max_err)
        {
            std::basic_ostringstream<wchar_t> stream;
            stream << L"FAIL: " << rsformatToString(m_formats.at(rs::stream::depth)) << " " << m_depth_intrin.width << "x" << m_depth_intrin.height << "; "
                   << "File: " << projection_tests_util::file_name.c_str() << " distance[mm]=" << m_distances[dd] << "; m_avg_error[pxls]=" << avg << "; m_max_error[pxls]=" << max;
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, stream.str().c_str(), __FILE__, __LINE__, "depth_to_camera_to_depth");
            m_is_failed = true;
        }
    }
}


/*
    Test:
        map_depth_to_color_to_depth

    Target:
        Checks compatibility of MapDepthToColor and MapColorToDepth

        Scope:
        All available '.rssdk' files from PROJECTION folder with different aspect ratios and serialized projection data

    Description:
        A set of valid point from a Depth image in pixel coordinates maps to Color and after that maps back to Depth image.
        An expectation is to have mapped back Depth image pixel coordinates near the source Depth pixel coordinates.
        An error gets as distance in pixels of source Depth coordinates and mapped back to Depth.

    Pass Criteria:
        Test passes if average error and maximal error less than threshold for all frames.
*/
TEST_F(projection_fixture, map_depth_to_color_to_depth)
{
    m_avg_err = 0.7f;
    m_max_err = 2.f;
    m_points_max = 100;
    const int32_t numFrames = 2;

    //TODO: what is invalid_value?
    uint16_t invalid_value = m_device->get_option(rs::option::r200_depth_control_score_minimum_threshold);
    float avg = 0.f, max = 0.f;
    int32_t mnpoints = 0;
    bool skipped = false;

    int depthWidth, depthHeight;
    depthWidth = m_depth_intrin.width;
    depthHeight = m_depth_intrin.height;

    for (int i = m_device->get_frame_count() - numFrames; i < m_device->get_frame_count(); i++)
    {
        m_device->set_frame_by_index(i, rs::stream::depth);

        int depthPitch = depthWidth * image_utils::get_pixel_size(m_device->get_stream_format(rs::stream::depth));
        image_info  DepthInfo = {depthWidth, depthHeight, convert_pixel_format(projection_tests_util::depth_format), depthPitch};

        custom_image depth (&DepthInfo,
                            m_device->get_frame_data(rs::stream::depth),
                            stream_type::depth,
                            image_interface::flag::any,
                            m_device->get_frame_timestamp(rs::stream::depth),
                            nullptr,
                            nullptr);


        /* Retrieve the depth pixels */
        const uint8_t * ddata = (uint8_t*)depth.query_data();
        ASSERT_FALSE(!ddata);

        std::vector<rs::core::point3dF32> pos_ijSrc;
        int32_t npoints = 0;
        for (int32_t y = 0; y < m_color_intrin.height; y++)
        {
            uint16_t *d = (uint16_t*)(ddata + y * depth.query_info().pitch);
            for (int32_t x = 0; x < m_color_intrin.width; x++)
            {
                if (d[x] == invalid_value || d[x] > MAX_DISTANCE) continue; // no mapping based on unreliable depth values
                rs::core::point3dF32 pos_ijSrcTmp = {(float)x, (float)y, (float)d[x]};
                pos_ijSrc.push_back(pos_ijSrcTmp);
                npoints ++;
                break;
            }
            if(npoints >= m_points_max) break;
        }

        std::vector<rs::core::pointF32> pos_ijDst1(npoints);
        std::vector<rs::core::pointF32> pos_ijDst2(npoints);
        m_sts = m_projection->map_depth_to_color(npoints, &pos_ijSrc[0], &pos_ijDst1[0]);
        if(m_sts == status_param_unsupported)
        {
            skipped = true;
            continue;
        }
        else if(m_sts < status_no_error)
        {
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, L"Unable to MapDepthToColor", __FILE__, __LINE__, "map_depth_to_color_to_depth");
            ASSERT_EQ(m_sts, status_no_error);
        }
        m_sts = m_projection->map_color_to_depth(&depth, npoints, &pos_ijDst1[0], &pos_ijDst2[0]);
        if( m_sts == status_param_unsupported )
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, L"Unable to MapColorToDepth", __FILE__, __LINE__, "map_depth_to_color_to_depth");
            ASSERT_EQ(m_sts, status_no_error);
        }

        /* Check coordinates */
        for(int32_t n = 0; n < npoints; n++ )
        {
            if( pos_ijDst2[n].x != -1.f )
            {
                float v = std::max( fabs(pos_ijSrc[n].x - pos_ijDst2[n].x), fabs(pos_ijSrc[n].y - pos_ijDst2[n].y) );
                if( max < v ) max = v;
                avg += v;
            }
        }
        mnpoints += npoints;
    }
    if(!skipped)
    {
        avg = avg / mnpoints;
        EXPECT_LE(avg, m_avg_err);
        EXPECT_LE(max, m_max_err);
        if( avg > m_avg_err || max > m_max_err)
        {
            std::basic_ostringstream<wchar_t> stream;
            stream << L"FAIL: " << rsformatToString(m_formats.at(rs::stream::color)) << " " << m_color_intrin.width << "x" << m_color_intrin.height << "; ";
            stream << rsformatToString(m_formats.at(rs::stream::depth)) << " " << m_depth_intrin.width << "x" << m_depth_intrin.height << "; ";
            stream << "File: " << projection_tests_util::file_name.c_str() << "; m_avg_error[pxls]=" << avg << "; m_max_error[pxls]=" << max;
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, stream.str().c_str(), __FILE__, __LINE__, "map_depth_to_color_to_depth");
            m_is_failed = true;
        }
    }
}


/*
    Test:
        map_depth_camera_color

    Target:
        Checks compatibility of ProjectDepthToCamera, ProjectCameraToColor and MapDepthToColor

    Scope:
        All available '.rssdk' files from PROJECTION folder with different aspect ratios and serialized projection data

    Description:
        A set of valid point from a Depth image in pixel coordinates maps to Color with MapDepthToColor.
        The same valid point from a Depth image in pixel coordinates projects to World with ProjectDepthToCamera
        and after that projects to Color with ProjectCameraToColor.
        An expectation is to have projected ProjectDepthToCamera->ProjectCameraToColor Color image pixel
        coordinates near the results of mapped Depth pixel coordinates with MapDepthToColor.
        An error gets as distance in pixels of mapped and projected Depth coordinates.

    Pass Criteria:
        Test passes if average error and maximal error less than threshold for all frames.
*/
TEST_F(projection_fixture, map_depth_camera_color)
{
    m_avg_err = 0.0001f;
    m_max_err = 0.001f;
    m_points_max = 100;
    const int32_t numFrames = 2;

    //TODO: what is invalid_value?
    uint16_t invalid_value = m_device->get_option(rs::option::r200_depth_control_score_minimum_threshold);
    float avg = 0.f, max = 0.f;
    int32_t mnpoints = 0;
    bool skipped = false;

    int depthWidth, depthHeight;
    depthWidth = m_depth_intrin.width;
    depthHeight = m_depth_intrin.height;

    for (int i = m_device->get_frame_count() - numFrames; i < m_device->get_frame_count(); i++)
    {
        m_device->set_frame_by_index(i, rs::stream::depth);

        int depthPitch = depthWidth * 2;
        image_info  DepthInfo = {depthWidth, depthHeight, convert_pixel_format(projection_tests_util::depth_format), depthPitch};

        custom_image depth (&DepthInfo,
                            m_device->get_frame_data(rs::stream::depth),
                            stream_type::depth,
                            image_interface::flag::any,
                            m_device->get_frame_timestamp(rs::stream::depth),
                            nullptr,
                            nullptr);


        /* Retrieve the depth pixels */
        const uint8_t * ddata = (uint8_t*)depth.query_data();
        ASSERT_FALSE(!ddata);
        std::vector<rs::core::point3dF32> pos_ijSrc;
        for (int32_t y = 0; y < m_color_intrin.height; y++)
        {
            uint16_t *d = (uint16_t*)(ddata + y * depth.query_info().pitch);
            for (int32_t x = 0; x < m_color_intrin.width; x++)
            {
                if (d[x] == invalid_value || d[x] > MAX_DISTANCE) continue; // no mapping based on unreliable depth values
                rs::core::point3dF32 pos_ijSrcTmp = {(float)x, (float)y, (float)d[x]};
                pos_ijSrc.push_back(pos_ijSrcTmp);
            }
        }

        int32_t npoints = pos_ijSrc.size();
        std::vector<rs::core::pointF32> pos_ijDst1(npoints);
        std::vector<rs::core::point3dF32> pos_ijMid(npoints);
        std::vector<rs::core::pointF32> pos_ijDst2(npoints);
        m_sts = m_projection->map_depth_to_color(npoints, &pos_ijSrc[0], &pos_ijDst1[0]);
        if( m_sts == status_param_unsupported )
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            ASSERT_EQ(m_sts, status_no_error);
        }
        m_sts = m_projection->project_depth_to_camera(npoints, &pos_ijSrc[0], &pos_ijMid[0]);
        if( m_sts == status_param_unsupported )
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, L"Unable to ProjectDepthToCamera", __FILE__, __LINE__, "map_depth_camera_color");
            ASSERT_EQ(m_sts, status_no_error);
        }
        m_sts = m_projection->project_camera_to_color(npoints, &pos_ijMid[0], &pos_ijDst2[0]);
        if( m_sts == status_param_unsupported )
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, L"Unable to ProjectCameraToColor", __FILE__, __LINE__, "map_depth_camera_color");
            ASSERT_EQ(m_sts, status_no_error);
        }

        /* Check coordinates */
        for(int32_t n = 0; n < npoints; n++ )
        {
            if( pos_ijDst1[n].x != -1.f && pos_ijDst2[n].x != -1.f )
            {
                float v = distancePixels( pos_ijDst1[n], pos_ijDst2[n] );
                if( max < v ) max = v;
                avg += v;
            }
        }
        mnpoints += npoints;
    }
    if( !skipped )
    {
        avg = avg / mnpoints;
        EXPECT_LE(avg, m_avg_err);
        EXPECT_LE(max, m_max_err);
        if( avg > m_avg_err || max > m_max_err)
        {
            std::basic_ostringstream<wchar_t> stream;
            stream << L"FAIL: " << rsformatToString(m_formats.at(rs::stream::color)) << " " << m_color_intrin.width << "x" << m_color_intrin.height << "; ";
            stream << rsformatToString(m_formats.at(rs::stream::depth)) << " " << m_depth_intrin.width << "x" << m_depth_intrin.height << "; ";
            stream << "File: " << projection_tests_util::file_name.c_str() << "; m_avg_error[pxls]=" << avg << "; m_max_error[pxls]=" << max;
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, stream.str().c_str(), __FILE__, __LINE__, "map_depth_camera_color");
            m_is_failed = true;
        }
    }
}


/*
    Test:
        map_color_camera_depth

    Target:
        Checks compatibility of ProjectColorToCamera, ProjectCameraToDepth and MapColorToDepth

    Scope:
        All available '.rssdk' files from PROJECTION folder with different aspect ratios and serialized projection data

    Description:
        A set of valid point from a Color image in pixel coordinates maps to Depth with MapColorToDepth.
        The same valid point from a Color image in pixel coordinates projects to World with ProjectColorToCamera
        and after that projects to Depth with ProjectCameraToDepth.
        An expectation is to have projected ProjectColorToCamera->ProjectCameraToDepth Depth image pixel
        coordinates near the results of mapped Color pixel coordinates with MapColorToDepth.
        An error gets as distance in pixels of mapped and projected Color coordinates.

    Pass Criteria:
        Test passes if average error and maximal error less than threshold for all frames.
*/
TEST_F(projection_fixture, map_color_camera_depth)
{
    const int32_t numFrames = 2;
    m_avg_err = .6f;
    m_max_err = 4.f;
    m_points_max = 100;

    //TODO: what is invalid_value?
    uint16_t invalid_value = m_device->get_option(rs::option::r200_depth_control_score_minimum_threshold);
    float avg = 0.f, max = 0.f;
    int32_t mnpoints = 0;
    bool skipped = false;

    int depthWidth, depthHeight;
    depthWidth = m_depth_intrin.width;
    depthHeight = m_depth_intrin.height;

    for (int i = m_device->get_frame_count() - numFrames; i < m_device->get_frame_count(); i++)
    {
        m_device->set_frame_by_index(i, rs::stream::depth);

        int depthPitch = depthWidth * 2;
        image_info  DepthInfo = {depthWidth, depthHeight, convert_pixel_format(projection_tests_util::depth_format), depthPitch};

        custom_image depth (&DepthInfo,
                            m_device->get_frame_data(rs::stream::depth),
                            stream_type::depth,
                            image_interface::flag::any,
                            m_device->get_frame_timestamp(rs::stream::depth),
                            nullptr,
                            nullptr);


        /* Retrieve the depth pixels */
        const uint8_t * ddata = (uint8_t*)depth.query_data();
        ASSERT_FALSE(!ddata);


        // Choose all points from a Depth
        std::vector<rs::core::point3dF32> pos_ijSrc;
        for (int32_t y = 0; y < m_color_intrin.height; y++)
        {
            uint16_t *d = (uint16_t*)(ddata + y * depth.query_info().pitch);
            for (int32_t x = 0; x < m_color_intrin.width; x++)
            {
                if (d[x] == invalid_value || d[x] > MAX_DISTANCE) continue; // no mapping based on unreliable depth values
                rs::core::point3dF32 pos_ijSrcTmp = {(float)x, (float)y, (float)d[x]};
                pos_ijSrc.push_back(pos_ijSrcTmp);
            }
        }

        // Find a Color map of the choosen points
        std::vector<rs::core::pointF32> pos_ijSrc1(pos_ijSrc.size());
        m_sts = m_projection->map_depth_to_color(pos_ijSrc.size(), &pos_ijSrc[0], &pos_ijSrc1[0]);
        if( m_sts == status_param_unsupported )
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, L"Unable to MapDepthToColor", __FILE__, __LINE__, "map_color_camera_depth");
            ASSERT_EQ(m_sts, status_no_error);
        }


        int32_t npoints = 0;
        std::vector<rs::core::pointF32> pos_ijSrc2;
        std::vector<rs::core::point3dF32> pos_ijSrc3;
        for (size_t n = 0; n < pos_ijSrc1.size(); n++)
        {
            if( pos_ijSrc1[n].x != -1.f && pos_ijSrc1[n].y != -1.f )
            {
                pos_ijSrc2.push_back(pos_ijSrc1[n]);
                rs::core::point3dF32 pos_ijSrcTmp = {pos_ijSrc1[n].x, pos_ijSrc1[n].y, pos_ijSrc[n].z};
                pos_ijSrc3.push_back(pos_ijSrcTmp);
                npoints ++;
                if( npoints >= m_points_max ) break;
            }
        }
        pos_ijSrc.clear();
        pos_ijSrc1.clear();

        // Maps and projects back Color points to Depth
        std::vector<rs::core::pointF32> pos_ijDst1(npoints);
        m_sts = m_projection->map_color_to_depth(&depth, npoints, &pos_ijSrc2[0], &pos_ijDst1[0]);
        if( m_sts == status_param_unsupported )
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, L"Unable to MapColorToDepth", __FILE__, __LINE__, "map_color_camera_depth");
            ASSERT_EQ(m_sts, status_no_error);
        }
        std::vector<rs::core::point3dF32> pos_ijMid(npoints);
        std::vector<rs::core::pointF32> pos_ijDst2(npoints);


        m_sts = m_projection->project_color_to_camera(npoints, &pos_ijSrc3[0], &pos_ijMid[0]);
        if( m_sts == status_param_unsupported )
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, L"Unable to ProjectColorToCamera", __FILE__, __LINE__, "map_color_camera_depth");
            ASSERT_EQ(m_sts, status_no_error);
        }
        m_sts = m_projection->project_camera_to_depth(npoints, &pos_ijMid[0], &pos_ijDst2[0]);
        if( m_sts == status_param_unsupported )
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, L"Unable to ProjectCameraToDepth", __FILE__, __LINE__, "map_color_camera_depth");
            ASSERT_EQ(m_sts, status_no_error);
        }

        /* Check coordinates */
        for(int32_t n = 0; n < npoints; n++ )
        {
            if( pos_ijDst1[n].x != -1.f && pos_ijDst2[n].x != -1.f )
            {
                float v = distancePixels( pos_ijDst1[n], pos_ijDst2[n] );
                if( max < v ) max = v;
                avg += v;
            }
        }
        mnpoints += npoints;
    }
    if( !skipped )
    {
        avg = avg / mnpoints;
        EXPECT_LE(avg, m_avg_err);
        EXPECT_LE(max, m_max_err);
        if( avg > m_avg_err || max > m_max_err)
        {
            std::basic_ostringstream<wchar_t> stream;
            stream << L"FAIL: " << rsformatToString(m_formats.at(rs::stream::color)) << " " << m_color_intrin.width << "x" << m_color_intrin.height << "; ";
            stream << rsformatToString(m_formats.at(rs::stream::depth)) << " " << m_depth_intrin.width << "x" << m_depth_intrin.height << "; ";
            stream << "File: " << projection_tests_util::file_name.c_str() << "; m_avg_error[pxls]=" << avg << "; m_max_error[pxls]=" << max;
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, stream.str().c_str(), __FILE__, __LINE__, "map_color_camera_depth");
            m_is_failed = true;
        }
    }
}


/*
    Test:
        query_uvmap_map_depth_to_color

    Target:
        Checks compatibility of QueryUVMap and MapDepthToColor

    Scope:
        1. All available '.rssdk' files from PROJECTION folder with different aspect ratios and serialized projection data
        2. All valid Depth values

    Description:
        From a Depth image picks all valid pixels and calls MapDepthToColor.
        Also gets UV Map with QueryUVMap. An expectation is to have UV Map
        coordinates near the results of mapped Depth pixel coordinates with MapDepthToColor.
        An error gets as distance in pixels of mapped Depth coordinates.

    Pass Criteria:
        Test passes if average error and maximal error less than threshold for all frames.
*/
TEST_F(projection_fixture, query_uvmap_map_depth_to_color)
{
    m_avg_err = 2.f;
    m_max_err = 3.f;
    const int32_t numFrames = 2;

    //TODO: what is invalid_value?
    uint16_t invalid_value = m_device->get_option(rs::option::r200_depth_control_score_minimum_threshold);
    float avg = 0.f, max = 0.f;
    int32_t mnpoints = 0;
    bool skipped = false;
    std::vector<rs::core::pointF32> uvMap(m_color_intrin.width * m_color_intrin.height);

    int depthWidth, depthHeight;
    depthWidth = m_depth_intrin.width;
    depthHeight = m_depth_intrin.height;

    for (int i = m_device->get_frame_count() - numFrames; i < m_device->get_frame_count(); i++)
    {
        m_device->set_frame_by_index(i, rs::stream::depth);

        int depthPitch = depthWidth * 2;
        image_info  DepthInfo = {depthWidth, depthHeight, convert_pixel_format(projection_tests_util::depth_format), depthPitch};

        custom_image depth (&DepthInfo,
                            m_device->get_frame_data(rs::stream::depth),
                            stream_type::depth,
                            image_interface::flag::any,
                            m_device->get_frame_timestamp(rs::stream::depth),
                            nullptr,
                            nullptr);


        /* Retrieve the depth pixels */
        const uint8_t * ddata = (uint8_t*)depth.query_data();
        ASSERT_FALSE(!ddata);

        /* Get uvMap */
        m_sts = m_projection->query_uvmap(&depth, &uvMap[0]);
        if( m_sts == status_param_unsupported )
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            ASSERT_EQ(m_sts, status_no_error);
        }

        /* Retrieve the depth pixels */

        // Choose valid points from a Depth
        std::vector<rs::core::point3dF32> pos_ijSrc;
        int32_t npoints = 0;
        for (int32_t y = 0; y < m_color_intrin.height; y++)
        {
            uint16_t *d = (uint16_t*)(ddata + y * depth.query_info().pitch);
            for (int32_t x = 0; x < m_color_intrin.width; x++)
            {
                if (d[x] == invalid_value || d[x] > MAX_DISTANCE) continue; // no mapping based on unreliable depth values
                rs::core::point3dF32 pos_ijSrcTmp = {(float)x, (float)y, (float)d[x]};
                pos_ijSrc.push_back(pos_ijSrcTmp);
                npoints ++;
            }
        }

        // Find a Color map of the choosen points
        std::vector<rs::core::pointF32> pos_ijDst(npoints);
        m_sts = m_projection->map_depth_to_color(npoints, &pos_ijSrc[0], &pos_ijDst[0]);
        if( m_sts == status_param_unsupported )
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, L"Unable to MapDepthToColor", __FILE__, __LINE__, "query_uvmap_map_depth_to_color");
            ASSERT_EQ(m_sts, status_no_error);
        }

        /* Check coordinates */
        for(int32_t n = 0; n < npoints; n++ )
        {
            rs::core::pointF32 uv = uvMap[pos_ijSrc[n].y*m_color_intrin.width+pos_ijSrc[n].x];
            if( pos_ijDst[n].x != -1.f && pos_ijDst[n].y != -1.f && uv.x >= 0.f && uv.y >= 0.f && uv.x < 1.f && uv.y < 1.f)
            {
                uv.x *= m_color_intrin.width;
                uv.y *= m_color_intrin.height;
                float v = distancePixels( pos_ijDst[n], uv );
                //rs::core::pointF32 uv2 = pos_ijDst[n];
                if( max < v ) max = v;
                avg += v;
            }
        }
        pos_ijDst.clear();
        pos_ijSrc.clear();
        mnpoints += npoints;
    }
    uvMap.clear();
    if(!skipped)
    {
        avg = avg / mnpoints;
        EXPECT_LE(avg, m_avg_err);
        EXPECT_LE(max, m_max_err);
        if( avg > m_avg_err || max > m_max_err)
        {
            std::basic_ostringstream<wchar_t> stream;
            stream << L"FAIL: " << rsformatToString(m_formats.at(rs::stream::color)) << " " << m_color_intrin.width << "x" << m_color_intrin.height << "; ";
            stream << rsformatToString(m_formats.at(rs::stream::depth)) << " " << m_depth_intrin.width << "x" << m_depth_intrin.height << "; ";
            stream << "File: " << projection_tests_util::file_name.c_str() << "; m_avg_error[pxls]=" << avg << "; m_max_error[pxls]=" << max;
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, stream.str().c_str(), __FILE__, __LINE__, "query_uvmap_map_depth_to_color");
            m_is_failed = true;
        }
    }

}


/*
    Test:
        query_invuvmap_map_color_to_depth

    Target:
        Checks compatibility of QueryInvUVMap and MapColorToDepth

    Scope:
        All available '.rssdk' files from PROJECTION folder with different aspect ratios and serialized projection data

    Description:
        From a Color image picks some amount of pixels and calls MapColorToDepth.
        Also gets Inverse UV Map with QueryInvUVMap. An expectation is to have Inverse UV Map
        coordinates near the results of mapped Color pixel coordinates with MapColorToDepth.
        An error gets as distance in pixels of mapped Color coordinates.

    Pass Criteria:
        Test passes if average error and maximal error less than threshold for all frames.
*/
TEST_F(projection_fixture, query_invuvmap_map_color_to_depth)
{
    m_avg_err = 1.f;
    m_max_err = 1.f;
    m_points_max = 1000;
    const int32_t numFrames = 2;
    float avg = 0.f, max = 0.f;
    int32_t mnpoints = 0;
    bool skipped = false;
    std::vector<rs::core::pointF32> invUvMap(m_color_intrin.width * m_color_intrin.height);

    int depthWidth, depthHeight;
    depthWidth = m_depth_intrin.width;
    depthHeight = m_depth_intrin.height;

    for (int i = m_device->get_frame_count() - numFrames; i < m_device->get_frame_count(); i++)
    {
        m_device->set_frame_by_index(i, rs::stream::depth);

        int depthPitch = depthWidth * 2;
        image_info  DepthInfo = {depthWidth, depthHeight, convert_pixel_format(projection_tests_util::depth_format), depthPitch};

        custom_image depth (&DepthInfo,
                            m_device->get_frame_data(rs::stream::depth),
                            stream_type::depth,
                            image_interface::flag::any,
                            m_device->get_frame_timestamp(rs::stream::depth),
                            nullptr,
                            nullptr);


        /* Retrieve the depth pixels */
        const void * ddata = depth.query_data();
        ASSERT_FALSE(!ddata);

        /* Get Inverse UV Map */
        m_sts = m_projection->query_invuvmap(&depth, &invUvMap[0]);
        if( m_sts == status_param_unsupported )
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, L"Unable to QueryInvUVMap", __FILE__, __LINE__, "query_invuvmap_map_color_to_depth");
            ASSERT_EQ(m_sts, status_no_error);
        }

        // Choose valid points from a Depth
        std::vector<rs::core::pointF32> pos_ijSrc;
        int32_t npoints = 0;
        for (int32_t y = m_color_intrin.height/2-20; y < m_color_intrin.height/2+20; y++)
        {
            for (int32_t x = m_color_intrin.width/2-20; x < m_color_intrin.width/2+20; x++)
            {
                rs::core::pointF32 pos_ijSrcTmp = {(float)x, (float)y};
                pos_ijSrc.push_back(pos_ijSrcTmp);
                npoints ++;
                break;
            }
            if( npoints >= m_points_max ) break;
        }


        // Find a Color map of the choosen points
        std::vector<rs::core::pointF32> pos_ijDst(npoints);
        m_sts = m_projection->map_color_to_depth(&depth, npoints, &pos_ijSrc[0], &pos_ijDst[0]);
        if( m_sts == status_param_unsupported )
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, L"Unable to MapColorToDepth", __FILE__, __LINE__, "query_invuvmap_map_color_to_depth");
            ASSERT_EQ(m_sts, status_no_error);
        }

        /* Check coordinates */
        for(int32_t n = 0; n < npoints; n++ )
        {
            rs::core::pointF32 invuv = invUvMap[pos_ijSrc[n].y*m_color_intrin.width+pos_ijSrc[n].x];
            if( pos_ijDst[n].x >= 0.f && pos_ijDst[n].y >= 0.f  && invuv.x >= 0.f )
            {
                invuv.x *= m_color_intrin.width;
                invuv.y *= m_color_intrin.height;
                float v = distancePixels( pos_ijDst[n], invuv );
                if( max < v ) max = v;
                avg += v;
            }
        }
        mnpoints += npoints;
        pos_ijDst.clear();
        pos_ijSrc.clear();
    }
    invUvMap.clear();
    if( !skipped )
    {
        avg = avg / mnpoints;
        EXPECT_LE(avg, m_avg_err);
        EXPECT_LE(max, m_max_err);
        if( avg > m_avg_err || max > m_max_err)
        {
            std::basic_ostringstream<wchar_t> stream;
            stream << L"FAIL: " << rsformatToString(m_formats.at(rs::stream::color)) << " " << m_color_intrin.width << "x" << m_color_intrin.height << "; ";
            stream << rsformatToString(m_formats.at(rs::stream::depth)) << " " << m_depth_intrin.width << "x" << m_depth_intrin.height << "; ";
            stream << "File: " << projection_tests_util::file_name.c_str() << "; m_avg_error[pxls]=" << avg << "; m_max_error[pxls]=" << max;
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, stream.str().c_str(), __FILE__, __LINE__, "query_invuvmap_map_color_to_depth");
            m_is_failed = true;
        }
    }

}


/*
    Test:
        query_vertices_project_depth_to_camera

    Target:
        Checks compatibility of QueryVertices and ProjectDepthToCamera

    Scope:
        1. All available '.rssdk' files from PROJECTION folder with different aspect ratios and serialized projection data
        2. All valid Depth values

    Description:
        From a Depth image picks all valid pixels and calls ProjectDepthToCamera.
        Also gets vertices from a Depth image with QueryInvUVMap. An expectation is to have vertices
        coordinates near the results of projected Depth pixel coordinates with ProjectDepthToCamera.
        An error gets as distance in mm.

    Pass Criteria:
        Test passes if average error and maximal error less than threshold for all frames.
*/
TEST_F(projection_fixture, query_vertices_project_depth_to_camera)
{
    m_avg_err = 2.f;
    m_max_err = 3.f;

    const int32_t numFrames = 2;
    //TODO: what is invalid_value?
    uint16_t invalid_value = m_device->get_option(rs::option::r200_depth_control_score_minimum_threshold);
    float avg = 0.f, max = 0.f;
    int32_t mnpoints = 0;
    bool skipped = false;
    std::vector<rs::core::point3dF32> pos_ijDst1(m_color_intrin.width*m_color_intrin.height);

    int depthWidth, depthHeight;
    depthWidth = m_depth_intrin.width;
    depthHeight = m_depth_intrin.height;

    for (int i = m_device->get_frame_count() - numFrames; i < m_device->get_frame_count(); i++)
    {
        m_device->set_frame_by_index(i, rs::stream::depth);

        int depthPitch = depthWidth * 2;
        image_info  DepthInfo = {depthWidth, depthHeight, convert_pixel_format(projection_tests_util::depth_format), depthPitch};

        custom_image depth (&DepthInfo,
                            m_device->get_frame_data(rs::stream::depth),
                            stream_type::depth,
                            image_interface::flag::any,
                            m_device->get_frame_timestamp(rs::stream::depth),
                            nullptr,
                            nullptr);


        /* Retrieve the depth pixels */
        const uint8_t * ddata = (uint8_t*)depth.query_data();
        ASSERT_FALSE(!ddata);


        // Get QueryVertices
        m_sts = m_projection->query_vertices(&depth, &pos_ijDst1[0]);
        if( m_sts == status_param_unsupported )
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, L"Unable to QueryVertices", __FILE__, __LINE__, "query_vertices_project_depth_to_camera");
            ASSERT_EQ(m_sts, status_no_error);
        }

        int32_t npoints = 0;
        std::vector<rs::core::point3dF32> pos_ijSrc;
        for (int32_t y = 0; y < m_color_intrin.height; y++)
        {
            uint16_t *d = (uint16_t*)(ddata + y * depth.query_info().pitch);
            for (int32_t x = 0; x < m_color_intrin.width; x++)
            {
                if (d[x] == invalid_value || d[x] > MAX_DISTANCE) continue; // no mapping based on unreliable depth values
                rs::core::point3dF32 pos_ijSrcTmp = {(float)x, (float)y, (float)d[x]};
                pos_ijSrc.push_back(pos_ijSrcTmp);
                npoints ++;
            }
        }

        /* Get  ProjectDepthToCamera */
        std::vector<rs::core::point3dF32> pos_ijDst2(npoints);
        m_sts = m_projection->project_depth_to_camera(npoints, &pos_ijSrc[0], &pos_ijDst2[0]);
        if( m_sts == status_param_unsupported )
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, L"Unable to ProjectDepthToCamera", __FILE__, __LINE__, "query_vertices_project_depth_to_camera");
            ASSERT_EQ(m_sts, status_no_error);
        }

        /* Check coordinates */
        for(int32_t n = 0; n < npoints; n++ )
        {
            rs::core::point3dF32 vertex = pos_ijDst1[pos_ijSrc[n].y*m_color_intrin.width+pos_ijSrc[n].x];
            if( !(vertex.x >= 0.f && vertex.y >= 0.f && pos_ijDst2[n].x >= 0.f && pos_ijDst2[n].y >= 0.f) ) continue;
            float v = distance3d( vertex, pos_ijDst2[n] );
            if( max < v ) max = v;
            avg += v;
        }
        mnpoints += npoints;
        pos_ijDst2.clear();
        pos_ijSrc.clear();
    }
    pos_ijDst1.clear();
    if( !skipped )
    {
        avg = avg / mnpoints;
        EXPECT_LE(avg, m_avg_err);
        EXPECT_LE(max, m_max_err);
        if( avg > m_avg_err )
        {
            std::basic_ostringstream<wchar_t> stream;
            stream << L"FAIL: " << rsformatToString(m_formats.at(rs::stream::color)) << " " << m_color_intrin.width << "x" << m_color_intrin.height << "; ";
            stream << rsformatToString(m_formats.at(rs::stream::depth)) << " " << m_depth_intrin.width << "x" << m_depth_intrin.height << "; ";
            stream << "File: " << projection_tests_util::file_name.c_str() << "; m_avg_error[mm]=" << avg << "; m_max_error[mm]=" << max;
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, stream.str().c_str(), __FILE__, __LINE__, "query_vertices_project_depth_to_camera");
            m_is_failed = true;
        }
    }
}


/*
    Test:
        query_uvmap_query_invuvmap

    Target:
        Checks compatibility of QueryInvUVMap and MapColorToDepth

    Scope:
        All available '.rssdk' files from PROJECTION folder with different aspect ratios and serialized projection data

    Description:
        Gets UV Map with QueryUVMap, and Inverse UV Map with QueryInvUVMap.
        An expectation is to have source Depth image pixel coordinates near the results
        of mapped back pixel coordinates with QueryInvUVMap.
        An error gets as distance in pixels of mapped Depth coordinates.

    Pass Criteria:
        Test passes if average error and maximal error less than threshold for all frames.
*/
TEST_F(projection_fixture, query_uvmap_query_invuvmap)
{
    m_avg_err = 3.f;
    m_max_err = 6.f;

    const int32_t numFrames = 2;
    float avg = 0.f, max = 0.f;
    int32_t npoints = 0;
    bool skipped = false;

    int depthWidth, depthHeight;
    depthWidth = m_depth_intrin.width;
    depthHeight = m_depth_intrin.height;

    for (int i = m_device->get_frame_count() - numFrames; i < m_device->get_frame_count(); i++)
    {
        m_device->set_frame_by_index(i, rs::stream::depth);

        int depthPitch = depthWidth * 2;
        image_info  DepthInfo = {depthWidth, depthHeight, convert_pixel_format(projection_tests_util::depth_format), depthPitch};

        custom_image depth (&DepthInfo,
                            m_device->get_frame_data(rs::stream::depth),
                            stream_type::depth,
                            image_interface::flag::any,
                            m_device->get_frame_timestamp(rs::stream::depth),
                            nullptr,
                            nullptr);


        /* Retrieve the depth pixels */
        const void * ddata = depth.query_data();
        ASSERT_FALSE(!ddata);


        /* Get UV Map */
        std::vector<rs::core::pointF32> uvMap(m_color_intrin.width * m_color_intrin.height);
        m_sts = m_projection->query_uvmap(&depth, &uvMap[0]);
        if( m_sts == status_param_unsupported )
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, L"Unable to QueryUVMap", __FILE__, __LINE__, "query_uvmap_query_invuvmap");
            ASSERT_EQ(m_sts, status_no_error);
        }

        /* Get Inverse UV Map */
        std::vector<rs::core::pointF32> invUvMap(m_color_intrin.width * m_color_intrin.height);
        m_sts = m_projection->query_invuvmap(&depth, &invUvMap[0]);
        if( m_sts == status_param_unsupported )
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, L"Unable to QueryInvUVMap", __FILE__, __LINE__, "query_uvmap_query_invuvmap");
            ASSERT_EQ(m_sts, status_no_error);
        }

        /* Check coordinates */
        for (int32_t y = 0; y < m_color_intrin.height; y++)
        {
            for (int32_t x = 0; x < m_color_intrin.width; x++)
            {
                rs::core::pointF32 uv = uvMap[y*m_color_intrin.width+x];
                if( !(uv.x >= 0.f && uv.y >= 0.f && uv.x < 1.f && uv.y < 1.f) ) continue;
                uv.x *= m_color_intrin.width;
                uv.y *= m_color_intrin.height;
                rs::core::pointF32 invuv = invUvMap[m_color_intrin.width*(int)uv.y+(int)uv.x];
                if( invuv.x < 0.f ) continue;
                rs::core::pointF32 src = {(float)x, (float)y};
                invuv.x = invuv.x * m_color_intrin.width;
                invuv.y = invuv.y * m_color_intrin.height;
                float v = distancePixels( src, invuv );
                if( max < v ) max = v;
                avg += v;
                npoints ++;
            }
        }
    }
    if( !skipped )
    {
        avg = avg / npoints;
        EXPECT_LE(avg, m_avg_err);
        if( avg > m_avg_err )//|| max > m_max_err)
        {
            std::basic_ostringstream<wchar_t> stream;
            stream << L"FAIL: " << rsformatToString(m_formats.at(rs::stream::color)) << " " << m_color_intrin.width << "x" << m_color_intrin.height << "; ";
            stream << rsformatToString(m_formats.at(rs::stream::depth)) << " " << m_depth_intrin.width << "x" << m_depth_intrin.height << "; ";
            stream << "File: " << projection_tests_util::file_name.c_str() << "; m_avg_error[pxls]=" << avg;
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, stream.str().c_str(), __FILE__, __LINE__, "query_uvmap_query_invuvmap");
            m_is_failed = true;
        }
    }
}


/*
    Test:
        create_depth_image_mapped_to_color_query_invuvmap

    Target:
        Checks compatibility of QueryInvUVMap and MapColorToDepth

    Scope:
        All available '.rssdk' files from PROJECTION folder with different aspect ratios and serialized projection data

    Description:
        Gets Inverse UV Map with QueryInvUVMap, and also creates Depth mapped to Color image with CreateDepthImageMappedToColor
        An expectation is to have exactely the same Depth values on the image and [picked from a Depth Image with QueryInvUVMap.
        An error gets as percent of inequal Depth values.

    Pass Criteria:
        Test passes if average error and maximal error less than threshold for all frames.
*/
/* The method tested is currently unavailable
TEST_F(projection_fixture, create_depth_image_mapped_to_color_query_invuvmap)
{
    m_avg_err = 1.f;
    m_max_err = 1.f;

    const int32_t numFrames = 2;
    //TODO: what is invalid_value?
    uint16_t invalid_value = m_device->get_option(rs::option::r200_depth_control_score_minimum_threshold);

    float avg = 0.f;
    int32_t npoints = 0;
    bool skipped = false;
    Image::ImageInfo depthInfo, d2cInfo;

    int depthWidth, depthHeight, colorWidth, colorHeight;
    depthWidth = m_depth_intrin.width;
    depthHeight = m_depth_intrin.height;
    colorWidth = m_color_intrin.width;
    colorHeight = m_color_intrin.height;

    m_device->start();
    for(int32_t i = 0; i < numFrames; i++)
    {
        m_device->set_frame_by_index(i);

        void *depthData, *colorData;
        int depthPitch, colorPitch;
        uint64_t depthTimestamp, colorTimestamp;
        depthData = (void*)m_device->get_frame_data(rs::stream::depth);
        colorData = (void*)m_device->get_frame_data(rs::stream::color);
        depthPitch = depthWidth * 2;
        colorPitch = colorWidth * 4;
        depthTimestamp = m_device->get_frame_timestamp(rs::stream::depth);
        colorTimestamp = m_device->get_frame_timestamp(rs::stream::color);
        ReadOnlyImage depth(StreamType::STREAM_TYPE_DEPTH, Image::PIXEL_FORMAT_DEPTH, depthWidth, depthHeight, depthData, depthPitch, depthTimestamp);
        ReadOnlyImage color(StreamType::STREAM_TYPE_COLOR, Image::PIXEL_FORMAT_RGB32, colorWidth, colorHeight, colorData, colorPitch, colorTimestamp);
        Sample sample;
        sample.depth = &depth;
        sample.color = &color;

        /* Get Inverse UV Map */
/*        std::vector<rs::core::pointF32> invUvMap(m_color_intrin.width * m_color_intrin.height);
        m_sts = m_projection->QueryInvUVMap(sample.depth, &invUvMap[0]);
        if( m_sts == STATUS_PARAM_UNSUPPORTED ) {
            skipped = true;
        } else if(m_sts < STATUS_NO_ERROR) {
            //std::cerr << "Unable to QueryInvUVMap\n";
            ASSERT_EQ(m_sts, STATUS_NO_ERROR);
        }

        /* Get CreateDepthImageMappedToColor */
/*        Image::ImageData d2cDat;
        Image* d2c = m_projection->CreateDepthImageMappedToColor(sample.depth, sample.color);
        ASSERT_NE(d2c, nullptr);

        /* Check image m_formats */
/*        depthInfo = sample.depth->QueryInfo();
        d2cInfo = d2c->QueryInfo();

        /* Check coordinates */
/* Retrieve the depth pixels */
/*        Image::ImageData ddata;
        ASSERT_EQ(sample.depth->AcquireAccess(Image::ACCESS_READ, Image::PIXEL_FORMAT_DEPTH, &ddata), STATUS_NO_ERROR);
        ASSERT_EQ(d2c->AcquireAccess(Image::ACCESS_READ, Image::PIXEL_FORMAT_DEPTH, &d2cDat), STATUS_NO_ERROR);

        for (int32_t y = 0; y < m_color_intrin.height; y++) {
            for (int32_t x = 0; x < m_color_intrin.width; x++) {
                rs::core::pointF32 invuv = invUvMap[y*m_color_intrin.width+x];
                invuv.x = invuv.x * m_color_intrin.width + 0.5;
                invuv.y = invuv.y * m_color_intrin.height + 0.5;
                if(  invuv.x < 0.f || invuv.y < 0.f || invuv.x >= m_color_intrin.width || invuv.y >= m_color_intrin.height ) continue;
                uint16_t d1 = ((uint16_t*)(ddata.planes[0]+(int)invuv.y*ddata.pitches[0]))[(int)invuv.x];
                uint16_t d2 = ((uint16_t*)(d2cDat.planes[0]+y*d2cDat.pitches[0]))[x];
                if (d1 == invalid_value || d2 == invalid_value) continue; // no mapping based on unreliable depth values
                if( 0 != abs(d1 - d2) )
                    avg ++;
                npoints ++;
            }
        }

        d2c->ReleaseAccess(&d2cDat);
        d2c->Release();
        sample.depth->ReleaseAccess(&ddata);

    }
    if( !skipped ) {
        avg = 100 * avg / npoints;
        if( avg > m_avg_err ) {
            //std::cout << "FAIL: "<<rsformatToString(m_formats.at(rs::stream::color)) <<" "<<m_color_intrin.width<<"x"<<m_color_intrin.height<<"; ";
            //std::cout << rsformatToString(m_formats.at(rs::stream::depth)) <<" "<<m_color_intrin.width<<"x"<<m_color_intrin.height<<"; ";
            //std::cout << "File "<<projection_tests_util::file_name<<"; ";
            //std::cout << "DEPTH: "<<rsformatToString(depthInfo.format)<< "; D2C: "<<rsformatToString( d2cInfo.format )<<"; ";
            //std::cout <<"m_avg_error "<<avg<<"%\n";
            m_is_failed = true;
        } else {
            //std::cout << "PASS: "<<rsformatToString(m_formats.at(rs::stream::color)) <<" "<<m_color_intrin.width<<"x"<<m_color_intrin.height<<"; ";
            //std::cout << rsformatToString(m_formats.at(rs::stream::depth)) <<" "<<m_color_intrin.width<<"x"<<m_color_intrin.height<<"; ";
            //std::cout << "File "<<projection_tests_util::file_name<<"; ";
            //std::cout <<"m_avg_error "<<avg<<"%\n";
        }
    }

}
*/


/*
    Test:
        create_color_image_mapped_to_depth_query_uvmap

    Target:
        Checks compatibility of CreateColorImageMappedToDepth and QueryUVMap

    Scope:
        All available '.rssdk' files from PROJECTION folder with different aspect ratios and serialized projection data

    Description:
        Gets UV Map with QueryUVMap, and also creates Color mapped to Depth image with CreateColorImageMappedToDepth
        An expectation is to have exactely the same Color values on the image and picked from a Color Image with QueryUVMap.
        An error gets as percent of inequal Color values.

    Pass Criteria:
        Test passes if average error and maximal error less than threshold for all frames.
*/
/* The method tested is currently unavailable
TEST_F(projection_fixture, create_color_image_mapped_to_depth_query_uvmap)
{
    m_avg_err = 1.f;
    m_max_err = 1.f;

    const int32_t numFrames = 2;
    float avg = 0.f;
    int32_t npoints = 0;
    int32_t colorComponents = 1;
    bool skipped = false;

    int depthWidth, depthHeight, colorWidth, colorHeight;
    depthWidth = m_depth_intrin.width;
    depthHeight = m_depth_intrin.height;
    colorWidth = m_color_intrin.width;
    colorHeight = m_color_intrin.height;

    m_device->start();
    for(int32_t i = 0; i < numFrames; i++)
    {
        m_device->set_frame_by_index(i);

        void *depthData, *colorData;
        int depthPitch, colorPitch;
        uint64_t depthTimestamp, colorTimestamp;
        depthData = (void*)m_device->get_frame_data(rs::stream::depth);
        colorData = (void*)m_device->get_frame_data(rs::stream::color);
        depthPitch = depthWidth * 2;
        colorPitch = colorWidth * 4;
        depthTimestamp = m_device->get_frame_timestamp(rs::stream::depth);
        colorTimestamp = m_device->get_frame_timestamp(rs::stream::color);
        ReadOnlyImage depth(StreamType::STREAM_TYPE_DEPTH, Image::PIXEL_FORMAT_DEPTH, depthWidth, depthHeight, depthData, depthPitch, depthTimestamp);
        ReadOnlyImage color(StreamType::STREAM_TYPE_COLOR, Image::PIXEL_FORMAT_RGB32, colorWidth, colorHeight, colorData, colorPitch, colorTimestamp);
        Sample sample;
        sample.depth = &depth;
        sample.color = &color;

        /* Get uvmap */
/*        std::vector<rs::core::pointF32> uvMap(m_color_intrin.width * m_color_intrin.height);
        m_sts = m_projection->QueryUVMap(sample.depth, &uvMap[0]);
        if( m_sts == STATUS_PARAM_UNSUPPORTED ) {
            skipped = true;
        } else if(m_sts < STATUS_NO_ERROR) {
            //std::cerr << "Unable to QueryUVMap\n";
            ASSERT_EQ(m_sts, STATUS_NO_ERROR);
        }

        /* Get CreateColorImageMappedToDepth */
/*        Image::ImageData c2dDat;
        Image* c2d = m_projection->CreateColorImageMappedToDepth(sample.depth, sample.color);
        ASSERT_NE(c2d, nullptr);

        /* Check coordinates */
/*        c2d->AcquireAccess(Image::ACCESS_READ, &c2dDat);
        ASSERT_EQ(c2d->AcquireAccess(Image::ACCESS_READ, &c2dDat), STATUS_NO_ERROR);

        /* Retrieve the color pixels */
/*        Image::ImageInfo c2dInfo = c2d->QueryInfo();
        Image::ImageData cdata;
        ASSERT_EQ(sample.color->AcquireAccess(Image::ACCESS_READ, c2dInfo.format, &cdata), STATUS_NO_ERROR);

        if( c2dInfo.format == Image::PIXEL_FORMAT_RGB24 ) colorComponents = 3;
        else if( c2dInfo.format == Image::PIXEL_FORMAT_RGB32 ) colorComponents = 4;
        for (int32_t y = 0; y < m_color_intrin.height; y++) {
            for (int32_t x = 0; x < m_color_intrin.width; x++) {
                rs::core::pointF32 uv = uvMap[y*m_color_intrin.width+x];
                if( uv.x >= 0.f && uv.y >= 0.f && uv.x < 1.f && uv.y < 1.f) {
                    uv.x *= m_color_intrin.width;
                    uv.y *= m_color_intrin.height;
                    for (int32_t cc = 0; cc < colorComponents; cc++) {
                        if( 0 != abs(cdata.planes[0][(int)uv.y * cdata.pitches[0] + (int)uv.x * colorComponents+cc] - c2dDat.planes[0][y*c2dDat.pitches[0]+x*colorComponents+cc]) ) {
                            avg ++;
                        }
                    }
                    npoints ++;
                }
            }
        }
        c2d->ReleaseAccess(&c2dDat);
        c2d->Release();
        sample.color->ReleaseAccess(&cdata);

    }
    if( !skipped ) {
        avg = 100 * avg / colorComponents / npoints;
        if( avg > m_avg_err ) {
            //std::cout << "FAIL: "<<rsformatToString(m_formats.at(rs::stream::color)) <<" "<<m_color_intrin.width<<"x"<<m_color_intrin.height<<"; ";
            //std::cout << rsformatToString(m_formats.at(rs::stream::depth)) <<" "<<m_color_intrin.width<<"x"<<m_color_intrin.height<<"; ";
            //std::cout << "File "<<projection_tests_util::file_name<<"; ";
            //std::cout <<"m_avg_error "<<avg<<"%\n";
            m_is_failed = true;
        } else {
            //std::cout << "PASS: "<<rsformatToString(m_formats.at(rs::stream::color)) <<" "<<m_color_intrin.width<<"x"<<m_color_intrin.height<<"; ";
            //std::cout << rsformatToString(m_formats.at(rs::stream::depth)) <<" "<<m_color_intrin.width<<"x"<<m_color_intrin.height<<"; ";
            //std::cout << "File "<<projection_tests_util::file_name<<"; ";
            //std::cout <<"m_avg_error "<<avg<<"%\n";
        }
    }
}
*/
