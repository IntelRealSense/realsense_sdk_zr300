// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/* realsense sdk */
#include "rs_sdk.h"
#include "basic_cmd_util.h"
#include "projection_viewer.h"
#include "projection_cmd_util.h"
#include "rs_sdk_version.h"

/* librealsense */
#include "librealsense/rs.hpp"

/* standard library */
#include <iostream>
#include <fstream>
#include <functional>
#include <mutex>
#include <thread>

/* See the samples for detailed description of the realsense api */
using namespace rs::core;
using namespace rs::utils;

/** @brief configure
 *
 * Enable streams of the device and initialize samples time sync iterface instance.
 * @param[in] device                Device instance.
 * @param[in] cmd_utility           Command line utility instance.
 * @param[out] sync_utility         Samples time sync interface instance to be initialized.
 * @return: 0                       Sucessfully configured device and sync utility.
 * @return: -1                      Configuring failed.
 */
int configure(rs::device* device, basic_cmd_util& cmd_utility, unique_ptr<samples_time_sync_interface>& sync_utility);

/** @brief create_frame_callback
 *
 * Create frame callback for device.
 * @param[in] sync_utility          Samples time sync interface instance.
 * @param[in] process_sample        Sample processing function called when a pair of color-depth frames is found.
 * @param[in] continue_streaming    Flag to check if the streaming is over.
 * @return: std::function<void(rs::frame)> Created frame callback.
 */
std::function<void(rs::frame)> create_frame_callback(unique_ptr<samples_time_sync_interface>& sync_utility,
                                                     std::function<void(const correlated_sample_set&)> process_sample,
                                                     projection_viewer& renderer,
                                                     bool& continue_streaming);

/** @brief create_processing_function
 *
 * Create function to process correlated frames that calls projection methods on such frames.
 * @param[in] projection            Projection interface instance.
 * @param[in] renderer              Projection viewer instance.
 * @param[in] world_data            A buffer of real world Z-coordinate points to be populated and rendered internally.
 * @param[in] depth_scale           Value for the mapping between depth image units and meters.
 * @param[in] process_sample_called Flag to check if the function was called.
 *                                  Used to warn the user whether sync utility managed to find a correlated pair of frames.
 * @return: std::function<void(const correlated_sample_set&)> Created processing function.
 */
std::function<void(const correlated_sample_set&)> create_processing_function(projection_interface* projection, projection_viewer& renderer,
                                                                             std::vector<uint16_t>& world_data, const double depth_scale,
                                                                             bool& process_sample_called);

/** @brief create_world_data
 *
 * Create real world image and get it as raw data.
 * @param[in] realsense_projection  RealSense projection object.
 * @param[in] depth                 Depth image.
 * @param[in] world_data            Real world data vector to return the mapped data into.
 * @return: 0                       Real world data successfully created.
 * @return: -1                      Failed to create real world data.
*/
const int create_world_data(projection_interface* realsense_projection, image_interface* depth, std::vector<uint16_t>& world_data);

/** @brief handle_uvmap
 *
 * Process uvmap scenario using projection to then render it on color stream of the main window.
 * @param[in] is_uvmap_queried      Flag value to specify if uvmap was requested by user through UI.
 * @param[in] max_depth_distance    Max depth distance specified by user to be used for filtering output points vector.
 * @param[in] scale                 Value for the mapping between depth image units and meters.
 * @param[in] projection            Projection interface instance.
 * @param[in] depth                 Depth image.
 * @param[in] color                 Color image.
 * @return: std::vector<pointF32>   Color points vector calculated from uvmap and filtered by the depth range.
 */
std::vector<pointF32> handle_uvmap(const bool is_uvmap_queried, const float max_depth_distance, const double scale,
                                   projection_interface* projection, image_interface* depth, image_interface* color);
/** @brief handle_invuvmap
 *
 * Process invuvmap scenario using projection api to then render it on depth stream of the main window.
 * @param[in] is_invuvmap_queried   Flag value to specify invuvmap was requested by user through UI.
 * @param[in] max_depth_distance    Max depth distance specified by user to be used for filtering output points vector.
 * @param[in] scale                 Value for the mapping between depth image units and meters.
 * @param[in] projection            Projection interface instance.
 * @param[in] depth                 Depth image.
 * @param[in] color                 Color image.
 * @return: std::vector<pointF32>   Depth points vector calculated from invuvmap and filtered by the depth range.
 */
std::vector<pointF32> handle_invuvmap(const bool is_invuvmap_queried, const float max_depth_distance, const double scale,
                                      projection_interface* projection, image_interface* depth, image_interface* color);

/** @brief handle_points_mapping
 *
 * Process points mapping scenario using projection to then render drawn and mapped points on corresponding streams of the main window.
 * @param[in] projection            Projection interface instance.
 * @param[in] depth                 Depth image.
 * @param[in] renderer              Projection viewer instance.
 */
void handle_points_mapping(projection_interface* projection, image_interface* depth, projection_viewer& renderer);

std::mutex sync_mutex;

int main(int argc, char* argv[])
{
    std::unique_ptr<context_interface> realsense_context;
    rs::device* realsense_device;
    unique_ptr<projection_interface> realsense_projection;
    unique_ptr<samples_time_sync_interface> sync_utility;
    rs::stream color_stream = rs::stream::color;

    cmd_option options;
    projection_cmd_util cmd_utility;
    if(argc != 1 && !cmd_utility.parse(argc, argv))
    {
        std::cerr << "\nError: Wrong command line options" << std::endl;
        std::cout << cmd_utility.get_help();
        return -1;
    }

    if(cmd_utility.get_cmd_option("-h --h -help --help -?", options))
    {
        std::cout << cmd_utility.get_help();
        return 0;
    }

    if (cmd_utility.get_streaming_mode() == streaming_mode::playback)
    {
        const std::string file_path = cmd_utility.get_file_path(streaming_mode::playback);
        if (!(std::ifstream(file_path.c_str()).good())) // check that file is accessible
        {
            std::cerr << "\nError: Playback file is not accessible. Probably, wrong path specified" << std::endl;
            return -1;
        }
        realsense_context = std::unique_ptr<rs::playback::context>(new rs::playback::context(file_path.c_str()));
    }
    if (cmd_utility.get_streaming_mode() == streaming_mode::live)
    {
        realsense_context = std::unique_ptr<context>(new context());
        if(realsense_context->get_device_count() == 0)
        {
            std::cerr << "\nError: No device detected" << std::endl;
            return -1;
        }
        color_stream = rs::stream::rectified_color; // get rectified_color from librealsense device for correct intrinsics/extrinsics
    }
    realsense_device = realsense_context->get_device(0);

    // configure device and init sync utility
    if (configure(realsense_device, cmd_utility, sync_utility) != 0)
    {
        std::cerr << "\nError: Unable to configure device\n";
        return -1;
    }

    bool continue_streaming = true;
    // create renderer
    projection_viewer renderer(
        {realsense_device->get_stream_width(rs::stream::color), realsense_device->get_stream_height(rs::stream::color)},
        {realsense_device->get_stream_width(rs::stream::depth), realsense_device->get_stream_height(rs::stream::depth)},
        [&continue_streaming]
        {
            continue_streaming = false;
        });

    intrinsics color_intrin = convert_intrinsics(realsense_device->get_stream_intrinsics(color_stream));
    intrinsics depth_intrin = convert_intrinsics(realsense_device->get_stream_intrinsics(rs::stream::depth));
    extrinsics extrin = convert_extrinsics(realsense_device->get_extrinsics(rs::stream::depth, color_stream));
    // create projection instance
    realsense_projection = get_unique_ptr_with_releaser(
        projection_interface::create_instance(&color_intrin, &depth_intrin, &extrin));


    // creating real world image data buffer for future
    std::vector<uint16_t> world_data(depth_intrin.width*depth_intrin.height);
    // get depth scale to get depth value units as meters
    const double depth_scale = realsense_device->get_depth_scale();
    bool process_sample_called = false;
    // processing of frame data when sync utility found a pair of correlated frames
    auto process_sample = create_processing_function(realsense_projection.get(), renderer, world_data, depth_scale, process_sample_called);
    // create frame callback for rs::device
    auto frame_callback = create_frame_callback(sync_utility, process_sample, renderer, continue_streaming);

    realsense_device->set_frame_callback(rs::stream::depth, frame_callback);
    realsense_device->set_frame_callback(rs::stream::color, frame_callback);
    realsense_device->start();
    while(realsense_device->is_streaming())
    {
        renderer.process_user_events(); // user events are processed in the main thread as required by GLFW
        if (!continue_streaming) // the value is changed by process_user_events()
        {
            break;
        }
    }

    renderer.terminate();
    {
        std::lock_guard<std::mutex> lock(sync_mutex);
        sync_utility->flush();
        sync_utility.reset(); // prevent samples_time_sync from processing new frames
                              // which can result in a deadlock on device->stop()
    }
    realsense_device->stop();
    if (!process_sample_called)
    {
        std::cerr << "\nWarning: Sync utility did not manage to match frames" << std::endl;
        return 0;
    }

    std::cout << "Finished streaming. Exiting. Goodbye!" << std::endl;
    return 0;
}


int configure(rs::device* device, basic_cmd_util& cmd_utility, unique_ptr<samples_time_sync_interface>& sync_utility)
{
    const streaming_mode mode = cmd_utility.get_streaming_mode();
    std::vector<rs::core::stream_type> streams = { rs::core::stream_type::depth, rs::core::stream_type::color };
    int streams_fps[static_cast<int>(stream_type::max)] = {0};
    int motions_fps[static_cast<int>(motion_type::max)] = {0};
    switch(mode)
    {
    case streaming_mode::playback:
    {
        bool color_stream_is_recorded = false, depth_stream_is_recorded = false; // check that color and depth are recorded
        bool expected_color_format_is_recorded = false, expected_depth_format_is_recorded = false; // check that streams have valid pixel_format
        for(auto stream = streams.begin(); stream != streams.end(); stream++)
        {
            auto lrs_stream = convert_stream_type(*stream);
            if(device->get_stream_mode_count(lrs_stream) > 0)
            {
                int width, height, fps;
                rs::format format;
                device->get_stream_mode(lrs_stream, 0, width, height, format, fps);
                if ((lrs_stream) == rs::stream::color)
                {
                    color_stream_is_recorded = true;
                    if (format == rs::format::bgra8
                        || format == rs::format::bgr8
                        || format == rs::format::rgba8
                        || format == rs::format::rgb8)
                    {
                        expected_color_format_is_recorded = true;
                    }
                }
                if ((lrs_stream) == rs::stream::depth)
                {
                    depth_stream_is_recorded = true;
                    if(format == rs::format::z16)
                    {
                        expected_depth_format_is_recorded = true;
                    }
                }
                device->enable_stream(lrs_stream, width, height, format, fps);
                streams_fps[static_cast<int>(lrs_stream)] = device->get_stream_framerate(lrs_stream);
            }
        }
        if (!color_stream_is_recorded)
        {
            std::cerr << "\nError: Color stream is not recorded.\n";
            return -1;
        }
        if (!depth_stream_is_recorded)
        {
            std::cerr << "\nError: Depth stream is not recorded.\n";
            return -1;
        }
        if (!expected_color_format_is_recorded)
        {
            std::cerr << "\nError: unexpected pixel format is recorded for COLOR stream\n";
            cmd_utility.get_help();
            return -1;
        }
        if (!expected_depth_format_is_recorded)
        {
            std::cerr << "\nError: unexpected pixel format is recorded for DEPTH stream\n";
            cmd_utility.get_help();
            return -1;
        }

        break;
    }
    case streaming_mode::live:
    {
        try // catch the cases when requested stream profile is not available/does not exist
        {
            for(auto stream = streams.begin(); stream != streams.end(); ++stream)
            {
                auto lrs_stream = convert_stream_type(*stream);
                if(device->get_stream_mode_count(lrs_stream) == 0) continue;
                device->enable_stream(lrs_stream,
                                      cmd_utility.get_stream_width(*stream),
                                      cmd_utility.get_stream_height(*stream),
                                      convert_pixel_format(cmd_utility.get_stream_pixel_format(*stream)),
                                      cmd_utility.get_stream_fps(*stream));
                streams_fps[static_cast<int>(lrs_stream)] = device->get_stream_framerate(lrs_stream);
            }
        }
        catch (rs::error e)
        {
            std::cerr << std::endl << "what(): " << e.what() << std::endl;
            return -1;
        }

        break;
    }
    default:
        std::cerr << "streaming mode is not supported\n";
        return -1;
    }
    sync_utility = rs::utils::get_unique_ptr_with_releaser(
                         rs::utils::samples_time_sync_interface::create_instance(streams_fps, motions_fps, device->get_name()));
    return 0;
}

std::function<void(rs::frame)> create_frame_callback(unique_ptr<samples_time_sync_interface>& sync_utility,
                                                     std::function<void(const correlated_sample_set&)> process_sample,
                                                     projection_viewer& renderer,
                                                     bool& continue_streaming)
{
    return [&sync_utility, process_sample, &renderer, &continue_streaming] (rs::frame new_frame)
    {
        std::lock_guard<std::mutex> lock(sync_mutex);
        if (!sync_utility)
            return;
        if (!continue_streaming)
        {
            renderer.update();
            return;
        }
        auto image = get_unique_ptr_with_releaser(image_interface::create_instance_from_librealsense_frame(new_frame, image_interface::flag::any));

        //create a container for correlated sample set
        correlated_sample_set sample = {};

        // push the image to the sample time sync
        // time sync may return correlated sample set - check the status
        if (sync_utility->insert(image.get(), sample))
        {
            // correlated sample set found - give it to projection and renderer
            // only synced frames are to be used in projection
            process_sample(sample);
            sample = {};
            renderer.update();
        }
    };
}

std::function<void(const correlated_sample_set&)> create_processing_function(projection_interface* projection, projection_viewer& renderer,
                                                                             std::vector<uint16_t>& world_data, const double depth_scale,
                                                                             bool& process_sample_called)
{
    return [projection, &renderer, &world_data, depth_scale, &process_sample_called] (const correlated_sample_set& sample)
    {
        process_sample_called = true;

        auto depth = get_unique_ptr_with_releaser(sample[stream_type::depth]);
        auto color = get_unique_ptr_with_releaser(sample[stream_type::color]);

        const int depth_width = depth->query_info().width;
        const int depth_height = depth->query_info().height;

        // rendering synthetically created by projection real world image
        if (create_world_data(projection, depth.get(), world_data) != 0)
        {
            throw std::runtime_error("Unable to create world data");
        }
        const int world_pitch = depth_width * get_pixel_size(pixel_format::z16);
        image_info world_info = {depth_width, depth_height, pixel_format::z16, world_pitch};
        auto world = get_unique_ptr_with_releaser(image_interface::create_instance_from_raw_data(
                                                      &world_info,
                                                      {(const void*)world_data.data(), nullptr},
                                                      stream_type::depth,
                                                      image_interface::flag::any,
                                                      0, 0));

        renderer.show_stream(image_type::depth, depth.get());
        renderer.show_stream(image_type::color, color.get());
        renderer.show_stream(image_type::world, world.get());

        std::vector<pointF32> points;
        points = handle_uvmap(renderer.is_uvmap_queried(), renderer.get_current_max_depth_distance(), depth_scale,
                              projection, depth.get(), color.get()); // uvmap
        if (!points.empty())
        {
            // show points based on uvmap pixel coordinates calculations and in the specified depth range
            renderer.draw_points(image_type::uvmap, points);
        }

        points.clear();
        points = handle_invuvmap(renderer.is_invuvmap_queried(), renderer.get_current_max_depth_distance(), depth_scale,
                                 projection, depth.get(), color.get()); // invuvmap
        if (!points.empty())
        {
            // show points based on invuvmap pixel coordinates calculations and in the specified depth range
            renderer.draw_points(image_type::invuvmap, points);
        }
        if (renderer.is_color_to_depth_queried()) // color image mapped to depth
        {
            /* Documentation reference: create_color_image_mapped_to_depth function */
            auto color2depth_image = get_unique_ptr_with_releaser(projection->create_color_image_mapped_to_depth(depth.get(), color.get()));
            const void* color2depth_data = color2depth_image->query_data();
            if (!color2depth_data)
            {
                throw std::runtime_error("Unable to get color mapped depth data");
            }
            // show color image mapped to depth in a separate window
            renderer.show_window(color2depth_image.get());
        }
        if (renderer.is_depth_to_color_queried()) // depth image mapped to color
        {
            /* Documentation reference: create_depth_image_mapped_to_color function */
            auto depth2color_image = get_unique_ptr_with_releaser(projection->create_depth_image_mapped_to_color(depth.get(), color.get()));
            const void* depth2color_data = depth2color_image->query_data();
            if (!depth2color_data)
            {
                throw std::runtime_error("Unable to get depth mapped color data");
            }
            // show depth image mapped to color in a separate window
            renderer.show_window(depth2color_image.get());
        }

        handle_points_mapping(projection, depth.get(), renderer);
    };
}


const int create_world_data(projection_interface* realsense_projection, image_interface* depth, std::vector<uint16_t>& world_data)
{
    if (!depth->query_data())
    {
        std::cerr << "\nError: Unable to query data from image object" << std::endl;
        return -1;
    }
    const int depth_width = depth->query_info().width;
    const int depth_height = depth->query_info().height;
    std::vector<point3dF32> matrix(depth_width*depth_height);
    /* Documentation reference: query_vertices function */
    status status_ = realsense_projection->query_vertices(depth, matrix.data());
    if (status_ != status_no_error)
    {
        std::cerr << "\nError: query_vertices bad status returned: " << status_ << std::endl;
        return -1;
    }
    for (int i = 0; i < depth_height; i++)
    {
        for (int j = 0; j < depth_width; j++)
        {
            // copy z value from vertices array to world_data
            world_data[j + i * depth_width] = static_cast<uint16_t>(matrix[j + i * depth_width].z);
        }
    }
    return 0;
}

std::vector<pointF32> handle_uvmap(const bool is_uvmap_queried, const float max_depth_distance, const double scale,
                                   projection_interface* projection, image_interface* depth, image_interface* color)
{
    if (!is_uvmap_queried)
    {
        return std::vector<pointF32>();
    }
    const int depth_width = depth->query_info().width;
    const int depth_height = depth->query_info().height;
    const int color_width = color->query_info().width;
    const int color_height = color->query_info().height;
    const void* depth_data = depth->query_data();

    /* Documentation reference: query_uvmap function */
    std::vector<pointF32> uvmap(depth_width*depth_height);
    status sts = projection->query_uvmap(depth, uvmap.data());
    if (sts != status_no_error)
    {
        std::cerr << "\nError: query_uvmap bad status returned: " << sts << std::endl;
        return std::vector<pointF32>();
    }
    // using uvmap to find color pixels coordinates
    std::vector<pointF32> points;
    for(int v = 0; v < depth_height; v++)
    {
        for (int u = 0; u < depth_width; u++)
        {
            // u, v - indexes of depth pixel
            // i, j - indexes of mapped color pixel
            const float i = (float)uvmap[u+v*depth_width].x*(float)color_width;
            const float j = (float)uvmap[u+v*depth_width].y*(float)color_height;
            if (i < 0) continue; if (j < 0) continue; // skip invalid pixel coordinates
            const uint16_t depth_value = ((uint16_t*)depth_data)[int(u)+int(v)*depth_width];
            // check the depth value
            // draw each point for which the corresponding depth value is in specified depth range
            if (float(depth_value*scale) > 0.f && float(depth_value*scale) <= max_depth_distance)
            {
                // add valid coordinates calculated by uvmap
                points.push_back({i, j});
            }
        }
    }
    return std::move(points);
}

std::vector<pointF32> handle_invuvmap(const bool is_invuvmap_queried, const float max_depth_distance, const double scale,
                        projection_interface* projection, image_interface* depth, image_interface* color)
{
    if (!is_invuvmap_queried)
    {
        return std::vector<pointF32>();
    }
    const int depth_width = depth->query_info().width;
    const int depth_height = depth->query_info().height;
    const int color_width = color->query_info().width;
    const int color_height = color->query_info().height;
    const void* depth_data = depth->query_data();

    /* Documentation reference: query_invuvmap function */
    std::vector<pointF32> invuvmap(color_width*color_height);
    status sts = projection->query_invuvmap(depth, invuvmap.data());
    if (sts != status_no_error)
    {
        std::cerr << "\nError: query_invuvmap bad status returned: " << sts << std::endl;
        return std::vector<pointF32>();
    }
    // using invuvmap to find depth pixels coordinates
    std::vector<pointF32> points;
    for(int i = 0; i < color_width; i++)
    {
        for(int j = 0; j < color_height; j++)
        {
            // i, j - indexes of color pixel
            // u, v - indexes of mapped depth pixel
            const float u = (float)invuvmap[i+j*color_width].x*(float)depth_width;
            const float v = (float)invuvmap[i+j*color_width].y*(float)depth_height;
            if(u < 0) continue; if(v < 0) continue; // skip invalid pixel coordinates
            const uint16_t depth_value = ((uint16_t*)depth_data)[int(u)+int(v)*depth_width];
            // check the depth value
            // draw each point for which the corresponding depth value is in specified depth range
            if (float(depth_value*scale) > 0.f && float(depth_value*scale) <= max_depth_distance)
            {
                // add valid coordinates calculated by invuvmap
                points.push_back({u, v});
            }
        }
    }
    return std::move(points);
}

void handle_points_mapping(projection_interface* projection, image_interface* depth, projection_viewer& renderer)
{
    const void* depth_data = depth->query_data();
    const int depth_width = depth->query_info().width;
    status sts = status::status_no_error;
    /* drawing on images */
    switch(renderer.image_with_drawn_points())
    {
        case image_type::depth:
        {
            std::vector<pointF32> drawn_points(renderer.get_points());
            int drawn_points_size = static_cast<int>(drawn_points.size());
            std::vector<point3dF32> depth_points_3D(drawn_points_size);
            for(int i = 0; i < drawn_points_size; i++)
            {
                depth_points_3D[i].x = drawn_points[i].x;
                depth_points_3D[i].y = drawn_points[i].y;
                depth_points_3D[i].z = ((int16_t*)depth_data)[static_cast<int>(drawn_points[i].x) + static_cast<int>(drawn_points[i].y) * depth_width];
            }

            /* Documentation reference: map_depth_to_color function */
            // mapping depth image to color image
            std::vector<pointF32> color_points(drawn_points_size);
            sts = projection->map_depth_to_color(drawn_points_size, depth_points_3D.data(), color_points.data());
            if (sts != status_no_error)
            {
                std::cerr << "Cannot map_depth_to_color. Status: " << sts << std::endl;
                break;
            }

            /* Documentation reference: project_depth_to_camera function */
            // projecting depth image to image from camera
            std::vector<point3dF32> world_points_3D(drawn_points_size);
            sts = projection->project_depth_to_camera(drawn_points_size, depth_points_3D.data(), world_points_3D.data());
            if (sts != status_no_error)
            {
                std::cerr << "Cannot project_depth_to_camera. Status: " << sts << std::endl;
                break;
            }

            for (int i = 0; i < drawn_points_size; i++)
            {
                if (world_points_3D[i].z <= 0) world_points_3D[i].z = -1.0f;
            }

            /* Documentation reference: project_camera_to_depth function */
            // projecting image from camera to world image
            std::vector<pointF32> world_points(drawn_points_size);
            sts = projection->project_camera_to_depth(drawn_points_size, world_points_3D.data(), world_points.data());
            if (sts != status_no_error)
            {
                std::cerr << "Cannot project_camera_to_depth. Status: " << sts << std::endl;
                break;
            }

            std::vector<pointF32> color_points_to_draw, world_points_to_draw;
            for (int i = 0; i < drawn_points_size; i++)
            {
                bool depth_point_valid = (depth_points_3D[i].z > 0);
                if (depth_point_valid) // add only valid points
                {
                    color_points_to_draw.push_back({color_points[i].x, color_points[i].y});
                    world_points_to_draw.push_back({world_points[i].x, world_points[i].y});
                }
            }

            // show original and mapped points
            renderer.draw_points(image_type::depth, drawn_points);
            renderer.draw_points(image_type::color, color_points_to_draw);
            renderer.draw_points(image_type::world, world_points_to_draw);
            sts = status_no_error;
            break;
        }
        case image_type::color:
        {
            std::vector<pointF32> drawn_points(renderer.get_points());
            int drawn_points_size = static_cast<int>(drawn_points.size());
            std::vector<pointF32> color_points(drawn_points_size);
            for(int i = 0; i < drawn_points_size; i++)
            {
                color_points[i].x = drawn_points[i].x;
                color_points[i].y = drawn_points[i].y;
            }

            /* Documentation reference: map_color_to_depth function */
            // mapping color image to depth image
            std::vector<pointF32> depth_points(drawn_points_size);
            sts = projection->map_color_to_depth(depth, drawn_points_size, color_points.data(), depth_points.data());
            if (sts != status_no_error)
            {
                if (sts != status_value_out_of_range)
                {
                    std::cerr << "Cannot map_color_to_depth. Status: " << sts << std::endl;
                    break;
                }
            }

            /* Documentation reference: project_color_to_camera function */
            // projecting color image to image from camera
            std::vector<pointF32> world_points(drawn_points_size);
            sts = projection->map_color_to_depth(depth, drawn_points_size, color_points.data(), world_points.data());
            if (sts != status_no_error)
            {
                if (sts != status_value_out_of_range)
                {
                    std::cerr << "Cannot map_color_to_depth. Status: " << sts << std::endl;
                    break;
                }
            }

            std::vector<pointF32> depth_points_to_draw, world_points_to_draw;
            for (int i = 0; i < drawn_points_size; i++)
            {
                bool depth_points_mapped = (depth_points[i].x > 0);
                if (depth_points_mapped) // add only valid points
                {
                    depth_points_to_draw.push_back({depth_points[i].x, depth_points[i].y});
                }
                bool world_points_mapped = (world_points[i].x > 0);
                if (world_points_mapped) // add only valid points
                {
                    world_points_to_draw.push_back({world_points[i].x, world_points[i].y});
                }
            }

            // show original and mapped points
            renderer.draw_points(image_type::color, color_points);
            renderer.draw_points(image_type::depth, depth_points_to_draw);
            renderer.draw_points(image_type::world, world_points_to_draw);
            sts = status_no_error;
            break;
        }
        case image_type::world:
        {
            std::vector<pointF32> drawn_points(renderer.get_points());
            int drawn_points_size = static_cast<int>(drawn_points.size());
            std::vector<point3dF32> depth_points_3D(drawn_points_size);
            for(int i = 0; i < drawn_points_size; i++)
            {
                if (drawn_points[i].x < 0.f) continue;
                depth_points_3D[i].x = drawn_points[i].x;
                depth_points_3D[i].y = drawn_points[i].y;
                depth_points_3D[i].z = ((int16_t*)depth_data)[static_cast<int>(drawn_points[i].x) + static_cast<int>(drawn_points[i].y) * depth_width];
                if (depth_points_3D[i].z <= 0) depth_points_3D[i].z = -1.0f;
            }

            /* Documentation reference: project_depth_to_camera function */
            // projecting depth image to image from camera
            std::vector<point3dF32> world_points_3D(drawn_points_size);
            sts = projection->project_depth_to_camera(drawn_points_size, depth_points_3D.data(), world_points_3D.data());
            if (sts != status_no_error)
            {
                std::cerr << "Cannot project_depth_to_camera. Status: " << sts << std::endl;
                break;
            }

            /* Documentation reference: project_camera_to_depth function */
            // projecting image from camera to depth image
            std::vector<pointF32> depth_points(drawn_points_size);
            sts = projection->project_camera_to_depth(drawn_points_size, world_points_3D.data(), depth_points.data());
            if (sts != status_no_error)
            {
                std::cerr << "Cannot project_camera_to_depth. Status: " << sts << std::endl;
                break;
            }

            /* Documentation reference: project_camera_to_color function */
            // projecting image from camera to color image
            std::vector<pointF32> color_points(drawn_points_size);
            sts = projection->project_camera_to_color(drawn_points_size, world_points_3D.data(), color_points.data());
            if (sts != status_no_error)
            {
                std::cerr << "Cannot project_camera_to_color. Status: " << sts << std::endl;
                break;
            }

            std::vector<pointF32> color_points_to_draw, depth_points_to_draw;
            for (int i = 0; i < drawn_points_size; i++)
            {
                bool depth_points_valid = (depth_points_3D[i].z > 0);
                if (depth_points_valid) // add only valid points
                {
                    color_points_to_draw.push_back({color_points[i].x, color_points[i].y});
                    depth_points_to_draw.push_back({depth_points[i].x, depth_points[i].y});
                }
            }

            // show original and mapped points
            renderer.draw_points(image_type::world, drawn_points);
            renderer.draw_points(image_type::depth, depth_points_to_draw);
            renderer.draw_points(image_type::color, color_points_to_draw);
            sts = status_no_error;
            break;
        }
        default: break; // process nothing
    }
}
