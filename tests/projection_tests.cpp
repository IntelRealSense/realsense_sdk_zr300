// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "projection_fixture.h"
#include <stdio.h>
#include <stdlib.h>
#include <locale>
#include <algorithm>
#include "rs/utils/librealsense_conversion_utils.h"
#include "rs/utils/smart_ptr_helpers.h"
#include "image/librealsense_image_utils.h"

#ifdef WIN32
#define NOMINMAX
#else
#include <dirent.h>
#endif

using namespace rs::core;
using namespace rs::utils;

static point3dF32 world3dSrc[CUBE_VERTICES];
static const point3dF32 cube100mm[CUBE_VERTICES] =
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

__inline point3dF32 *cube100mmWorldTr(float trX, float trY, float trZ)
{
    for( int n=0; n<CUBE_VERTICES; n++ )
    {
        world3dSrc[n].x = cube100mm[n].x + trX;
        world3dSrc[n].y = cube100mm[n].y + trY;
        world3dSrc[n].z = cube100mm[n].z + trZ;
    }
    return &world3dSrc[0];
}

__inline float distance3d(point3dF32 v1, point3dF32 v2)
{
    return sqrtf( (v1.x - v2.x)*(v1.x - v2.x) +
                  (v1.y - v2.y)*(v1.y - v2.y) +
                  (v1.z - v2.z)*(v1.z - v2.z) );
}

__inline float distancePixels(pointF32 v1, pointF32 v2)
{
#if WIN32
	return std::max(fabs(v1.x - v2.x), fabs(v1.y - v2.y));
#else
    return static_cast<float>(std::max(fabs(static_cast<double>(v1.x - v2.x)), fabs(static_cast<double>(v1.y - v2.y))));
#endif
}
static const wchar_t* rsformatToWString(rs::format format)
{
    switch(format)
    {
        case rs::format::any:                       return L"UNKNOWN";
        case rs::format::bgra8:                     return L"bgra8";
        case rs::format::rgba8:                     return L"rgba8";
        case rs::format::bgr8:                      return L"bgr8";
        case rs::format::rgb8:                      return L"rgb8";
        case projection_tests_util::depth_format:   return L"z16";
        case rs::format::disparity16:               return L"disparity16";
        case rs::format::y8:                        return L"y8";
        case rs::format::y16:                       return L"y16";
        case rs::format::yuyv:                      return L"yuyv";
        default:                                    return L"Incorrect Pixel Format";
    };
}

__inline sizeI32 imSize(int32_t w, int32_t h) {sizeI32 sz = {w, h}; return sz;}


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
    point3dF32 pos3dDst[CUBE_VERTICES], pos_ijSrc[CUBE_VERTICES];
    pointF32 pos_ijDst[CUBE_VERTICES];
    for(int32_t dd = 0; dd < (int32_t)m_distances.size(); dd++)
    {
        point3dF32 *pos3dSrc = cube100mmWorldTr( 0.f, 0.f, m_distances[dd] );
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
            stream << L"FAIL: " << rsformatToWString(m_formats.at(rs::stream::color)) << " " << m_color_intrin.width << "x" << m_color_intrin.height << "; "
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

    point3dF32 pos3dDst[CUBE_VERTICES], pos_ijSrc[CUBE_VERTICES];
    pointF32 pos_ijDst[CUBE_VERTICES];
    for(int32_t dd = 0; dd < (int32_t)m_distances.size(); dd++)
    {
        point3dF32 *pos3dSrc = cube100mmWorldTr( 0.f, 0.f, m_distances[dd] );
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
            stream << L"FAIL: " << rsformatToWString(m_formats.at(rs::stream::depth)) << " " << m_depth_intrin.width << "x" << m_depth_intrin.height << "; "
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
        point3dF32 pos_uvzSrc[9] =
        {
            {static_cast<float>(m_color_intrin.width)/2.f, static_cast<float>(m_color_intrin.height)/2.f, m_distances[dd]},
            {5.f, 5.f, m_distances[dd]},
            {(static_cast<float>(m_color_intrin.width)-5.f), 5.f, m_distances[dd]},
            {5.f, (static_cast<float>(m_color_intrin.height)-5.f), m_distances[dd]},
            {(static_cast<float>(m_color_intrin.width)-5.f), (static_cast<float>(m_color_intrin.height)-5.f), m_distances[dd]},
            {100.f, 100.f, m_distances[dd]},
            {(static_cast<float>(m_color_intrin.width)-100.f), 100.f, m_distances[dd]},
            {100.f, (static_cast<float>(m_color_intrin.height)-100.f), m_distances[dd]},
            {(static_cast<float>(m_color_intrin.width)-100.f), (static_cast<float>(m_color_intrin.height)-100.f), m_distances[dd]},
        };
        const int npoints = sizeof(pos_uvzSrc)/sizeof(pos_uvzSrc[0]);
        point3dF32 pos3dDst[npoints];
        pointF32 pos_uvzDst[npoints];
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
            stream << L"FAIL: " << rsformatToWString(m_formats.at(rs::stream::color)) << " " << m_color_intrin.width << "x" << m_color_intrin.height << "; "
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
        point3dF32 pos_uvzSrc[9] =
        {
            {static_cast<float>(m_color_intrin.width)/2.f, static_cast<float>(m_color_intrin.height)/2.f, m_distances[dd]},
            {5.f, 5.f, m_distances[dd]},
            {(static_cast<float>(m_color_intrin.width)-5.f), 5.f, m_distances[dd]},
            {5.f, (static_cast<float>(m_color_intrin.height)-5.f), m_distances[dd]},
            {(static_cast<float>(m_color_intrin.width)-5.f), (static_cast<float>(m_color_intrin.height)-5.f), m_distances[dd]},
            {100.f, 100.f, m_distances[dd]},
            {(static_cast<float>(m_color_intrin.width)-100.f), 100.f, m_distances[dd]},
            {100.f, (static_cast<float>(m_color_intrin.height)-100.f), m_distances[dd]},
            {(static_cast<float>(m_color_intrin.width)-100.f), (static_cast<float>(m_color_intrin.height)-100.f), m_distances[dd]},
        };
        const int npoints = sizeof(pos_uvzSrc)/sizeof(pos_uvzSrc[0]);
        point3dF32 pos3dDst[npoints];
        pointF32 pos_uvzDst[npoints];
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
            stream << L"FAIL: " << rsformatToWString(m_formats.at(rs::stream::depth)) << " " << m_depth_intrin.width << "x" << m_depth_intrin.height << "; "
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
    const int32_t skipped_frames_at_begin = 5;

    uint16_t invalid_value = 0;
    float avg = 0.f, max = 0.f;
    int32_t mnpoints = 0;
    bool skipped = false;

    int depthWidth = m_depth_intrin.width;
    int depthHeight = m_depth_intrin.height;

    for (int i = skipped_frames_at_begin; i < projection_tests_util::total_frames; i++)
    {
        m_device->set_frame_by_index(i, rs::stream::depth);

        int depthPitch = depthWidth * image_utils::get_pixel_size(m_device->get_stream_format(rs::stream::depth));
        image_info DepthInfo = {depthWidth, depthHeight, convert_pixel_format(projection_tests_util::depth_format), depthPitch};

        auto depth = get_unique_ptr_with_releaser(image_interface::create_instance_from_raw_data(&DepthInfo,
                            {m_device->get_frame_data(rs::stream::depth), nullptr},
                            stream_type::depth,
                            image_interface::flag::any,
                            m_device->get_frame_timestamp(rs::stream::depth),
                            m_device->get_frame_number(rs::stream::depth)));

        /* Retrieve the depth pixels */
        const uint8_t * ddata = (uint8_t*)depth->query_data();
        ASSERT_FALSE(!ddata);

        std::vector<point3dF32> pos_ijSrc;
        int32_t npoints = 0;
        for (int32_t y = 0; y < m_color_intrin.height; y++)
        {
            uint16_t *d = (uint16_t*)(ddata + y * depth->query_info().pitch);
            for (int32_t x = 0; x < m_color_intrin.width; x++)
            {
                if (d[x] == invalid_value || d[x] > MAX_DISTANCE) continue; // no mapping based on unreliable depth values
                point3dF32 pos_ijSrcTmp = {(float)x, (float)y, (float)d[x]};
                pos_ijSrc.push_back(pos_ijSrcTmp);
                npoints ++;
                break;
            }
            if(npoints >= m_points_max) break;
        }

        std::vector<pointF32> pos_ijDst1(npoints);
        std::vector<pointF32> pos_ijDst2(npoints);
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
        m_sts = m_projection->map_color_to_depth(depth.get(), npoints, &pos_ijDst1[0], &pos_ijDst2[0]);
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
                float v = static_cast<float>(std::max(fabs(pos_ijSrc[n].x - pos_ijDst2[n].x), fabs(pos_ijSrc[n].y - pos_ijDst2[n].y)));
                if( max < v ) max = v;
                avg += v;
            }
        }
        mnpoints += npoints;
    }
    if(!skipped)
    {
        ASSERT_NE(mnpoints, 0);
        avg = avg / static_cast<float>(mnpoints);
        EXPECT_LE(avg, m_avg_err);
        EXPECT_LE(max, m_max_err);
        if( avg > m_avg_err || max > m_max_err)
        {
            std::basic_ostringstream<wchar_t> stream;
            stream << L"FAIL: " << rsformatToWString(m_formats.at(rs::stream::color)) << " " << m_color_intrin.width << "x" << m_color_intrin.height << "; ";
            stream << rsformatToWString(m_formats.at(rs::stream::depth)) << " " << m_depth_intrin.width << "x" << m_depth_intrin.height << "; ";
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
    const int32_t skipped_frames_at_begin = 5;

    uint16_t invalid_value = 0;
    float avg = 0.f, max = 0.f;
    int32_t mnpoints = 0;
    bool skipped = false;

    for (int i = skipped_frames_at_begin; i < projection_tests_util::total_frames; i++)
    {
        m_device->set_frame_by_index(i, rs::stream::depth);

        int depthPitch = m_depth_intrin.width * image_utils::get_pixel_size(projection_tests_util::depth_format);
        image_info  DepthInfo = {m_depth_intrin.width, m_depth_intrin.height, convert_pixel_format(projection_tests_util::depth_format), depthPitch};

        auto depth = get_unique_ptr_with_releaser(image_interface::create_instance_from_raw_data(&DepthInfo,
                            {m_device->get_frame_data(rs::stream::depth), nullptr},
                            stream_type::depth,
                            image_interface::flag::any,
                            m_device->get_frame_timestamp(rs::stream::depth),
                            m_device->get_frame_number(rs::stream::depth)));


        /* Retrieve the depth pixels */
        const uint8_t * ddata = (uint8_t*)depth->query_data();
        ASSERT_FALSE(!ddata);
        std::vector<point3dF32> pos_ijSrc;
        for (int32_t y = 0; y < m_color_intrin.height; y++)
        {
            uint16_t *d = (uint16_t*)(ddata + y * depth->query_info().pitch);
            for (int32_t x = 0; x < m_color_intrin.width; x++)
            {
                if (d[x] == invalid_value || d[x] > MAX_DISTANCE) continue; // no mapping based on unreliable depth values
                point3dF32 pos_ijSrcTmp = {(float)x, (float)y, (float)d[x]};
                pos_ijSrc.push_back(pos_ijSrcTmp);
            }
        }

        int32_t npoints = static_cast<int32_t>(pos_ijSrc.size());
        std::vector<pointF32> pos_ijDst1(npoints);
        std::vector<point3dF32> pos_ijMid(npoints);
        std::vector<pointF32> pos_ijDst2(npoints);
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
    if(!skipped)
    {
        ASSERT_NE(mnpoints, 0);
        avg = avg / static_cast<float>(mnpoints);
        EXPECT_LE(avg, m_avg_err);
        EXPECT_LE(max, m_max_err);
        if( avg > m_avg_err || max > m_max_err)
        {
            std::basic_ostringstream<wchar_t> stream;
            stream << L"FAIL: " << rsformatToWString(m_formats.at(rs::stream::color)) << " " << m_color_intrin.width << "x" << m_color_intrin.height << "; ";
            stream << rsformatToWString(m_formats.at(rs::stream::depth)) << " " << m_depth_intrin.width << "x" << m_depth_intrin.height << "; ";
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
    const int32_t skipped_frames_at_begin = 5;
    m_avg_err = .6f;
    m_max_err = 4.f;
    m_points_max = 100;

    uint16_t invalid_value = 0;
    float avg = 0.f, max = 0.f;
    int32_t mnpoints = 0;
    bool skipped = false;

    for (int i = skipped_frames_at_begin; i < projection_tests_util::total_frames; i++)
    {
        m_device->set_frame_by_index(i, rs::stream::depth);

        int depthPitch = m_depth_intrin.width * image_utils::get_pixel_size(projection_tests_util::depth_format);
        image_info DepthInfo = {m_depth_intrin.width, m_depth_intrin.height, convert_pixel_format(projection_tests_util::depth_format), depthPitch};

        auto depth = get_unique_ptr_with_releaser(image_interface::create_instance_from_raw_data(&DepthInfo,
                            {m_device->get_frame_data(rs::stream::depth), nullptr},
                            stream_type::depth,
                            image_interface::flag::any,
                            m_device->get_frame_timestamp(rs::stream::depth),
                            m_device->get_frame_number(rs::stream::depth)));

        /* Retrieve the depth pixels */
        const uint8_t * ddata = (uint8_t*)depth->query_data();
        ASSERT_FALSE(!ddata);


        // Choose all points from a Depth
        std::vector<point3dF32> pos_ijSrc;
        for (int32_t y = 0; y < m_color_intrin.height; y++)
        {
            uint16_t *d = (uint16_t*)(ddata + y * depth->query_info().pitch);
            for (int32_t x = 0; x < m_color_intrin.width; x++)
            {
                if (d[x] == invalid_value || d[x] > MAX_DISTANCE) continue; // no mapping based on unreliable depth values
                point3dF32 pos_ijSrcTmp = {(float)x, (float)y, (float)d[x]};
                pos_ijSrc.push_back(pos_ijSrcTmp);
            }
        }

        // Find a Color map of the choosen points
        std::vector<pointF32> pos_ijSrc1(pos_ijSrc.size());
        m_sts = m_projection->map_depth_to_color(static_cast<int32_t>(pos_ijSrc.size()), &pos_ijSrc[0], &pos_ijSrc1[0]);
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
        std::vector<pointF32> pos_ijSrc2;
        std::vector<point3dF32> pos_ijSrc3;
        for (size_t n = 0; n < pos_ijSrc1.size(); n++)
        {
            if( pos_ijSrc1[n].x != -1.f && pos_ijSrc1[n].y != -1.f )
            {
                pos_ijSrc2.push_back(pos_ijSrc1[n]);
                point3dF32 pos_ijSrcTmp = {pos_ijSrc1[n].x, pos_ijSrc1[n].y, pos_ijSrc[n].z};
                pos_ijSrc3.push_back(pos_ijSrcTmp);
                npoints ++;
                if( npoints >= m_points_max ) break;
            }
        }
        pos_ijSrc.clear();
        pos_ijSrc1.clear();

        // Maps and projects back Color points to Depth
        std::vector<pointF32> pos_ijDst1(npoints);
        m_sts = m_projection->map_color_to_depth(depth.get(), npoints, &pos_ijSrc2[0], &pos_ijDst1[0]);
        if( m_sts == status_param_unsupported )
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, L"Unable to MapColorToDepth", __FILE__, __LINE__, "map_color_camera_depth");
            ASSERT_EQ(m_sts, status_no_error);
        }
        std::vector<point3dF32> pos_ijMid(npoints);
        std::vector<pointF32> pos_ijDst2(npoints);


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

        for(int32_t n = 0; n < npoints; n++ )
        {
            if( pos_ijDst1[n].x != -1.f && pos_ijDst2[n].x != -1.f )
            {
                float v = distancePixels(pos_ijDst1[n], pos_ijDst2[n]);
                if(max < v) max = v;
                avg += v;
            }
        }
        mnpoints += npoints;
    }
    if(!skipped)
    {
        ASSERT_NE(mnpoints, 0);
        avg = avg / static_cast<float>(mnpoints);
        EXPECT_LE(avg, m_avg_err);
        EXPECT_LE(max, m_max_err);
        if( avg > m_avg_err || max > m_max_err)
        {
            std::basic_ostringstream<wchar_t> stream;
            stream << L"FAIL: " << rsformatToWString(m_formats.at(rs::stream::color)) << " " << m_color_intrin.width << "x" << m_color_intrin.height << "; ";
            stream << rsformatToWString(m_formats.at(rs::stream::depth)) << " " << m_depth_intrin.width << "x" << m_depth_intrin.height << "; ";
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
    const int32_t skipped_frames_at_begin = 5;

    uint16_t invalid_value = 0;
    float avg = 0.f, max = 0.f;
    int32_t mnpoints = 0;
    bool skipped = false;
    std::vector<pointF32> uvMap(m_depth_intrin.width * m_depth_intrin.height);

    for (int i = skipped_frames_at_begin; i < projection_tests_util::total_frames; i++)
    {
        m_device->set_frame_by_index(i, rs::stream::depth);


        int depthPitch = m_depth_intrin.width * image_utils::get_pixel_size(projection_tests_util::depth_format);
        image_info  DepthInfo = {m_depth_intrin.width, m_depth_intrin.height, convert_pixel_format(projection_tests_util::depth_format), depthPitch};

        auto depth = get_unique_ptr_with_releaser(image_interface::create_instance_from_raw_data(&DepthInfo,
                            {m_device->get_frame_data(rs::stream::depth), nullptr},
                            stream_type::depth,
                            image_interface::flag::any,
                            m_device->get_frame_timestamp(rs::stream::depth),
                            m_device->get_frame_number(rs::stream::depth)));
        const uint8_t * ddata = (uint8_t*)depth->query_data();

        /* Get uvMap */
        m_sts = m_projection->query_uvmap(depth.get(), uvMap.data());
        if(m_sts == status_feature_unsupported)
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            ASSERT_EQ(m_sts, status_no_error);
        }

        /* Retrieve the depth pixels */

        // Choose valid points from a Depth
        std::vector<point3dF32> pos_ijSrc;
        int32_t npoints = 0;
        for (int32_t y = 0; y < m_depth_intrin.height; y++)
        {
            uint16_t *d = (uint16_t*)(ddata + y*depth->query_info().pitch);
            for (int32_t x = 0; x < m_depth_intrin.width; x++)
            {
                if (d[x] == invalid_value || d[x] > MAX_DISTANCE) continue; // no mapping based on unreliable depth values
                point3dF32 pos_ijSrcTmp = {(float)x, (float)y, (float)d[x]};
                pos_ijSrc.push_back(pos_ijSrcTmp);
                npoints ++;
            }
        }

        // Find a Color map of the choosen points
        std::vector<pointF32> pos_ijDst(npoints);
        m_sts = m_projection->map_depth_to_color(npoints, pos_ijSrc.data(), pos_ijDst.data());
        if( m_sts == status_param_unsupported )
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, L"Unable to MapDepthToColor", __FILE__, __LINE__, "query_uvmap_map_depth_to_color");
            ASSERT_EQ(m_sts, status_no_error);
        }

        for(int32_t n = 0; n < npoints; n++ )
        {
            pointF32 uv = uvMap[static_cast<int>(pos_ijSrc[n].y*static_cast<float>(m_depth_intrin.width) + pos_ijSrc[n].x)];
            if( pos_ijDst[n].x != -1.f && pos_ijDst[n].y != -1.f && uv.x >= 0.f && uv.y >= 0.f && uv.x < 1.f && uv.y < 1.f)
            {
                uv.x *= static_cast<float>(m_depth_intrin.width);
                uv.y *= static_cast<float>(m_depth_intrin.height);
                float v = distancePixels( pos_ijDst[n], uv );
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
        ASSERT_NE(mnpoints, 0);
        avg = avg / static_cast<float>(mnpoints);
        EXPECT_LE(avg, m_avg_err);
        EXPECT_LE(max, m_max_err);
        if( avg > m_avg_err || max > m_max_err)
        {
            std::basic_ostringstream<wchar_t> stream;
            stream << L"FAIL: " << rsformatToWString(m_formats.at(rs::stream::color)) << " " << m_color_intrin.width << "x" << m_color_intrin.height << "; ";
            stream << rsformatToWString(m_formats.at(rs::stream::depth)) << " " << m_depth_intrin.width << "x" << m_depth_intrin.height << "; ";
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
    const int32_t skipped_frames_at_begin = 5;
    float avg = 0.f, max = 0.f;
    int32_t mnpoints = 0;
    bool skipped = false;
    std::vector<pointF32> invUvMap(m_color_intrin.width * m_color_intrin.height);

    for (int i = skipped_frames_at_begin; i < projection_tests_util::total_frames; i++)
    {
        m_device->set_frame_by_index(i, rs::stream::depth);

        int depthPitch = m_depth_intrin.width * image_utils::get_pixel_size(m_device->get_stream_format(rs::stream::depth));
        image_info DepthInfo = {m_depth_intrin.width, m_depth_intrin.height, convert_pixel_format(projection_tests_util::depth_format), depthPitch};

        auto depth = get_unique_ptr_with_releaser(image_interface::create_instance_from_raw_data(&DepthInfo,
                            {m_device->get_frame_data(rs::stream::depth), nullptr},
                            stream_type::depth,
                            image_interface::flag::any,
                            m_device->get_frame_timestamp(rs::stream::depth),
                            m_device->get_frame_number(rs::stream::depth)));

        /* Get Inversed UV Map */
        m_sts = m_projection->query_invuvmap(depth.get(), &invUvMap[0]);
        if( m_sts == status_feature_unsupported )
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, L"Unable to QueryInvUVMap", __FILE__, __LINE__, "query_invuvmap_map_color_to_depth");
            ASSERT_EQ(m_sts, status_no_error);
        }

        // Choose valid points from a Depth
        std::vector<pointF32> pos_ijSrc;
        int32_t npoints = 0;
        for (int32_t y = m_color_intrin.height/2-20; y < m_color_intrin.height/2+20; y++)
        {
            for (int32_t x = m_color_intrin.width/2-20; x < m_color_intrin.width/2+20; x++)
            {
                pointF32 pos_ijSrcTmp = {(float)x, (float)y};
                pos_ijSrc.push_back(pos_ijSrcTmp);
                npoints ++;
                break;
            }
            if( npoints >= m_points_max ) break;
        }


        // Find a Color map of the choosen points
        std::vector<pointF32> pos_ijDst(npoints);
        m_sts = m_projection->map_color_to_depth(depth.get(), npoints, pos_ijSrc.data(), pos_ijDst.data());
        if( m_sts == status_param_unsupported )
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, L"Unable to MapColorToDepth", __FILE__, __LINE__, "query_invuvmap_map_color_to_depth");
            ASSERT_EQ(m_sts, status_no_error);
        }

        for(int32_t n = 0; n < npoints; n++ )
        {
            pointF32 invuv = invUvMap[static_cast<int>(pos_ijSrc[n].y*static_cast<float>(m_color_intrin.width)+pos_ijSrc[n].x)];
            if( pos_ijDst[n].x >= 0.f && pos_ijDst[n].y >= 0.f  && invuv.x >= 0.f )
            {
                invuv.x *= static_cast<float>(m_color_intrin.width);
                invuv.y *= static_cast<float>(m_color_intrin.height);
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
    if(!skipped)
    {
        ASSERT_NE(mnpoints, 0);
        avg = avg / static_cast<float>(mnpoints);
        EXPECT_LE(avg, m_avg_err);
        EXPECT_LE(max, m_max_err);
        if( avg > m_avg_err || max > m_max_err)
        {
            std::basic_ostringstream<wchar_t> stream;
            stream << L"FAIL: " << rsformatToWString(m_formats.at(rs::stream::color)) << " " << m_color_intrin.width << "x" << m_color_intrin.height << "; ";
            stream << rsformatToWString(m_formats.at(rs::stream::depth)) << " " << m_depth_intrin.width << "x" << m_depth_intrin.height << "; ";
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

    const int32_t skipped_frames_at_begin = 5;
    uint16_t invalid_value = 0;
    float avg = 0.f, max = 0.f;
    int32_t mnpoints = 0;
    bool skipped = false;
    std::vector<point3dF32> pos_ijDst1(m_depth_intrin.width*m_depth_intrin.height);

    for (int i = skipped_frames_at_begin; i < projection_tests_util::total_frames; i++)
    {
        m_device->set_frame_by_index(i, rs::stream::depth);

        int depthPitch = m_depth_intrin.width * image_utils::get_pixel_size(projection_tests_util::depth_format);
        image_info DepthInfo = {m_depth_intrin.width, m_depth_intrin.height, convert_pixel_format(projection_tests_util::depth_format), depthPitch};

        auto depth = get_unique_ptr_with_releaser(image_interface::create_instance_from_raw_data(&DepthInfo,
                            {m_device->get_frame_data(rs::stream::depth), nullptr},
                            stream_type::depth,
                            image_interface::flag::any,
                            m_device->get_frame_timestamp(rs::stream::depth),
                            m_device->get_frame_number(rs::stream::depth)));
        const uint8_t* ddata = (uint8_t*)depth->query_data();

        // Get QueryVertices
        m_sts = m_projection->query_vertices(depth.get(), pos_ijDst1.data());
        if( m_sts == status_feature_unsupported )
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, L"Unable to QueryVertices", __FILE__, __LINE__, "query_vertices_project_depth_to_camera");
            ASSERT_EQ(m_sts, status_no_error);
        }

        int32_t npoints = 0;
        std::vector<point3dF32> pos_ijSrc;
        for (int32_t y = 0; y < m_depth_intrin.height; y++)
        {
            uint16_t *d = (uint16_t*)(ddata + y*depth->query_info().pitch);
            for (int32_t x = 0; x < m_depth_intrin.width; x++)
            {
                if (d[x] == invalid_value || d[x] > MAX_DISTANCE) continue; // no mapping based on unreliable depth values
                point3dF32 pos_ijSrcTmp = {(float)x, (float)y, (float)d[x]};
                pos_ijSrc.push_back(pos_ijSrcTmp);
                npoints++;
            }
        }

        /* Get  ProjectDepthToCamera */
        std::vector<point3dF32> pos_ijDst2(npoints);
        m_sts = m_projection->project_depth_to_camera(npoints, pos_ijSrc.data(), pos_ijDst2.data());
        if( m_sts == status_param_unsupported )
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, L"Unable to ProjectDepthToCamera", __FILE__, __LINE__, "query_vertices_project_depth_to_camera");
            ASSERT_EQ(m_sts, status_no_error);
        }

        for(int32_t n = 0; n < npoints; n++ )
        {
            point3dF32 vertex = pos_ijDst1[static_cast<int>(pos_ijSrc[n].y*static_cast<float>(m_depth_intrin.width)+pos_ijSrc[n].x)];
            if( !(vertex.x >= 0.f && vertex.y >= 0.f && pos_ijDst2[n].x >= 0.f && pos_ijDst2[n].y >= 0.f) ) continue;
            float v = distance3d(vertex, pos_ijDst2[n]);
            if( max < v ) max = v;
            avg += v;
        }
        mnpoints += npoints;
        pos_ijDst2.clear();
        pos_ijSrc.clear();
    }
    pos_ijDst1.clear();
    if(!skipped)
    {
        ASSERT_NE(mnpoints, 0);
        avg = avg / static_cast<float>(mnpoints);
        EXPECT_LE(avg, m_avg_err);
        EXPECT_LE(max, m_max_err);
        if( avg > m_avg_err )
        {
            std::basic_ostringstream<wchar_t> stream;
            stream << L"FAIL: " << rsformatToWString(m_formats.at(rs::stream::color)) << " " << m_color_intrin.width << "x" << m_color_intrin.height << "; ";
            stream << rsformatToWString(m_formats.at(rs::stream::depth)) << " " << m_depth_intrin.width << "x" << m_depth_intrin.height << "; ";
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
TEST_F(projection_fixture, DISABLED_query_uvmap_query_invuvmap)
{
    m_avg_err = 3.f;
    m_max_err = 6.f;

    const int32_t skipped_frames_at_begin = 5;
    float avg = 0.f, max = 0.f;
    int32_t npoints = 0;
    bool skipped = false;

    for (int i = skipped_frames_at_begin; i < projection_tests_util::total_frames; i++)
    {
        m_device->set_frame_by_index(i, rs::stream::depth);

        int depthPitch = m_depth_intrin.width * image_utils::get_pixel_size(projection_tests_util::depth_format);
        image_info  DepthInfo = { m_depth_intrin.width, m_depth_intrin.height, convert_pixel_format(projection_tests_util::depth_format), depthPitch };

        auto depth = get_unique_ptr_with_releaser(image_interface::create_instance_from_raw_data(&DepthInfo,
                            {m_device->get_frame_data(rs::stream::depth), nullptr},
                            stream_type::depth,
                            image_interface::flag::any,
                            m_device->get_frame_timestamp(rs::stream::depth),
                            m_device->get_frame_number(rs::stream::depth)));
        /* Get UV Map */
        std::vector<pointF32> uvMap(m_depth_intrin.width * m_depth_intrin.height);
        m_sts = m_projection->query_uvmap(depth.get(), uvMap.data());
        if(m_sts == status_feature_unsupported)
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, L"Unable to QueryUVMap", __FILE__, __LINE__, "query_uvmap_query_invuvmap");
            ASSERT_EQ(m_sts, status_no_error);
        }

        /* Get Inverse UV Map */
        std::vector<pointF32> invUvMap(m_color_intrin.width * m_color_intrin.height);
        m_sts = m_projection->query_invuvmap(depth.get(), invUvMap.data());
        if(m_sts == status_feature_unsupported)
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, L"Unable to QueryInvUVMap", __FILE__, __LINE__, "query_uvmap_query_invuvmap");
            ASSERT_EQ(m_sts, status_no_error);
        }

        for (int32_t y = 0; y < m_depth_intrin.height; y++)
        {
            for (int32_t x = 0; x < m_depth_intrin.width; x++)
            {
                pointF32 uv = uvMap[y*m_depth_intrin.width+x];
                if (uv.x < 0.f || uv.x >= 1.f || uv.y < 0.f || uv.y >= 1.f) continue;
                uv.x *= static_cast<float>(m_depth_intrin.width); uv.x += 0.5f;
                uv.y *= static_cast<float>(m_depth_intrin.height); uv.y += 0.5f;
                pointF32 invuv = invUvMap[m_color_intrin.width*(int)uv.y+(int)uv.x];
                if(invuv.x < 0.f || invuv.x >= 1.f || invuv.y < 0.f || invuv.y >= 1.f) continue;
                invuv.x *= static_cast<float>(m_color_intrin.width);
                invuv.y *= static_cast<float>(m_color_intrin.height);
                pointF32 src = {(float)x, (float)y};
                float v = distancePixels( src, invuv );
                if(max < v) max = v;
                avg += v;
                npoints++;
            }
        }
    }
    if(!skipped)
    {
        ASSERT_NE(npoints, 0);
        avg = avg / static_cast<float>(npoints);
        EXPECT_LE(avg, m_avg_err);
        EXPECT_LE(max, m_max_err);
        if( avg > m_avg_err || max > m_max_err)
        {
            std::basic_ostringstream<wchar_t> stream;
            stream << L"FAIL: " << rsformatToWString(m_formats.at(rs::stream::color)) << " " << m_color_intrin.width << "x" << m_color_intrin.height << "; ";
            stream << rsformatToWString(m_formats.at(rs::stream::depth)) << " " << m_depth_intrin.width << "x" << m_depth_intrin.height << "; ";
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
TEST_F(projection_fixture, create_depth_image_mapped_to_color_query_invuvmap)
{
    m_avg_err = 1.f;
    m_max_err = 1.f;

    const int32_t skipped_frames_at_begin = 5;
    uint16_t invalid_value = 0;

    float avg = 0.f;
    int32_t npoints = 0;
    bool skipped = false;

    int depthPitch = m_depth_intrin.width * image_utils::get_pixel_size(projection_tests_util::depth_format);
    int colorPitch = m_color_intrin.width * image_utils::get_pixel_size(projection_tests_util::color_format);
    image_info depthInfo = { m_depth_intrin.width, m_depth_intrin.height, convert_pixel_format(projection_tests_util::depth_format), depthPitch };
    image_info colorInfo = { m_color_intrin.width, m_color_intrin.height, convert_pixel_format(projection_tests_util::color_format), colorPitch };

    m_device->start();
    for(int i = skipped_frames_at_begin; i < projection_tests_util::total_frames; i++)
    {
        m_device->set_frame_by_index(i, rs::stream::depth);

        const void* depthData = (void*)m_device->get_frame_data(rs::stream::depth);
        const void* colorData = (void*)m_device->get_frame_data(rs::stream::color);
        auto depth = get_unique_ptr_with_releaser(image_interface::create_instance_from_raw_data(&depthInfo,
                           {depthData, nullptr},
                           stream_type::depth,
                           image_interface::flag::any,
                           m_device->get_frame_timestamp(rs::stream::depth),
                           m_device->get_frame_number(rs::stream::depth)));
        auto color = get_unique_ptr_with_releaser(image_interface::create_instance_from_raw_data(&colorInfo,
                           {colorData, nullptr},
                           stream_type::color,
                           image_interface::flag::any,
                           m_device->get_frame_timestamp(rs::stream::color),
                           m_device->get_frame_number(rs::stream::color)));

        /* Get Inverse UV Map */
        std::vector<pointF32> invUvMap(colorInfo.width * colorInfo.height);
        m_sts = m_projection->query_invuvmap(depth.get(), invUvMap.data());
        if(m_sts == status_feature_unsupported)
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            ASSERT_EQ(m_sts, status_no_error);
        }

        auto depth2color  = get_unique_ptr_with_releaser(m_projection->create_depth_image_mapped_to_color(depth.get(), color.get()));
        ASSERT_NE(depth2color.get(), nullptr);
        ASSERT_NE(depth2color->query_data(), nullptr);
        const uint8_t* depth2color_data = (const uint8_t*)depth2color->query_data();
        image_info depth2colorInfo = depth2color->query_info();
        const uint8_t* depth_data = (const uint8_t*)depth->query_data();

        for (int32_t y = 0; y < m_depth_intrin.height; y++)
        {
            for (int32_t x = 0; x < m_depth_intrin.width; x++)
            {
                pointF32 invuv = invUvMap[y*m_color_intrin.width+x];
                if(invuv.x < 0.f || invuv.y < 0.f || invuv.x >= 1.f || invuv.y >= 1.f) continue;
                invuv.x *= static_cast<float>(m_color_intrin.width); invuv.x += 0.5f;
                invuv.y *= static_cast<float>(m_color_intrin.height); invuv.y += 0.5f;
                uint16_t d1 = (uint16_t)(depth_data + (int)invuv.y*depthInfo.pitch)[(int)invuv.x];
                uint16_t d2 = (uint16_t)(depth2color_data + y*depth2colorInfo.pitch)[x];
                if (d1 == invalid_value || d2 == invalid_value) continue;
                if(0 != abs(d1 - d2))
                {
                    avg++;
                }
                npoints++;
            }
        }
    }
    if(!skipped)
    {
        ASSERT_NE(npoints, 0);
        avg = avg / static_cast<float>(npoints);
        EXPECT_LE(avg, m_avg_err);
        if( avg > m_avg_err )
        {
            std::basic_ostringstream<wchar_t> stream;
            stream << L"FAIL: " << rsformatToWString(m_formats.at(rs::stream::color)) << " " << m_color_intrin.width << "x" << m_color_intrin.height << "; ";
            stream << rsformatToWString(m_formats.at(rs::stream::depth)) << " " << m_depth_intrin.width << "x" << m_depth_intrin.height << "; ";
            stream << "File: " << projection_tests_util::file_name.c_str() << "; avg_err= " << avg;
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, stream.str().c_str(), __FILE__, __LINE__, "create_depth_image_mapped_to_color_query_invuvmap");
            m_is_failed = true;
        }
    }

}


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
TEST_F(projection_fixture, create_color_image_mapped_to_depth_query_uvmap)
{
    m_avg_err = 1.f;
    m_max_err = 1.f;

    const int32_t skipped_frames_at_begin = 5;
    uint32_t invalid_value = 0;
    float avg = 0.f;
    int32_t npoints = 0;
    int32_t colorComponents = 1;
    bool skipped = false;

    int depthPitch = m_depth_intrin.width * image_utils::get_pixel_size(projection_tests_util::depth_format);
    int colorPitch = m_color_intrin.width * image_utils::get_pixel_size(projection_tests_util::color_format);
    image_info depthInfo = { m_depth_intrin.width, m_depth_intrin.height, convert_pixel_format(projection_tests_util::depth_format), depthPitch };
    image_info colorInfo = { m_color_intrin.width, m_color_intrin.height, convert_pixel_format(projection_tests_util::color_format), colorPitch };

    m_device->start();
    for(int i = skipped_frames_at_begin; i < projection_tests_util::total_frames; i++)
    {
        m_device->set_frame_by_index(i, rs::stream::depth);

        const void* depthData = m_device->get_frame_data(rs::stream::depth);
        const void* colorData = m_device->get_frame_data(rs::stream::color);
        auto depth = get_unique_ptr_with_releaser(image_interface::create_instance_from_raw_data(&depthInfo,
                           {depthData, nullptr},
                           stream_type::depth,
                           image_interface::flag::any,
                           m_device->get_frame_timestamp(rs::stream::depth),
                           m_device->get_frame_number(rs::stream::depth)));
        auto color = get_unique_ptr_with_releaser(image_interface::create_instance_from_raw_data(&colorInfo,
                           {colorData, nullptr},
                           stream_type::color,
                           image_interface::flag::any,
                           m_device->get_frame_timestamp(rs::stream::color),
                           m_device->get_frame_number(rs::stream::color)));
        /* Get uvmap */
        std::vector<pointF32> uvMap(depthInfo.width * depthInfo.height);
        m_sts = m_projection->query_uvmap(depth.get(), uvMap.data());
        if(m_sts == status_feature_unsupported)
        {
            skipped = true;
        }
        else if(m_sts < status_no_error)
        {
            ASSERT_EQ(m_sts, status_no_error);
        }

        auto color2depth  = get_unique_ptr_with_releaser(m_projection->create_color_image_mapped_to_depth(depth.get(), color.get()));
        ASSERT_NE(color2depth.get(), nullptr);
        ASSERT_NE(color2depth->query_data(), nullptr);
        const uint8_t* color2depth_data = (const uint8_t*)color2depth->query_data();
        const uint8_t* color_data = (const uint8_t*)color->query_data();

        image_info color2depth_info = color2depth->query_info();
        colorComponents = image_utils::get_pixel_size(color2depth_info.format);
        ASSERT_NE(colorComponents, 0);
        for (int32_t y = 0; y < m_color_intrin.height; y++)
        {
            for (int32_t x = 0; x < m_color_intrin.width; x++)
            {
                pointF32 uv = uvMap[y*m_depth_intrin.width+x];
                if(uv.x < 0.f || uv.x >= 1.f || uv.y < 0.f || uv.y >= 1.f) continue;
                uv.x *= static_cast<float>(m_depth_intrin.width); uv.x += 0.5f;
                uv.y *= static_cast<float>(m_depth_intrin.height); uv.y += 0.5f;
                for (int32_t cc = 0; cc < colorComponents; cc++)
                {
                    uint32_t c1 = (uint32_t)(color_data + (int)uv.y*colorInfo.pitch)[(int)uv.x + cc];
                    uint32_t c2 = (uint32_t)(color2depth_data + y*colorInfo.pitch)[x + cc];
                    if (c1 == invalid_value || c2 == invalid_value) continue;
					if(0 != abs((long)(c1 - c2)))
                    {
                        avg++;
                    }
                }
                npoints++;
            }
        }
    }
    if(!skipped)
    {
        ASSERT_NE(npoints, 0);
        avg = avg / static_cast<float>(colorComponents) / static_cast<float>(npoints);
        EXPECT_LE(avg, m_avg_err);
        if( avg > m_avg_err )
        {
            std::basic_ostringstream<wchar_t> stream;
            stream << L"FAIL: " << rsformatToWString(m_formats.at(rs::stream::color)) << " " << m_color_intrin.width << "x" << m_color_intrin.height << "; ";
            stream << rsformatToWString(m_formats.at(rs::stream::depth)) << " " << m_depth_intrin.width << "x" << m_depth_intrin.height << "; ";
            stream << "File: " << projection_tests_util::file_name.c_str() << "; avg_err= " << avg;
            m_log_util.m_logger->logw(logging_service::LEVEL_ERROR, stream.str().c_str(), __FILE__, __LINE__, "create_depth_image_mapped_to_color_query_invuvmap");
            m_is_failed = true;
        }
    }
}
