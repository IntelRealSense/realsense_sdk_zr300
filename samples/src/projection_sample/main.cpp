// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

// Projection Module Sample
// This sample demonstrates an application usage of a projection module, which implements mappings between various coordinate systems
// used by SDK modules.
// The projection module implements the projection interface, which is a common way for the application or SDK to interact with
// the module.

#include <memory>
#include <vector>
#include <iostream>
#include <librealsense/rs.hpp>

#include "rs/utils/librealsense_conversion_utils.h"

#include "rs_sdk.h"

using namespace std;
using namespace rs::core;
using namespace rs::utils;

//helper functions
void get_depth_coordinates_from_rectangle_on_depth_image(std::shared_ptr<image_interface> depthImage, vector<point3dF32> &depth_coordinates);
void get_color_coordinates_from_rectangle_on_color_image(std::shared_ptr<image_interface> colorImage, vector<pointF32> &color_coordinates);

int main ()
{
    rs::context context;
    if(context.get_device_count() == 0)
    {
        cerr<<"no device detected" << endl;
        return -1;
    }

    rs::device* device = context.get_device(0);

    //color profile
    int color_width = 640, color_height = 480, color_fps = 60, color_pixel_size = 3;
    const rs::format color_format = rs::format::rgb8;

    //depth profile
    int depth_width = 628, depth_height = 468, depth_fps = 60, depth_pixel_size = 2;
    const rs::format depth_format = rs::format::z16;

    device->enable_stream(rs::stream::color, color_width, color_height, color_format, color_fps);
    device->enable_stream(rs::stream::depth, depth_width, depth_height, depth_format, depth_fps);

    device->start();

    intrinsics color_intrin = convert_intrinsics(device->get_stream_intrinsics(rs::stream::color));
    intrinsics depth_intrin = convert_intrinsics(device->get_stream_intrinsics(rs::stream::depth));
    extrinsics extrin = convert_extrinsics(device->get_extrinsics(rs::stream::depth, rs::stream::color));

    auto projection_ = rs::utils::get_unique_ptr_with_releaser(projection_interface::create_instance(&color_intrin, &depth_intrin, &extrin));

    device->wait_for_frames();

    image_info  color_info;
    color_info.width = color_width;
    color_info.height = color_height;
    color_info.format = rs::utils::convert_pixel_format(color_format);
    color_info.pitch = color_pixel_size * color_width;

    std::shared_ptr<image_interface> color_image = get_shared_ptr_with_releaser(image_interface::create_instance_from_raw_data(
                                                                                   &color_info,
                                                                                   {device->get_frame_data(rs::stream::color), nullptr},
                                                                                   stream_type::color,
                                                                                   image_interface::flag::any,
                                                                                   device->get_frame_timestamp(rs::stream::color),
                                                                                   device->get_frame_number(rs::stream::color)));

    image_info  depth_info;
    depth_info.width = depth_width;
    depth_info.height = depth_height;
    depth_info.format = rs::utils::convert_pixel_format(depth_format);
    depth_info.pitch = depth_pixel_size * depth_width;

    std::shared_ptr<image_interface> depth_image = get_shared_ptr_with_releaser(image_interface::create_instance_from_raw_data(
                                                                                   &depth_info,
                                                                                   {device->get_frame_data(rs::stream::depth), nullptr},
                                                                                   stream_type::depth,
                                                                                   image_interface::flag::any,
                                                                                   device->get_frame_timestamp(rs::stream::depth),
                                                                                   device->get_frame_number(rs::stream::depth)));

    /**
     * MapDepthToColor example.
     * Map depth coordinates to color coordinates for a few pixels
     */
    // create a point3dF32 array for the depth coordinates you would like to project, for example the central rectangle
    vector<point3dF32> depth_coordinates;
    get_depth_coordinates_from_rectangle_on_depth_image(depth_image, depth_coordinates);

    vector<pointF32> mapped_color_coordinates(depth_coordinates.size());
    if(projection_->map_depth_to_color(depth_coordinates.size(), depth_coordinates.data(), mapped_color_coordinates.data()) < status_no_error)
    {
        cerr<<"failed to map the depth coordinates to color" << endl;
        return -1;
    }

    /**
     * MapColorToDepth example.
     * Map color coordinates to depth coordiantes for a few pixels.
     */
    // create a pointF32 array for the color coordinates you would like to project, for example the central rectangle
    vector<pointF32> color_coordinates;
    get_color_coordinates_from_rectangle_on_color_image(color_image, color_coordinates);

    vector<pointF32> mapped_dDepth_coordinates(color_coordinates.size());
    if(projection_->map_color_to_depth(depth_image.get(), color_coordinates.size(), color_coordinates.data(), mapped_dDepth_coordinates.data()) < status_no_error)
    {
        cerr<<"failed to map the color coordinates to depth" << endl;
        return -1;
    }

    /**
     * ProjectDepthToCamera Example.
     * Map depth coordinates to world coordinates for a few pixels.
     */
    vector<point3dF32> world_coordinates_from_depth_coordinates(depth_coordinates.size());
    if(projection_->project_depth_to_camera(depth_coordinates.size(), depth_coordinates.data(), world_coordinates_from_depth_coordinates.data()) < status_no_error)
    {
        cerr<<"failed to project depth coordinates to world coordinates" << endl;
        return -1;
    }

    /**
     * ProjectColorToCamera Example.
     * Map color pixel coordinates to camera coordinates for a few pixels.
     */
    //create array of color coordinates + depth value in the point3dF32 structure
    vector<point3dF32> color_coordinates_with_depth_value(mapped_color_coordinates.size());
    for(size_t i = 0; i < mapped_color_coordinates.size(); i++)
    {
        color_coordinates_with_depth_value[i].x = mapped_color_coordinates[i].x;
        color_coordinates_with_depth_value[i].y = mapped_color_coordinates[i].y;
        color_coordinates_with_depth_value[i].z = depth_coordinates[i].z;
    }

    vector<point3dF32> world_coordinates_from_color_coordinates(color_coordinates_with_depth_value.size());
    if(projection_->project_color_to_camera(color_coordinates_with_depth_value.size(),
                                            color_coordinates_with_depth_value.data(),
                                            world_coordinates_from_color_coordinates.data()) < status_no_error)
    {
        cerr<<"failed to map the color coordinates to world" << endl;
        return -1;
    }

    /**
     * ProjectCameraToDepth Example.
     * Map camera coordinates to depth coordinates for a few pixels.
     */
    vector<pointF32> depth_coordinates_from_world_coordinates(world_coordinates_from_depth_coordinates.size());
    if(projection_->project_camera_to_depth(world_coordinates_from_depth_coordinates.size(),
                                            world_coordinates_from_depth_coordinates.data(),
                                            depth_coordinates_from_world_coordinates.data()) < status_no_error)
    {
        cerr<<"failed to map the world coordinates to depth coordinates" << endl;
        return -1;
    }

    /**
     * ProjectCameraToColor Example.
     * Map camera coordinates to color coordinates for a few pixels.
     */
    vector<pointF32> color_coordinates_from_world_coordinates(world_coordinates_from_color_coordinates.size());
    if(projection_->project_camera_to_color(world_coordinates_from_color_coordinates.size(),
                                            world_coordinates_from_color_coordinates.data(),
                                            color_coordinates_from_world_coordinates.data()) < status_no_error)
    {
        cerr<<"failed to map the world coordinates to color coordinates" << endl;
        return -1;
    }

    /**
     * QueryUVMap Example.
     * Retrieve the UV map for the specific depth image. The UVMap is a pointF32 array of depth size width*height.
     */
    auto depth_image_info = depth_image->query_info();
    vector<pointF32> uvmap(depth_image_info.width * depth_image_info.height);
    if(projection_->query_uvmap(depth_image.get(), uvmap.data()) < status_no_error)
    {
        cerr<<"failed to query UV map" << endl;
        return -1;
    }

    /**
     * QueryInvUVMap Example.
     * Retrieve the inverse UV map for the specific depth image. The inverse UV map maps color coordinates
     * back to the depth coordinates. The inverse UVMap is a pointF32 array of color size width*height.
     */
    auto color_image_info = color_image->query_info();
    vector<pointF32> inv_uvmap(color_image_info.width * color_image_info.height);
    if(projection_->query_invuvmap(depth_image.get(), inv_uvmap.data()) < status_no_error)
    {
        cerr<<"failed to query invariant UV map" << endl;
        return -1;
    }

    /**
     * QueryVertices Example.
     * Retrieve the vertices for the specific depth image. The vertices is a point3dF32 array of depth
     * size width*height. The world coordiantes units are in mm.
     */
    vector<point3dF32> vertices(depth_image_info.width * depth_image_info.height);
    if(projection_->query_vertices(depth_image.get(), vertices.data()) < status_no_error)
    {
        cerr<<"failed to query vertices" << endl;
        return -1;
    }

    /**
     * CreateColorImageMappedToDepth Example.
     * Get the color pixel for every depth pixel using the UV map, and output a color image, aligned in space
     * and resolution to the depth image.
     */
    auto color_image_mapped_to_depth = get_unique_ptr_with_releaser(projection_->create_color_image_mapped_to_depth(depth_image.get(), color_image.get()));
    //use the mapped image...

    //The application must release the instance after use. (e.g. use smart ptr)

    /**
     * CreateDepthImageMappedToColor Example.
     * Map every depth pixel to the color image resolution, and output a depth image, aligned in space
     * and resolution to the color image. The color image size may be different from original.
     */
    auto depth_image_mapped_to_color = get_unique_ptr_with_releaser(projection_->create_depth_image_mapped_to_color(depth_image.get(), color_image.get()));
    //use the mapped image...

    //The application must release the instance after use. (e.g. use smart ptr)

    device->stop();

    return 0;
}

void get_depth_coordinates_from_rectangle_on_depth_image(std::shared_ptr<image_interface> depthImage, vector<point3dF32> &depth_coordinates)
{
    if(!depthImage)
    {
        cerr<<"cant use null image" << endl;
        return;
    }

    auto depth_image_info = depthImage->query_info();

    // get read access to the detph image data
    const void * depth_image_data = depthImage->query_data();
    if(!depth_image_data)
    {
        cerr<<"failed to get depth image data" << endl;
        return;
    }

    const int start_x = depth_image_info.width / 4; const int endX = (depth_image_info.width  * 3) / 4;
    const int start_y = depth_image_info.height / 4; const int endY = (depth_image_info.height * 3) / 4;
    for(int i = start_x; i < endX; i++)
    {
        for(int j = start_y; j < endY; j++)
        {
            point3dF32 coordinate;
            coordinate.x = i;
            coordinate.y = j;
            coordinate.z = reinterpret_cast<uint16_t *> (const_cast<void *>(depth_image_data))[depth_image_info.width * j + i];
            depth_coordinates.push_back(coordinate);
        }
    }

}

void get_color_coordinates_from_rectangle_on_color_image(std::shared_ptr<image_interface> color_image, vector<pointF32> &color_coordinates)
{
    if(!color_image)
    {
        cerr<<"cant use null image" << endl;
        return;
    }

    auto color_image_info = color_image->query_info();

    // create a pointF32 array for the color coordinates you would like to project, for example the central rectangle
    const int startX = color_image_info.width / 4; const int endX = (color_image_info.width  * 3) / 4;
    const int startY = color_image_info.height/ 4; const int endY = (color_image_info.height * 3) / 4;
    for(int i = startX; i < endX; i++)
    {
        for(int j = startY; j < endY; j++)
        {
            pointF32 coordinate;
            coordinate.x = i;
            coordinate.y = j;
            color_coordinates.push_back(coordinate);
        }
    }
}
