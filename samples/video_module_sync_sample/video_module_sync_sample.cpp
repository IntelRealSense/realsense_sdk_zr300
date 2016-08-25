// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

// Video Module Synchronized Sample
// This sample demonstrates an application usage of a computer vision module, which implements a synchronous samples processing.
// The video module implements the video module interface, which is a common way for the application or SDK to interact with
// the module. It also implements a module specific interface, in this example - the module calculates the maximal depth value
// in the image.

#include <memory>
#include <vector>
#include <iostream>
#include <unistd.h>
#include <librealsense/rs.hpp>

#include "rs_sdk.h"
#include "rs/cv_modules/max_depth_value_module/max_depth_value_module.h"

using namespace std;
using namespace rs::core;
using namespace rs::utils;
using namespace rs::cv_modules;

int main (int argc, char* argv[])
{
    // initialize the device from live device or playback file, based on command line parameters.
    std::unique_ptr<context_interface> ctx;
    int frames_count = 0;

    if (argc > 1)
    {
        if(access(argv[1], F_OK) == -1)
        {
            cerr<<"error : playback file does not exists" << endl;
            return -1;
        }
        //use the playback file context
        auto playback_context = new rs::playback::context(argv[1]);
        ctx.reset(playback_context);

        //get frames count from the device
        auto playback_device = playback_context->get_playback_device();
        if (playback_device)
            frames_count = playback_device->get_frame_count();
    }
    else
    {
        //use a real context
        ctx.reset(new context());
        if(ctx->get_device_count() == 0)
        {
            cerr<<"error : cant find devices" << endl;
            return -1;
        }

        //default number of frames from a real device
        frames_count = 100;
    }

    rs::device * device = ctx->get_device(0); //device memory managed by the context

    // initialize the module
    std::unique_ptr<max_depth_value_module> module(new max_depth_value_module());

    // get the first supported module configuration
    const auto device_name = device->get_name();
    video_module_interface::supported_module_config supported_config = {};
    for(auto i = 0;; i++)
    {
        if (module->query_supported_module_config(i, supported_config) < status_no_error)
        {
            cerr<<"error : failed to query supported module configuration for index : " << i <<endl;
            return -1;
        }

        auto is_current_device_valid = (std::strcmp(device_name, supported_config.device_name) == 0) || supported_config.device_name == nullptr;
        if (!is_current_device_valid)
        {
            //skip config due to miss-matching the current connected device
            continue;
        }

        if(!(supported_config.config_flags & video_module_interface::supported_module_config::flags::sync_processing_supported))
        {
            //skip config due to unsupported processing mode
            continue;
        }

        break;
    }

    //construct an actual model configuration to be set on the module
    video_module_interface::actual_module_config actual_config = {};
    std::memcpy(actual_config.device_info.name, supported_config.device_name, std::strlen(supported_config.device_name));

    //enable the camera streams from the selected module configuration
    vector<stream_type> actual_streams;
    vector<stream_type> possible_streams =  { stream_type::depth,
                                              stream_type::color,
                                              stream_type::infrared,
                                              stream_type::infrared2,
                                              stream_type::fisheye
                                            };
    for(auto &stream : possible_streams)
    {
        //enable the stream from the supported module configuration
        auto supported_stream_config = supported_config[stream];

        if (!supported_stream_config.is_enabled)
            continue;

        rs::stream librealsense_stream = convert_stream_type(stream);

        bool is_matching_stream_mode_found = false;
        auto stream_mode_count = device->get_stream_mode_count(librealsense_stream);
        for(int j = 0; j < stream_mode_count; j++)
        {
            int width, height, frame_rate;
            rs::format format;
            device->get_stream_mode(librealsense_stream, j, width, height, format, frame_rate);
            bool is_acceptable_stream_mode = (width == supported_stream_config.ideal_size.width &&
                                              height == supported_stream_config.ideal_size.height &&
                                              frame_rate == supported_stream_config.ideal_frame_rate);
            if(is_acceptable_stream_mode)
            {
                device->enable_stream(librealsense_stream, width, height, format, frame_rate);

                video_module_interface::actual_image_stream_config &actual_stream_config = actual_config[stream];
                actual_stream_config.size.width = width;
                actual_stream_config.size.height= height;
                actual_stream_config.frame_rate = frame_rate;
                actual_stream_config.intrinsics = convert_intrinsics(device->get_stream_intrinsics(librealsense_stream));
                actual_stream_config.extrinsics = convert_extrinsics(device->get_extrinsics(rs::stream::depth, librealsense_stream));
                actual_stream_config.is_enabled = true;

                is_matching_stream_mode_found = true;

                break;
            }
        }

        if(is_matching_stream_mode_found)
        {
            actual_streams.push_back(stream);
        }
        else
        {
            cerr<<"error : didnt find matching stream configuration" << endl;
            return -1;
        }
    }

    //setting the projection object
    rs::utils::unique_ptr<rs::core::projection_interface> projection;
    if(device->is_stream_enabled(rs::stream::color) && device->is_stream_enabled(rs::stream::depth))
    {
        intrinsics color_intrin = convert_intrinsics(device->get_stream_intrinsics(rs::stream::color));
        intrinsics depth_intrin = convert_intrinsics(device->get_stream_intrinsics(rs::stream::depth));
        extrinsics extrin = convert_extrinsics(device->get_extrinsics(rs::stream::depth, rs::stream::color));
        projection.reset(rs::core::projection_interface::create_instance(&color_intrin, &depth_intrin, &extrin));
        actual_config.projection = projection.get();
    }

    // setting the enabled module configuration
    if(module->set_module_config(actual_config) < status_no_error)
    {
        cerr<<"error : failed to set the enabled module configuration" << endl;
        return -1;
    }

    device->start();

    for(int i = 0; i < frames_count; i++)
    {
        device->wait_for_frames();

        //construct the sample set
        correlated_sample_set sample_set = {};
        for(auto &stream : actual_streams)
        {
            rs::stream librealsense_stream = convert_stream_type(stream);
            image_info info = {device->get_stream_width(librealsense_stream),
                               device->get_stream_height(librealsense_stream),
                               rs::utils::convert_pixel_format(device->get_stream_format(librealsense_stream)),
                               device->get_stream_width(librealsense_stream)
                              };

            //the image is created with ref count 1 and its not released in this scope, no need to add_ref.
            sample_set[stream] = image_interface::create_instance_from_raw_data(&info,
                                                                                {device->get_frame_data(librealsense_stream), nullptr},
                                                                                stream,
                                                                                image_interface::flag::any,
                                                                                device->get_frame_timestamp(librealsense_stream),
                                                                                device->get_frame_number(librealsense_stream),
                                                                                nullptr);
        }

        //send synced sample set to the module
        if(module->process_sample_set_sync(&sample_set) < status_no_error)
        {
            cerr<<"error : failed to process sample" << endl;
        }
        else
        {
            //query the module for output on this samples
            auto output_data = module->get_max_depth_value_data();
            cout<<"got module max depth value : "<< output_data.max_depth_value << ", for frame number : " << output_data.frame_number << endl;
        }
    }

    if(module->query_video_module_control())
    {
        module->query_video_module_control()->reset();
    }
    device->stop();
    return 0;
}
