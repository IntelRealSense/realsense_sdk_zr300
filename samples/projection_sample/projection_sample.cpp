// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <memory>
#include <vector>
#include <iostream>
#include <librealsense/rs.hpp>

#include "rs_sdk.h"

using namespace std;
using namespace rs::core;

//helper functions
void get_depth_coordinates_from_rectangle_on_depth_image(custom_image &depthImage, vector<point3dF32> &depthCoordinates);
void get_color_coordinates_from_rectangle_on_color_image(custom_image &colorImage, vector<pointF32> &colorCoordinates);

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
    int colorWidth = 640, colorHeight = 480, colorFps = 60, color_pixel_size = 3;
    const rs::format colorFormat = rs::format::rgb8;

    //depth profile
    int depthWidth = 628, depthHeight = 468, depthFps = 60, depth_pixel_size = 2;
    const rs::format depthFormat = rs::format::z16;

    device->enable_stream(rs::stream::color, colorWidth, colorHeight, colorFormat, colorFps);
    device->enable_stream(rs::stream::depth, depthWidth, depthHeight, depthFormat, depthFps);

    device->start();

    rs_intrinsics color_intrin = device->get_stream_intrinsics(rs::stream::color);
    rs_intrinsics depth_intrin = device->get_stream_intrinsics(rs::stream::depth);
    rs_extrinsics extrinsics = device->get_extrinsics(rs::stream::depth, rs::stream::color);

    std::unique_ptr<projection> projection_ = std::unique_ptr<projection>(projection::create_instance(&color_intrin, &depth_intrin, &extrinsics));

    device->wait_for_frames();

    image_info  ColorInfo;
    ColorInfo.width = colorWidth;
    ColorInfo.height = colorHeight;
    ColorInfo.format = rs::utils::convert_pixel_format(colorFormat);
    ColorInfo.pitch = color_pixel_size * colorWidth;

    custom_image colorImage (&ColorInfo,
                             device->get_frame_data(rs::stream::color),
                             stream_type::color,
                             image_interface::flag::any,
                             device->get_frame_timestamp(rs::stream::color),
                             device->get_frame_number(rs::stream::color),
                             nullptr, nullptr);

    image_info  DepthInfo;
    DepthInfo.width = depthWidth;
    DepthInfo.height = depthHeight;
    DepthInfo.format = rs::utils::convert_pixel_format(depthFormat);
    DepthInfo.pitch = depth_pixel_size * depthWidth;

    custom_image depthImage (&DepthInfo,
                             device->get_frame_data(rs::stream::depth),
                             stream_type::depth,
                             image_interface::flag::any,
                             device->get_frame_timestamp(rs::stream::depth),
                             device->get_frame_number(rs::stream::depth),
                             nullptr, nullptr);

    /**
     * MapDepthToColor example.
     * Map depth coordinates to color coordinates for a few pixels
     */
    // create a point3dF32 array for the depth coordinates you would like to project, for example the central rectangle
    vector<point3dF32> depthCoordinates;
    get_depth_coordinates_from_rectangle_on_depth_image(depthImage, depthCoordinates);

    vector<pointF32> mappedColorCoordinates(depthCoordinates.size());
    if(projection_->map_depth_to_color(depthCoordinates.size(), depthCoordinates.data(), mappedColorCoordinates.data()) < status_no_error)
    {
        cerr<<"failed to map the depth coordinates to color" << endl;
        return -1;
    }

    /**
     * MapColorToDepth example.
     * Map color coordinates to depth coordiantes for a few pixels.
     */
    // create a pointF32 array for the color coordinates you would like to project, for example the central rectangle
    vector<pointF32> colorCoordinates;
    get_color_coordinates_from_rectangle_on_color_image(colorImage, colorCoordinates);

    vector<pointF32> mappedDepthCoordinates(colorCoordinates.size());
    if(projection_->map_color_to_depth(&depthImage, colorCoordinates.size(), colorCoordinates.data(), mappedDepthCoordinates.data()) < status_no_error)
    {
        cerr<<"failed to map the color coordinates to depth" << endl;
        return -1;
    }

    /**
     * ProjectDepthToCamera Example.
     * Map depth coordinates to world coordinates for a few pixels.
     */
    vector<point3dF32> worldCoordinatesFromDepthCoordinates(depthCoordinates.size());
    if(projection_->project_depth_to_camera(depthCoordinates.size(), depthCoordinates.data(), worldCoordinatesFromDepthCoordinates.data()) < status_no_error)
    {
        cerr<<"failed to project depth coordinates to world coordinates" << endl;
        return -1;
    }

    /**
     * ProjectColorToCamera Example.
     * Map color pixel coordinates to camera coordinates for a few pixels.
     */
    //create array of color coordinates + depth value in the point3dF32 structure
    vector<point3dF32> colorCoordinatesWithDepthValue(mappedColorCoordinates.size());
    for(size_t i = 0; i < mappedColorCoordinates.size(); i++)
    {
        colorCoordinatesWithDepthValue[i].x = mappedColorCoordinates[i].x;
        colorCoordinatesWithDepthValue[i].y = mappedColorCoordinates[i].y;
        colorCoordinatesWithDepthValue[i].z = depthCoordinates[i].z;
    }

    vector<point3dF32> worldCoordinatesFromColorCoordinates(colorCoordinatesWithDepthValue.size());
    if(projection_->project_color_to_camera(colorCoordinatesWithDepthValue.size(),
                                           colorCoordinatesWithDepthValue.data(),
                                           worldCoordinatesFromColorCoordinates.data()) < status_no_error)
    {
        cerr<<"failed to map the color coordinates to world" << endl;
        return -1;
    }

    /**
     * ProjectCameraToDepth Example.
     * Map camera coordinates to depth coordinates for a few pixels.
     */
    vector<pointF32> depthCoordinatesFromWorldCoordinates(worldCoordinatesFromDepthCoordinates.size());
    if(projection_->project_camera_to_depth(worldCoordinatesFromDepthCoordinates.size(),
                                           worldCoordinatesFromDepthCoordinates.data(),
                                           depthCoordinatesFromWorldCoordinates.data()) < status_no_error)
    {
        cerr<<"failed to map the world coordinates to depth coordinates" << endl;
        return -1;
    }

    /**
     * ProjectCameraToColor Example.
     * Map camera coordinates to color coordinates for a few pixels.
     */
    vector<pointF32> colorCoordinatesFromWorldCoordinates(worldCoordinatesFromColorCoordinates.size());
    if(projection_->project_camera_to_color(worldCoordinatesFromColorCoordinates.size(),
                                           worldCoordinatesFromColorCoordinates.data(),
                                           colorCoordinatesFromWorldCoordinates.data()) < status_no_error)
    {
        cerr<<"failed to map the world coordinates to color coordinates" << endl;
        return -1;
    }

    /**
     * QueryUVMap Example.
     * Retrieve the UV map for the specific depth image. The UVMap is a pointF32 array of depth size width*height.
     */
    auto depthImageInfo = depthImage.query_info();
    vector<pointF32> uvmap(depthImageInfo.width * depthImageInfo.height);
    if(projection_->query_uvmap(&depthImage, uvmap.data()) < status_no_error)
    {
        cerr<<"failed to query UV map" << endl;
        return -1;
    }

    /**
     * QueryInvUVMap Example.
     * Retrieve the inverse UV map for the specific depth image. The inverse UV map maps color coordinates
     * back to the depth coordinates. The inverse UVMap is a pointF32 array of color size width*height.
     */
    auto colorImageInfo = colorImage.query_info();
    vector<pointF32> invUVmap(colorImageInfo.width * colorImageInfo.height);
    if(projection_->query_invuvmap(&depthImage, invUVmap.data()) < status_no_error)
    {
        cerr<<"failed to query invariant UV map" << endl;
        return -1;
    }

    /**
     * QueryVertices Example.
     * Retrieve the vertices for the specific depth image. The vertices is a point3dF32 array of depth
     * size width*height. The world coordiantes units are in mm.
     */
    vector<point3dF32> vertices(depthImageInfo.width * depthImageInfo.height);
    if(projection_->query_vertices(&depthImage, vertices.data()) < status_no_error)
    {
        cerr<<"failed to query vertices" << endl;
        return -1;
    }

    /**
     * CreateColorImageMappedToDepth Example.
     * Get the color pixel for every depth pixel using the UV map, and output a color image, aligned in space
     * and resolution to the depth image.
     */
    std::unique_ptr<custom_image> colorImageMappedToDepth = std::unique_ptr<custom_image>(projection_->create_color_image_mapped_to_depth(&depthImage, &colorImage));
    //use the mapped image...

    //The application must release the instance after use. (e.g. use smart ptr)

    /**
     * CreateDepthImageMappedToColor Example.
     * Map every depth pixel to the color image resolution, and output a depth image, aligned in space
     * and resolution to the color image. The color image size may be different from original.
     */
    std::unique_ptr<custom_image> depthImageMappedToColor = std::unique_ptr<custom_image>(projection_->create_depth_image_mapped_to_color(&depthImage, &colorImage));
    //use the mapped image...

    //The application must release the instance after use. (e.g. use smart ptr)

    device->stop();

    return 0;
}

void get_depth_coordinates_from_rectangle_on_depth_image(custom_image &depthImage, vector<point3dF32> &depthCoordinates)
{
    auto depthImageInfo = depthImage.query_info();

    // get read access to the detph image data
    const void * depthImageData = depthImage.query_data();
    if(!depthImageData)
    {
        cerr<<"failed to get depth image data" << endl;
        return;
    }

    const int startX = depthImageInfo.width / 4; const int endX = (depthImageInfo.width  * 3) / 4;
    const int startY = depthImageInfo.height / 4; const int endY = (depthImageInfo.height * 3) / 4;
    for(int i = startX; i < endX; i++)
    {
        for(int j = startY; j < endY; j++)
        {
            point3dF32 coordinate;
            coordinate.x = i;
            coordinate.y = j;
            coordinate.z = reinterpret_cast<uint16_t *> (const_cast<void *>(depthImageData))[depthImageInfo.width * j + i];
            depthCoordinates.push_back(coordinate);
        }
    }

}

void get_color_coordinates_from_rectangle_on_color_image(custom_image &colorImage, vector<pointF32> &colorCoordinates)
{
    auto colorImageInfo = colorImage.query_info();

    // create a pointF32 array for the color coordinates you would like to project, for example the central rectangle
    const int startX = colorImageInfo.width / 4; const int endX = (colorImageInfo.width  * 3) / 4;
    const int startY = colorImageInfo.height/ 4; const int endY = (colorImageInfo.height * 3) / 4;
    for(int i = startX; i < endX; i++)
    {
        for(int j = startY; j < endY; j++)
        {
            pointF32 coordinate;
            coordinate.x = i;
            coordinate.y = j;
            colorCoordinates.push_back(coordinate);
        }
    }
}
