// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/* realsense sdk */
#include "rs_sdk.h"
#include "image/librealsense_image_utils.h"

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
#include <memory>

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
uint8_t* create_world_data(projection_interface* realsense_projection, const void* depth_data, image_interface* depth, int depth_width, int depth_height);

/* helper functions */
/*
 * All the helper functions defined are usually known as vector operations such as dot product, normalization of vector, etc.
 * In that sense, it seems more correct to call the parameters vectors, though these parameters are single points in 2D/3D space.
 */
/**
 * @brief dot_product
 * Calculate dot(scalar) product of two vectors(points)
 * @param[in] vertice0              First vertice
 * @param[in] vertice1              Second vertice
 * @return                          Dot(scalar) product
 */
__inline float dot_product(point3dF32 vertice0, point3dF32 vertice1);
/**
 * @brief normalize
 * Normalize vector values by coordinates
 * @param[out] vector               Vector to be normalized
 */
void normalize(point3dF32& vector);
/**
 * @brief cross_product
 * Calculate cross(vector) product of two vectors(points)
 * @param[in] vertice0              First vertice
 * @param[in] vertice1              Second vertice
 * @return                          Cross(vector) product
 */
point3dF32 cross_product(point3dF32 vertice0, point3dF32 vertice1);

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
    std::unique_ptr<context_interface> realsense_context;
    rs::device* realsense_device;
    std::unique_ptr<projection_interface> realsense_projection;
    rs::stream color_stream = rs::stream::color;

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
            std::cerr << "\nError: Playback file is not accessible" << std::endl;
            return -1;
        }
        realsense_context = std::unique_ptr<rs::playback::context>(new rs::playback::context(file_path.c_str()));
        realsense_device = realsense_context->get_device(0); // device is managed by context, no need to deallocate manually
        if (!realsense_device)
        {
            std::cerr << "\nError: Unable to get device" << std::endl;
            return -1;
        }
        std::vector<rs::stream> streams = { rs::stream::color, rs::stream::depth };

        bool expected_color_format_is_recorded = false, expected_depth_format_is_recorded = false;
        for(auto stream_iterator = streams.begin(); stream_iterator != streams.end(); stream_iterator++)
        {
            if(realsense_device->get_stream_mode_count(*stream_iterator) > 0)
            {
                int width, height, fps;
                rs::format format;
                realsense_device->get_stream_mode(*stream_iterator, 0, width, height, format, fps);
                if ((*stream_iterator) == rs::stream::color && format == rs::format::bgra8)
                {
                    expected_color_format_is_recorded = true;
                }
                if ((*stream_iterator) == rs::stream::depth && format == rs::format::z16)
                {
                    expected_depth_format_is_recorded = true;
                }
                realsense_device->enable_stream(*stream_iterator, width, height, format, fps);
            }
        }
        if (!expected_color_format_is_recorded)
        {
            std::cerr << "\nError: unexpected pixel format is recorded for color stream. The pixel format must be: bgra8" << std::endl;
            return -1;
        }
        if (!expected_depth_format_is_recorded)
        {
            std::cerr << "\nError: unexpected pixel format is recorded for depth stream. The pixel format must be: z16" << std::endl;
            return -1;
        }
    }
    else /* camera live streaming */
    {
        realsense_context = std::unique_ptr<context>(new context());
        if(realsense_context->get_device_count() == 0)
        {
            std::cerr << "\nError: No device detected" << std::endl;
            return -1;
        }
        realsense_device = realsense_context->get_device(0); // device is managed by context, no need to deallocate manually

        /* parsing resolution parameters */
        int default_depth_width = 628, default_depth_height = 468, default_color_width = 640, default_color_height = 480, default_fps = 30;
        struct stream_resolution_info { int width, height, fps; };
        // depth resolution
        const std::string depth_resolution = parser.get<std::string>("depth"); // parsing depth stream resolution
        stream_resolution_info depth_info;
        sscanf(depth_resolution.c_str(), "%dx%dx%d", &depth_info.width, &depth_info.height, &depth_info.fps);
        bool is_resolution_valid = false;
        try
        {
            for (int index = 0; ; index++)
            {
                int width, height, fps;
                rs::format format;
                realsense_device->get_stream_mode(rs::stream::depth, index, width, height, format, fps);
                if (format == rs::format::z16)
                    if ((width == depth_info.width) && (height == depth_info.height) && (fps == depth_info.fps))
                    {
                        is_resolution_valid = true;
                    }
            }
        }
        catch (rs::error)
        {
            if (!is_resolution_valid) // set default resolution if no stream mode parameters matched
            {
                std::cerr << "Provided DEPTH resolution is invalid. Using default resolution" << std::endl;
                depth_info.width = default_depth_width;
                depth_info.height = default_depth_height;
                depth_info.fps = default_fps;
            }
        }
        // color resolution
        const std::string color_resolution = parser.get<std::string>("color"); // parsing color stream resolution
        stream_resolution_info color_info;
        sscanf(color_resolution.c_str(), "%dx%dx%d", &color_info.width, &color_info.height, &color_info.fps);
        is_resolution_valid = false;
        try
        {
            for (int index = 0; ; index++)
            {
                int width, height, fps;
                rs::format format;
                realsense_device->get_stream_mode(rs::stream::color, index, width, height, format, fps);
                if (format == rs::format::bgra8)
                    if ((width == color_info.width) && (height == color_info.height) && (fps == color_info.fps))
                    {
                        is_resolution_valid = true;
                    }
            }
        }
        catch (rs::error)
        {
            if (!is_resolution_valid) // set default resolution if no stream mode parameters matched
            {
                std::cerr << "Provided COLOR resolution is invalid. Using default resolution" << std::endl;
                color_info.width = default_color_width;
                color_info.height = default_color_height;
                color_info.fps = default_fps;
            }
        }

        realsense_device->enable_stream(rs::stream::depth, depth_info.width, depth_info.height, rs::format::z16, depth_info.fps);
        realsense_device->enable_stream(rs::stream::color, color_info.width, color_info.height, rs::format::bgra8, color_info.fps);
        color_stream = rs::stream::rectified_color;
    }
    if (!(realsense_device->is_stream_enabled(rs::stream::depth) && realsense_device->is_stream_enabled(rs::stream::color))
            && (realsense_device->get_stream_format(rs::stream::color) == rs::format::bgra8))
    {
        std::cerr << "\nError: Streaming is not enabled" << std::endl;
        return -1;
    }

    intrinsics color_intrin = convert_intrinsics(realsense_device->get_stream_intrinsics(color_stream));
    intrinsics depth_intrin = convert_intrinsics(realsense_device->get_stream_intrinsics(rs::stream::depth));

    const int depth_width = depth_intrin.width;
    const int depth_height = depth_intrin.height;
    const int color_width = color_intrin.width;
    const int color_height = color_intrin.height;

    extrinsics extrin = convert_extrinsics(realsense_device->get_extrinsics(rs::stream::depth, color_stream));
    realsense_projection = std::unique_ptr<projection_interface>(
        projection_interface::create_instance(&color_intrin, &depth_intrin, &extrin));

    // creating drawing object
    projection_gui gui_handler(depth_width, depth_height, color_width, color_height);

    bool continue_showing = true;
    int gui_status = 0;
    realsense_device->start();
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
        const int color_pitch = color_width * image_utils::get_pixel_size(rs::format::bgra8);

        // image objects (image info and data pointers)
        image_info  depth_info = {depth_width, depth_height, convert_pixel_format(rs::format::z16), depth_pitch};
        std::unique_ptr<image_interface> depth(image_interface::create_instance_from_raw_data(&depth_info,
                                               depth_data,
                                               stream_type::depth,
                                               image_interface::flag::any,
                                               realsense_device->get_frame_timestamp(rs::stream::depth),
                                               realsense_device->get_frame_number(rs::stream::depth),
                                               nullptr,
                                               nullptr));
        image_info  color_info = {color_width, color_height, convert_pixel_format(rs::format::bgra8), color_pitch};
        std::unique_ptr<image_interface> color(image_interface::create_instance_from_raw_data(&color_info,
                                               color_data,
                                               stream_type::color,
                                               image_interface::flag::any,
                                               realsense_device->get_frame_timestamp(rs::stream::color),
                                               realsense_device->get_frame_number(rs::stream::color),
                                               nullptr,
                                               nullptr));

        const std::unique_ptr<uint8_t> world_data =
            std::unique_ptr<uint8_t>(create_world_data(realsense_projection.get(), depth_data, depth.get(), depth_width, depth_height)); // deallocation is handled by smart ptr
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
            status = realsense_projection->query_uvmap(depth.get(), uvmap.data());
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
                    if(i < 0) continue; if(j < 0) continue; // skip invalid pixel coordinates
                    uvmap_data[u+v*depth_width] = ((uint32_t*)color_data)[i+j*color_width];
                }
            }
            gui_handler.create_image((const void*)uvmap_data.data(), image_type::uvmap, CV_8UC4);
            status = status_no_error;
        }
        if (gui_handler.is_invuvmap_queried()) // inversed uvmap
        {
            /* Documentation reference: query_invuvmap function */
            std::vector<pointF32> invuvmap(color_width*color_height);
            status = realsense_projection->query_invuvmap(depth.get(), invuvmap.data());
            if (status != status_no_error)
            {
                std::cerr << "\nError: query_invuvmap bad status returned: " << status << std::endl;
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
                    if(u < 0) continue; if(v < 0) continue; // skip invalid pixel coordinates
                    uint16_t depth_value = ((uint16_t*)depth_data)[u+v*depth_width];
                    if(depth_value > 0)
                    {
                        invuvmap_data[i+j*color_width] = ((uint16_t*)depth_data)[u+v*depth_width];
                    }
                }
            }
            gui_handler.create_image((const void*)invuvmap_data.data(), image_type::invuvmap, CV_16UC1);
            status = status_no_error;
        }
        if (gui_handler.is_color_to_depth_queried()) // color image mapped to depth
        {
            /* Documentation reference: create_color_image_mapped_to_depth function */
            smart_ptr<image_interface> color2depth_image(realsense_projection->create_color_image_mapped_to_depth(depth.get(), color.get()));
            const void* color2depth_data = color2depth_image->query_data();
            if (!color2depth_data)
            {
                std::cerr << "Unable to get color mapped depth data" << std::endl;
                return -1;
            }
            gui_handler.create_image((const void*)color2depth_data, image_type::color2depth, CV_8UC4); // processing data generated internally
        }
        if (gui_handler.is_depth_to_color_queried()) // depth image mapped to color
        {
            /* Documentation reference: create_depth_image_mapped_to_color function */
            smart_ptr<image_interface> depth2color_image(realsense_projection->create_depth_image_mapped_to_color(depth.get(), color.get()));
            const void* depth2color_data = depth2color_image->query_data();
            if (!depth2color_data)
            {
                std::cerr << "Unable to get depth mapped color data" << std::endl;
                return -1;
            }
            gui_handler.create_image((const void*)depth2color_data, image_type::depth2color, CV_16UC1); // processing data generated internally
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
                status = realsense_projection->map_color_to_depth(depth.get(), drawn_points_size, color_points.data(), depth_points.data());
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
                status = realsense_projection->map_color_to_depth(depth.get(), drawn_points_size, color_points.data(), world_points.data());
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
    std::cout << "\nFinished streaming. Exiting. Goodbye!" << std::endl;
    return 0;
}

uint8_t* create_world_data(projection_interface* realsense_projection, const void* depth_data, image_interface* depth, int depth_width, int depth_height)
{
    std::vector<point3dF32> matrix(depth_width*depth_height);
    const int pitch = depth_width;
    uint8_t* raw_data = new uint8_t[pitch * depth_height]; // an array to store z-value of vertices
    uint8_t* raw_data_ptr = raw_data; // pointer to the raw data
    if (!depth->query_data())
    {
        std::cerr << "\nError: Unable to query data from image object" << std::endl;
        delete[] raw_data;
        return nullptr;
    }
    /* Documentation reference: query_vertices function */
    status status_ = realsense_projection->query_vertices(depth, matrix.data());
    if (status_ != status_no_error)
    {
        std::cerr << "\nError: query_vertices bad status returned: " << status_ << std::endl;
        delete[] raw_data;
        return nullptr;
    }
    for (int i = 0; i < depth_height; i++)
    {
        for (int j = 0; j < depth_width; j++)
        {
            raw_data[j + i * depth_width] = matrix[j + i * depth_width].z;
        }
    }
    /* light */
    float cursor_pos_x = 1.f, cursor_pos_y = 1.f; // the light source is in the top-left corner
    point3dF32 light;
    light.x = cursor_pos_x / (float)depth_width - 0.5f;
    light.y = 0.5f - cursor_pos_y / (float)depth_height;
    light.z = -1.0f;
    float light_intensity;

    /*
     * imagining the image as a number of quadrilaterals(square-shaped polygons)
     * to create 3D effect
     */
    for (int j = 1; j < depth_height - 1; j++)
    {
        for (int i = 1; i < depth_width - 1; i++)
        {
            uint16_t invalid_depth_range = ((uint16_t*)depth_data)[i + j * depth_width];
            if (invalid_depth_range < 500 && invalid_depth_range > 7000) // 100 units == 0.1 metres
            {
                raw_data_ptr[i] = raw_data_ptr[i+1] = raw_data_ptr[i+2] = raw_data_ptr[i+3] = 0;
                continue;
            }
            /* applying lighting */
            // getting square midpoint(core)
            point3dF32 vertice0 = matrix[j * depth_width + i];
            // getting 4 vertices of the square
            point3dF32 vertice1 = matrix[(j-1) * depth_width + (i-1)];
            point3dF32 vertice2 = matrix[(j-1) * depth_width + (i+1)];
            point3dF32 vertice3 = matrix[(j+1) * depth_width + (i+1)];
            point3dF32 vertice4 = matrix[(j+1) * depth_width + (i-1)];
            // taking their value dependent on the midpoint(core)
            vertice1.x = vertice1.x-vertice0.x; vertice1.y = vertice1.y-vertice0.y; vertice1.z = vertice1.z-vertice0.z;
            vertice2.x = vertice2.x-vertice0.x; vertice2.y = vertice2.y-vertice0.y; vertice2.z = vertice2.z-vertice0.z;
            vertice3.x = vertice3.x-vertice0.x; vertice3.y = vertice3.y-vertice0.y; vertice3.z = vertice3.z-vertice0.z;
            vertice4.x = vertice4.x-vertice0.x; vertice4.y = vertice4.y-vertice0.y; vertice4.z = vertice4.z-vertice0.z;
            // getting surface normals for each of the 4 triangles(zones) of the square; normalizing them after
            point3dF32 surface_normal1 = cross_product(vertice1,vertice2); normalize(surface_normal1);
            point3dF32 surface_normal2 = cross_product(vertice2,vertice3); normalize(surface_normal2);
            point3dF32 surface_normal3 = cross_product(vertice3,vertice4); normalize(surface_normal3);
            point3dF32 surface_normal4 = cross_product(vertice4,vertice1); normalize(surface_normal4);
            // getting average normal to square
            surface_normal1.x += surface_normal2.x + surface_normal3.x + surface_normal4.x;
            surface_normal1.y += surface_normal2.y + surface_normal3.y + surface_normal4.y;
            surface_normal1.z += surface_normal2.z + surface_normal3.z + surface_normal4.z;
            normalize(surface_normal1);
            normalize(light);
            // getting mirrored light's coefficient
            light_intensity = dot_product(surface_normal1, light);
            // setting pixel values to the light's intensity
            raw_data_ptr[i] = raw_data_ptr[i+1] = raw_data_ptr[i+2] = uint8_t(abs(light_intensity)*255);
            raw_data_ptr[i+3] = 0xff;
        }
        raw_data_ptr += pitch;
    }
    return raw_data;
}


/* helper functions */
float dot_product(point3dF32 vertice0, point3dF32 vertice1)
{
    return vertice0.x * vertice1.x + vertice0.y * vertice1.y + vertice0.z * vertice1.z;
}

void normalize(point3dF32& vector)
{
    auto vector_length = vector.x*vector.x + vector.y*vector.y + vector.z*vector.z;
    if (vector_length>0)
    {
        vector_length = sqrt(vector_length);
        vector.x = vector.x/vector_length; vector.y = vector.y/vector_length; vector.z = vector.z/vector_length;
    }
}

point3dF32 cross_product(point3dF32 vertice0, point3dF32 vertice1)
{
    point3dF32 vec = {vertice0.y*vertice1.z - vertice0.z*vertice1.y, vertice0.z*vertice1.x - vertice0.x*vertice1.z, vertice0.x*vertice1.y - vertice0.y*vertice1.x };
    return vec;
}
