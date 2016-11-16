// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/*
 * external_camera_sample
 *
 * This sample shows how to use video4linux along with the RealSense(TM) SDK and librealsense
 * The sample uses v4l2 lib to capture color frames from an external camera (webcam).
 * Color frames provided via v4l2 and depth frames provided via librealsense are grouped together into a correlated sample set
 * using a samples_time_sync_interface for external devices.
 *
 * The correlated sample set can be passed to middlewares to be processed together, however this sample does not use
 * any specific MW and instead, simply calls process_samples(...) to demonstrate the flow up to this point
 *
 */

#include <iostream>
#include <rs/core/context.h>
#include <linux/videodev2.h>
#include <future>
#include <viewer.h>
#include "rs_streamer.h"
#include <rs/utils/self_releasing_array_data_releaser.h>
#include "rs/utils/librealsense_conversion_utils.h"
#include "v4l2_streamer.h"
#include "rs/utils/samples_time_sync_interface.h"

using namespace rs::core;



image_info v4l2format_to_image_info(v4l2_format format)
{
    image_info info;
    info.width = format.fmt.pix.width;
    info.height = format.fmt.pix.height;
    rs::format pix_fmt = convert_to_rs_format(format.fmt.pix.pixelformat);
    info.format =  rs::utils::convert_pixel_format(pix_fmt);
    info.pitch = format.fmt.pix.bytesperline; //format.fmt.pix.width * num_bpp(info.format)
    return info;
}


std::shared_ptr<rs::utils::samples_time_sync_interface> create_depth_and_external_color_sync()
{
    //No need for motions to be synchronized
    int motions_fps[static_cast<int>(motion_type::max)] = {0};
    
    //Adding color and depth with 30 fps each as required for sycnrhonization
    int streams_fps[static_cast<int>(stream_type::max)] = {0};
    streams_fps[static_cast<int>(rs::core::stream_type::color)] = 30;
    streams_fps[static_cast<int>(rs::core::stream_type::depth)] = 30;

    //creating an external device sync utility
    // Wrap it with a shared_ptr to automatically call release() on the instance upon destruction
    return rs::utils::get_shared_ptr_with_releaser(
        rs::utils::samples_time_sync_interface::create_instance(
            streams_fps, motions_fps, rs::utils::samples_time_sync_interface::external_device_name));
}

void process_samples(rs::core::correlated_sample_set& sample_set)
{
    static int num_frames = 0;
    num_frames++;
    std::cout << "Processing sample set" << std::endl;
    std::cout << "\t Color = " << sample_set[stream_type::color]->query_frame_number() << std::endl;
    std::cout << "\t Depth = " << sample_set[stream_type::depth]->query_frame_number() << std::endl;
}

void release_sample_set(rs::core::correlated_sample_set& sample_set)
{
    sample_set[stream_type::color]->release();
    sample_set[stream_type::depth]->release();
}

int main()
{
    //Number of seconds to stream images from the cameras
    constexpr unsigned int time_to_run_in_seconds = 10;

    //Create a realsense wrapper for streaming depth images
    // This is an over-simplified wrapper that provides asynchronous streaming using callbacks
    rs_streamer depth_streamer;
    
    //Initializing the realsense streamer before streaming
    if(depth_streamer.init() == false)
    {
        std::cerr << "Failed to initialize rs_streamer (make sure device is connected)" << std::endl;
        return -1;
    }

    //Calling helper function to create a sync utility for synchronizing external device images with other images
    //This sample requires that color images from an external device will be synchronized with depth images from librealsense
    std::shared_ptr<rs::utils::samples_time_sync_interface> external_color_rs_depth_sync = create_depth_and_external_color_sync();

    //Creating a callback object that will be invoked whever a depth frame is available
    // The callback captures the sync utility (by value, since it is a shared_ptr) so that it can insert new images to it
    auto depth_frames_callback = [external_color_rs_depth_sync](rs::frame f)
    {
        static rs::utils::viewer depth_viewer(1, 628, 468, nullptr, "Depth Viewer");

        //Converting the received frame to an image using create_instance_from_librealsense_frame
        // Wrap the image with a shared_ptr for automatic release
        std::shared_ptr<image_interface> depth_image = rs::utils::get_shared_ptr_with_releaser(
            image_interface::create_instance_from_librealsense_frame(f, image_interface::flag::any));

        //Initializing an empty sample set
        rs::core::correlated_sample_set sample_set = {};
        
        //Passing the new depth image to the sync utility, if a matching color image exists
        // then sample_set will include both depth and color images
        if(external_color_rs_depth_sync->insert(depth_image.get(), sample_set))
        {
            //Found a correlation between depth and color images
            
            //At this point the sample set can be passed to middlewares to process the samples
            process_samples(sample_set);
            
            //After all processing is completed, the owner of the samples set, i.e this callback, is responsible
            // for releasing the images held by the sample set
            release_sample_set(sample_set);
        }

        depth_viewer.show_image(depth_image);
    };


    v4l_streamer external_camera;

    auto callback = [&external_color_rs_depth_sync](void* buffer, v4l2_buffer buffer_info, v4l2_format v4l2format)
    {
        static rs::utils::viewer color_viewer(1, 640, 480, nullptr, "Color Viewer");
        static int num_frames = 0;

        //Converting v4l_format to image_info
        image_info image_info = v4l2format_to_image_info(v4l2format);
        
        uint8_t* copied_buffer = new uint8_t[buffer_info.length];
        memcpy(copied_buffer, buffer, buffer_info.length);
        
        rs::utils::self_releasing_array_data_releaser* data_releaser = new rs::utils::self_releasing_array_data_releaser(copied_buffer);
        image_interface::image_data_with_data_releaser data_container(copied_buffer, data_releaser);
        double time_stamp = buffer_info.timestamp.tv_usec;
        uint64_t frame_number = num_frames++;
        timestamp_domain time_stamp_domain = timestamp_domain::camera;
        std::shared_ptr<image_interface> color_image = rs::utils::get_shared_ptr_with_releaser(image_interface::create_instance_from_raw_data(&image_info, data_container,
                            stream_type::color, image_interface::flag::any, time_stamp, frame_number,time_stamp_domain));

        rs::core::correlated_sample_set sample_set = {};
        if(external_color_rs_depth_sync->insert(color_image.get(), sample_set))
        {
            process_samples(sample_set);
            release_sample_set(sample_set);
        }
        
        color_viewer.show_image(color_image);
    };

    try
    {
        depth_streamer.start_streaming(rs::stream::depth, rs::format::z16, 628u, 468u, 30u, depth_frames_callback);
        external_camera.start_streaming(callback);
    }
    catch(const rs::error& e)
    {
        std::cerr << e.what() << std::endl;
        return -1;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return -1;
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(time_to_run_in_seconds));
    
    external_color_rs_depth_sync->flush();
    external_camera.stop_streaming();
    depth_streamer.stop_streaming();
    
    return 0;

}

