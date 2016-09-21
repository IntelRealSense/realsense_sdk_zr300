// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

// Video Module Asynchronized Sample
// This sample demonstrates an application usage of a computer vision module, which implements asynchronous samples processing.
// The video module implements the video module interface, which is a common way for the application or SDK to interact with
// the module. It also implements a module specific interface, in this example - the module calculates the maximal depth value
// in the image.

#include <memory>
#include <vector>
#include <iostream>
#include <functional>
#include <thread>
#include <unistd.h>
#include <librealsense/rs.hpp>
#include <map>

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

    if (argc > 1)
    {
        if(access(argv[1], F_OK) == -1)
        {
            cerr<<"error : playback file does not exists" << endl;
            return -1;
        }
        //use the playback file context
        ctx.reset(new rs::playback::context(argv[1]));
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
    }

    rs::device * device = ctx->get_device(0); //device memory managed by the context

    // initialize the module
    auto milliseconds_added_to_simulate_larger_computation_time = 100;
    std::unique_ptr<max_depth_value_module> module(new max_depth_value_module(milliseconds_added_to_simulate_larger_computation_time));

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

        if(!(supported_config.config_flags & video_module_interface::supported_module_config::flags::async_processing_supported))
        {
            //skip config due to unsupported processing mode
            continue;
        }
        break;
    }

    //construct an actual model configuration to be set on the module
    video_module_interface::actual_module_config actual_config = {};
    std::memcpy(actual_config.device_info.name, supported_config.device_name, std::strlen(supported_config.device_name));
    rs::source active_sources = static_cast<rs::source>(0);

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
        auto supported_stream_config = supported_config[stream];

        if (!supported_stream_config.is_enabled)
            continue;

        rs::stream librealsense_stream = convert_stream_type(stream);

        bool is_matching_stream_mode_found = false;
        auto stream_mode_count = device->get_stream_mode_count(librealsense_stream);
        for(int i = 0; i < stream_mode_count; i++)
        {
            int width, height, frame_rate;
            rs::format format;
            device->get_stream_mode(librealsense_stream, i, width, height, format, frame_rate);
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

                active_sources = rs::source::video;
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

    //define callbacks to the actual streams and set them, the callback lifetime assumes the module is available.
    std::map<stream_type, std::function<void(rs::frame)>> stream_callback_per_stream;
    for(auto stream : actual_streams)
    {
        stream_callback_per_stream[stream] = [stream, &module](rs::frame frame)
        {
            correlated_sample_set sample_set = {};
            //the image is created with ref count 1 and is not released in this scope, no need to add_ref.
            sample_set[stream] = image_interface::create_instance_from_librealsense_frame(frame, image_interface::flag::any, nullptr);

            //send asynced sample set to the module
            if(module->process_sample_set_async(&sample_set) < status_no_error)
            {
                cerr<<"error : failed to process sample" << endl;
            }
        };

        device->set_frame_callback(convert_stream_type(stream), stream_callback_per_stream[stream]);
    }

    //define callback to the motion events and set it, the callback lifetime assumes the module is available.
    std::function<void(rs::motion_data)> motion_callback;
    std::function<void(rs::timestamp_data)> timestamp_callback;
    if (device->supports(rs::capabilities::motion_events))
    {
        vector<motion_type> actual_motions;
        vector<motion_type> possible_motions =  { motion_type::accel,
                                                  motion_type::gyro
                                                };

        for(auto motion: possible_motions)
        {
            auto supported_motion_config = supported_config[motion];

            if (!supported_motion_config.is_enabled)
                continue;

            video_module_interface::actual_motion_sensor_config &actual_motion_config = actual_config[motion];
            actual_motion_config.flags = sample_flags::none;
            actual_motion_config.frame_rate = 0; // not implemented by librealsense
            actual_motion_config.intrinsics = convert_motion_intrinsics(device->get_motion_intrinsics());
            actual_motion_config.extrinsics = convert_extrinsics(device->get_motion_extrinsics_from(rs::stream::depth));
            actual_motion_config.is_enabled = true;
            actual_motions.push_back(motion);
        }

        if(actual_motions.size()< 0)
        {
            //enable motion from the selected module configuration
            motion_callback = [actual_motions, &module](rs::motion_data entry)
            {
                correlated_sample_set sample_set = {};
                for(auto actual_motion : actual_motions)
                {
                    sample_set[actual_motion].timestamp = entry.timestamp_data.timestamp;
                    sample_set[actual_motion].type = actual_motion;
                    sample_set[actual_motion].frame_number = entry.timestamp_data.frame_number;
                    sample_set[actual_motion].data[0] = entry.axes[0];
                    sample_set[actual_motion].data[1] = entry.axes[1];
                    sample_set[actual_motion].data[2] = entry.axes[2];
                }

                //send asynced sample set to the module
                if(module->process_sample_set_async(&sample_set) < status_no_error)
                {
                    cerr<<"error : failed to process sample" << endl;
                }
            };

            timestamp_callback = [](rs::timestamp_data entry) { /* no operation */ };

            device->enable_motion_tracking(motion_callback, timestamp_callback);

            if(active_sources == rs::source::video)
            {
                active_sources = rs::source::all_sources;
            }
            else // none sources
            {
                active_sources = rs::source::motion_data;
            }
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

    device->start(active_sources);

    auto start = std::chrono::system_clock::now();
    while(std::chrono::system_clock::now() - start < std::chrono::seconds(3))
    {
        auto output_data = module->get_max_depth_value_data();
        cout<<"got module max depth value : "<< output_data.max_depth_value << ", for frame number : " << output_data.frame_number << endl;
    }

    if(module->query_video_module_control())
    {
        module->query_video_module_control()->reset();
    }

    device->stop(active_sources);
    return 0;
}
