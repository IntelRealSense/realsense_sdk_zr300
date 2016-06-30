// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <memory>
#include <vector>
#include <iostream>
#include <functional>
#include <thread>
#include <unistd.h>
#include <librealsense/rs.hpp>

#include "rs_sdk.h"
#include "video_module_mock.h"

using namespace std;
using namespace rs::core;
using namespace rs::utils;
using namespace rs::mock;

int main (int argc, char* argv[])
{
    // initialize the device from live device or playback file, based on command line parameters.
    std::unique_ptr<context> ctx;

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
    bool is_complete_sample_set_required = false;
    std::unique_ptr<video_module_interface> module(new video_module_mock(is_complete_sample_set_required));

    // get the first supported module configuration
    video_module_interface::supported_module_config supported_config = {};
    if (module->query_supported_module_config(0, supported_config) < status_no_error)
    {
        cerr<<"error : failed to query the first supported module configuration" << endl;
        return -1;
    }

    if(supported_config.complete_sample_set_required)
    {
        cerr<<"error : (complete_sample_set_required == true) is not inmplemented" << endl;
        return -1;
    }

    auto device_name = device->get_name();
    auto is_current_device_valid = (std::strcmp(device_name, supported_config.device_name) == 0);
    if (!is_current_device_valid)
    {
        cerr<<"error : current device is not supported by the current supported module configuration" << endl;
        return -1;
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
            correlated_sample_set sample_set;
            smart_ptr<metadata_interface> metadata(new rs::core::metadata());
            smart_ptr<image_interface> image(new lrs_image(frame, image_interface::flag::any, metadata));
            sample_set[stream] = image;

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

            actual_config[motion].flags = sample_flags::none;
            actual_config[motion].frame_rate = 0; // not implemented by librealsense
            actual_config[motion].is_enabled = true;
            actual_motions.push_back(motion);
        }

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

    //setting the projection object
    std::unique_ptr<rs::core::projection> projection;
    if(device->is_stream_enabled(rs::stream::color) && device->is_stream_enabled(rs::stream::depth))
    {
        rs_intrinsics color_intrin = device->get_stream_intrinsics(rs::stream::color);
        rs_intrinsics depth_intrin = device->get_stream_intrinsics(rs::stream::depth);
        rs_extrinsics extrinsics = device->get_extrinsics(rs::stream::depth, rs::stream::color);
        projection.reset(rs::core::projection::create_instance(&color_intrin, &depth_intrin, &extrinsics));
        module->set_projection(projection.get());
    }

    // setting the enabled module configuration
    if(module->set_module_config(actual_config) < status_no_error)
    {
        cerr<<"error : failed to set the enabled module configuration" << endl;
        return -1;
    }

    device->start(active_sources);

    //
    std::this_thread::sleep_for(std::chrono::seconds(3));

    if(module->query_video_module_control())
    {
        module->query_video_module_control()->reset();
    }

    device->stop(active_sources);
}
