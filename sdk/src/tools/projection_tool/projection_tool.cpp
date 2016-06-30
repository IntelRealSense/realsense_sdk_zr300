// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/* realsense sdk */
#include "rs_sdk.h"
#include "image/image_utils.h"

/* librealsense */
#include "librealsense/rs.hpp"

/* standard library */
#include <string>
#include <iostream>
#include <algorithm>
#include <map>
#include <cstdint>
#include <fstream>
#include <cstdio>

/* drawing */
#include "projection_gui.h"

/* See the samples for detailed description of the realsense api */

using namespace cv;
using namespace rs::core;
using namespace rs::utils;

/**
 * @brief create_world_data
 * Return real world image as a raw data
 * @param[in] realsense_projection  RealSense projection object
 * @param[in] depth_data            Data returned from depth camera
 * @param[in] depth                 RealSense image object
 * @param[in] depth_width           Width of the depth image in pixels
 * @param[in] depth_height          Height of the depth image in pixels
 * @return                          World image raw data
 * @return nullptr                  Failed to query vertices from projection image
*/
uint8_t* create_world_data(projection* realsense_projection, const void* depth_data, custom_image* depth, int depth_width, int depth_height);

/* parsing data */
static const char* keys =
{
    "{ help h |            | Show the usage        }"
    "{ file f |            | Specify playback file path }"
    "{ depth  | 628x468x30 | Specify depth stream resolution: WIDTHxHEIGHTxFPS }"
    "{ color  | 640x480x30 | Specify color stream resolution: WIDTHxHEIGHTxFPS }"
};

int main(int argc, char* argv[])
{
    std::unique_ptr<context> realsense_context;
    rs::device* realsense_device;
    std::unique_ptr<projection> realsense_projection;
    rs::stream color_stream = rs::stream::color;

    int default_depth_width = 628, default_depth_height = 468, default_color_width = 640, default_color_height = 480, default_fps = 30;
    int max_depth_width = 628, max_depth_height = 468, max_color_width = 1920, max_color_height = 1080, max_fps = 60;

    CommandLineParser parser(argc, argv, keys);
#if (CV_MAJOR_VERSION == 3)
    if (parser.has("help"))
    {
        parser.printMessage();
        return 0;
    }
#elif (CV_MAJOR_VERSION == 2)
    const bool is_help_specified = parser.get<bool>("help");
    if (is_help_specified)
    {
        parser.printParams();
        return 0;
    }
#endif
    const std::string file_path = parser.get<std::string>("file");
    if (!file_path.empty()) /* file playback */
    {
        if (!(std::ifstream(file_path.c_str()).good())) // check that file is accessible
        {
            std::cerr << "Error: Playback file is not accessible" << std::endl;
            return -1;
        }
        realsense_context = std::unique_ptr<rs::playback::context>(new rs::playback::context(file_path));
        realsense_device = realsense_context->get_device(0); // device is managed by context, no need to deallocate manually
        if (!realsense_device)
        {
            std::cerr << "Error: Unable to get device" << std::endl;
            return -1;
        }
        std::vector<rs::stream> streams = { rs::stream::color, rs::stream::depth };

        for(auto stream_iterator = streams.begin(); stream_iterator != streams.end(); stream_iterator++)
        {
            if(realsense_device->get_stream_mode_count(*stream_iterator) > 0)
            {
                int width, height, fps;
                rs::format format;
                realsense_device->get_stream_mode(*stream_iterator, 0, width, height, format, fps);
                realsense_device->enable_stream(*stream_iterator, width, height, format, fps);
            }
        }
    }
    else /* camera live streaming */
    {
        struct stream_resolution_info { int width, height, fps; };
        // depth resolution
        const std::string depth_resolution = parser.get<std::string>("depth"); // parsing depth stream resolution
        stream_resolution_info depth_info;
        sscanf(depth_resolution.c_str(), "%dx%dx%d", &depth_info.width, &depth_info.height, &depth_info.fps);
        if (depth_info.width <= 0 || depth_info.width > max_depth_width) depth_info.width = default_depth_width;
        if (depth_info.height <= 0 || depth_info.height > max_depth_height) depth_info.height = default_depth_height;
        if (depth_info.fps <= 0 || depth_info.fps > max_fps) depth_info.fps = default_fps;
        // color resolution
        const std::string color_resolution = parser.get<std::string>("color"); // parsing color stream resolution
        stream_resolution_info color_info;
        sscanf(color_resolution.c_str(), "%dx%dx%d", &color_info.width, &color_info.height, &color_info.fps);
        if (color_info.width <= 0 || color_info.width > max_color_width) color_info.width = default_color_width;
        if (color_info.height <= 0 || color_info.height > max_color_height) color_info.height = default_color_height;
        if (color_info.fps <= 0 || color_info.fps > max_fps) color_info.fps = default_fps;

        realsense_context = std::unique_ptr<context>(new context());
        if(realsense_context->get_device_count() == 0)
        {
            std::cerr << "Error: No device detected" << std::endl;
            return -1;
        }
        realsense_device = realsense_context->get_device(0); // device is managed by context, no need to deallocate manually
        realsense_device->enable_stream(rs::stream::depth, depth_info.width, depth_info.height, rs::format::z16, depth_info.fps);
        realsense_device->enable_stream(rs::stream::color, color_info.width, color_info.height, rs::format::bgra8, color_info.fps);
        color_stream = rs::stream::rectified_color;
    }
    if (!(realsense_device->is_stream_enabled(rs::stream::depth) && realsense_device->is_stream_enabled(rs::stream::color))
            && (realsense_device->get_stream_format(rs::stream::color) == rs::format::bgra8))
    {
        std::cerr << "Error: Streaming is not enabled" << std::endl;
        return -1;
    }
    realsense_device->start();

    rs_intrinsics color_intrin = realsense_device->get_stream_intrinsics(color_stream);
    rs_intrinsics depth_intrin = realsense_device->get_stream_intrinsics(rs::stream::depth);

    const int depth_width = depth_intrin.width;
    const int depth_height = depth_intrin.height;
    const int color_width = color_intrin.width;
    const int color_height = color_intrin.height;

    rs_extrinsics extrinsics = realsense_device->get_extrinsics(rs::stream::depth, color_stream);
    realsense_projection = std::unique_ptr<projection>(projection::create_instance(&color_intrin, &depth_intrin, &extrinsics));

    // creating drawing object
    projection_gui gui_handler(depth_width, depth_height, color_width, color_height);

    bool continue_showing = true;
    int gui_status = 0;
    while(realsense_device->is_streaming() && continue_showing)
    {
        realsense_device->wait_for_frames();
        // read images
        const void* depth_data = (void*)realsense_device->get_frame_data(rs::stream::depth); // deallocation is handled by the context
        if (!depth_data)
        {
            std::cerr << "Unable to get depth data" << std::endl;
            return -1;
        }
        const void* color_data = (void*)realsense_device->get_frame_data(rs::stream::color);
        if (!color_data)
        {
            std::cerr << "Unable to get color data" << std::endl;
            return -1;
        }
        const int depth_pitch = depth_width * image_utils::get_pixel_size(rs::format::z16);

        // image objects (image info and data pointers)
        image_info  depth_info = {depth_width, depth_height, convert_pixel_format(rs::format::z16), depth_pitch};
        custom_image depth (&depth_info,
                            depth_data,
                            stream_type::depth,
                            image_interface::flag::any,
                            realsense_device->get_frame_timestamp(rs::stream::depth),
                            nullptr,
                            nullptr);
        const std::unique_ptr<uint8_t> world_data =
            std::unique_ptr<uint8_t>(create_world_data(realsense_projection.get(), depth_data, &depth, depth_width, depth_height)); // deallocation is handled by smart ptr
        if (!(world_data.get()))
        {
            std::cerr << "Unable to get world data" << std::endl;
            destroyAllWindows();
            break;
        }
        // creating cv::Mat from raw data
        // every image should be properly created
        gui_status = gui_handler.create_image(color_data, image_type::color, CV_8UC4); if (gui_status < 0) break;
        gui_status = gui_handler.create_image(depth_data, image_type::depth, CV_16UC1); if (gui_status < 0) break;
        gui_status = gui_handler.create_image((void*)world_data.get(), image_type::world, CV_8UC1); if (gui_status < 0) break;

        /* Projection functions: */
        status status = status_no_error;
        if (gui_handler.is_uvmap_queried()) // uvmap
        {
            /* Documentation reference: query_uvmap function */
            std::vector<pointF32> uvmap(depth_width*depth_height);
            status = realsense_projection->query_uvmap(&depth, uvmap.data());
            if (status != status_no_error)
            {
                std::cerr << "Error: query_uvmap bad status returned: " << status << std::endl;
                break;
            }
            std::vector<uint32_t> uvmap_data(depth_width*depth_height);
            for(int u = 0; u < depth_width; u++)
            {
                for(int v = 0; v < depth_height; v++)
                {
                    uvmap_data[u+v*depth_width] = 255;
                    int i = (float)uvmap[u+v*depth_width].x*(float)color_width;
                    int j = (float)uvmap[u+v*depth_width].y*(float)color_height;
                    if(i < 0 || i > depth_width-1) continue;
                    if(j < 0 || j > depth_height-1) continue;
                    uvmap_data[u+v*depth_width] = ((uint32_t*)color_data)[i+j*color_width];
                }
            }
            gui_handler.create_image((void*)uvmap_data.data(), image_type::uvmap, CV_8UC4);
            status = status_no_error;
        }
        if (gui_handler.is_invuvmap_queried()) // inversed uvmap
        {
            /* Documentation reference: query_invuvmap function */
            std::vector<pointF32> invuvmap(color_width*color_height);
            status = realsense_projection->query_invuvmap(&depth, invuvmap.data());
            if (status != status_no_error)
            {
                std::cerr << "Error: query_invuvmap bad status returned: " << status << std::endl;
                break;
            }
            std::vector<uint16_t> invuvmap_data(color_width*color_height);
            for(int i = 0; i < color_width; i++)
            {
                for(int j = 0; j < color_height; j++)
                {
                    invuvmap_data[i+j*color_width] = 255;
                    int u = (float)invuvmap[i+j*color_width].x*(float)depth_width;
                    int v = (float)invuvmap[i+j*color_width].y*(float)depth_height;
                    if(u < 0 || u > color_width-1) continue;
                    uint16_t depth_value = ((uint16_t*)depth_data)[u+v*depth_width];
                    if(depth_value > 0)
                    {
                        invuvmap_data[i+j*color_width] = ((uint16_t*)depth_data)[u+v*depth_width];
                    }
                }
            }
            gui_handler.create_image((void*)invuvmap_data.data(), image_type::invuvmap, CV_16UC1);
            status = status_no_error;
        }
        gui_handler.convert_to_visualized_images();

        /* drawing on images */
        switch(gui_handler.image_with_drawn_points())
        {
            case image_type::depth:
            {
                std::vector<pointI32> drawn_points(gui_handler.get_points());
                int drawn_points_size = drawn_points.size();
                std::vector<point3dF32> depth_points_3D(drawn_points_size);
                for(int i = 0; i < drawn_points_size; i++)
                {
                    depth_points_3D[i].x = drawn_points[i].x;
                    depth_points_3D[i].y = drawn_points[i].y;
                    depth_points_3D[i].z = ((int16_t*)depth_data)[drawn_points[i].x + drawn_points[i].y * depth_width];
                }

                /* Documentation reference: map_depth_to_color function */
                // mapping depth image to color image
                std::vector<pointF32> color_points(drawn_points_size);
                status = realsense_projection->map_depth_to_color(drawn_points_size, depth_points_3D.data(), color_points.data());
                if (status != status_no_error)
                {
                    std::cerr << "Cannot map_depth_to_color. Status: " << status << std::endl;
                    break;
                }

                /* Documentation reference: project_depth_to_camera function */
                // projecting depth image to image from camera
                std::vector<point3dF32> world_points_3D(drawn_points_size);
                status = realsense_projection->project_depth_to_camera(drawn_points_size, depth_points_3D.data(), world_points_3D.data());
                if (status != status_no_error)
                {
                    std::cerr << "Cannot project_depth_to_camera. Status: " << status << std::endl;
                    break;
                }

                //TODO: remove this workaround
                for (int i = 0; i < drawn_points_size; i++)
                {
                    if (world_points_3D[i].z <= 0) world_points_3D[i].z = -1.0f;
                }

                /* Documentation reference: project_camera_to_depth function */
                // projhecting image from camera to world image
                std::vector<pointF32> world_points(drawn_points_size);
                status = realsense_projection->project_camera_to_depth(drawn_points_size, world_points_3D.data(), world_points.data());
                if (status != status_no_error)
                {
                    std::cerr << "Cannot project_camera_to_depth. Status: " << status << std::endl;
                    break;
                }

                // drawing points on cv::Mat
                for (int i = 0; i < drawn_points_size; i++)
                {
                    gui_handler.draw_points(image_type::depth, drawn_points[i].x, drawn_points[i].y);
                    bool depth_points_valid = (depth_points_3D[i].z > 0);
                    if (depth_points_valid)
                    {
                        gui_handler.draw_points(image_type::color, color_points[i].x, color_points[i].y);
                        gui_handler.draw_points(image_type::world, world_points[i].x, world_points[i].y);
                    }
                }
                status = status_no_error;
                break;
            }
            case image_type::color:
            {
                std::vector<pointI32> drawn_points(gui_handler.get_points());
                int drawn_points_size = drawn_points.size();
                std::vector<pointF32> color_points(drawn_points_size);
                for(int i = 0; i < drawn_points_size; i++)
                {
                    color_points[i].x = drawn_points[i].x;
                    color_points[i].y = drawn_points[i].y;
                }

                /* Documentation reference: map_color_to_depth function */
                // mapping color image to depth image
                std::vector<pointF32> depth_points(drawn_points_size);
                status = realsense_projection->map_color_to_depth(&depth, drawn_points_size, color_points.data(), depth_points.data());
                if (status != status_no_error)
                {
                    if (status != status_value_out_of_range)
                    {
                        std::cerr << "Cannot map_color_to_depth. Status: " << status << std::endl;
                        break;
                    }
                }

                /* Documentation reference: project_color_to_camera function */
                // projecting color image to image from camera
                std::vector<pointF32> world_points(drawn_points_size);
                status = realsense_projection->map_color_to_depth(&depth, drawn_points_size, color_points.data(), world_points.data());
                if (status != status_no_error)
                {
                    if (status != status_value_out_of_range)
                    {
                        std::cerr << "Cannot map_color_to_depth. Status: " << status << std::endl;
                        break;
                    }
                }

                // drawing points on cv::Mat
                for (int i = 0; i < drawn_points_size; i++)
                {
                    gui_handler.draw_points(image_type::color, color_points[i].x, color_points[i].y);
                    bool depth_points_mapped = (depth_points[i].x > 0);
                    if (depth_points_mapped)
                    {
                        gui_handler.draw_points(image_type::depth, depth_points[i].x, depth_points[i].y);
                    }
                    bool world_points_mapped = (world_points[i].x > 0);
                    if (world_points_mapped)
                    {
                        gui_handler.draw_points(image_type::world, world_points[i].x, world_points[i].y);
                    }
                }
                status = status_no_error;
                break;
            }
            case image_type::world:
            {
                std::vector<pointI32> drawn_points(gui_handler.get_points());
                int drawn_points_size = drawn_points.size();
                std::vector<point3dF32> depth_points_3D(drawn_points_size);
                for(int i = 0; i < drawn_points_size; i++)
                {
                    if (drawn_points[i].x < 0.f) continue;
                    depth_points_3D[i].x = drawn_points[i].x;
                    depth_points_3D[i].y = drawn_points[i].y;
                    depth_points_3D[i].z = ((int16_t*)depth_data)[drawn_points[i].x + drawn_points[i].y * depth_width];
                    if (depth_points_3D[i].z <= 0) depth_points_3D[i].z = -1.0f;
                }

                /* Documentation reference: project_depth_to_camera function */
                // projecting depth image to image from camera
                std::vector<point3dF32> world_points_3D(drawn_points_size);
                status = realsense_projection->project_depth_to_camera(drawn_points_size, depth_points_3D.data(), world_points_3D.data());
                if (status != status_no_error)
                {
                    std::cerr << "Cannot project_depth_to_camera. Status: " << status << std::endl;
                    break;
                }

                /* Documentation reference: project_camera_to_depth function */
                // projecting image from camera to depth image
                std::vector<pointF32> depth_points(drawn_points_size);
                status = realsense_projection->project_camera_to_depth(drawn_points_size, world_points_3D.data(), depth_points.data());
                if (status != status_no_error)
                {
                    std::cerr << "Cannot project_camera_to_depth. Status: " << status << std::endl;
                    break;
                }

                /* Documentation reference: project_camera_to_color function */
                // projecting image from camera to color image
                std::vector<pointF32> color_points(drawn_points_size);
                status = realsense_projection->project_camera_to_color(drawn_points_size, world_points_3D.data(), color_points.data());
                if (status != status_no_error)
                {
                    std::cerr << "Cannot project_camera_to_color. Status: " << status << std::endl;
                    break;
                }

                // drawing points on cv::Mat
                for (int i = 0; i < drawn_points_size; i++)
                {
                    gui_handler.draw_points(image_type::world, drawn_points[i].x, drawn_points[i].y);
                    bool depth_points_valid = (depth_points_3D[i].z > 0);
                    if (depth_points_valid)
                    {
                        gui_handler.draw_points(image_type::depth, depth_points[i].x, depth_points[i].y);
                        gui_handler.draw_points(image_type::color, color_points[i].x, color_points[i].y);
                    }
                }
                status = status_no_error;
                break;
            }
            default: break; // process nothing
        }
        continue_showing = gui_handler.show_window();
    }
    if (realsense_device) realsense_device->stop();
    return 0;
}

uint8_t* create_world_data(projection* realsense_projection, const void* depth_data, custom_image* depth, int depth_width, int depth_height)
{
    std::vector<point3dF32> matrix(depth_width*depth_height);
    const int pitch = depth_width;
    uint8_t* raw_data = new uint8_t[pitch * depth_height]; // an array to store z-value of vertices
    if (!depth->query_data())
    {
        std::cerr << "Error: Unable to query data from image object" << std::endl;
        return nullptr;
    }
    /* Documentation reference: query_vertices function */
    status status = realsense_projection->query_vertices(depth, matrix.data());
    if (status != status_no_error)
    {
        std::cerr << "Error: query_vertices bad status returned: " << status << std::endl;
        return nullptr;
    }
    for (int i = 0; i < depth_height; i++)
    {
        for (int j = 0; j < depth_width; j++)
        {
            raw_data[j + i * depth_width] = matrix[j + i * depth_width].z;
        }
    }
    return raw_data;
}
