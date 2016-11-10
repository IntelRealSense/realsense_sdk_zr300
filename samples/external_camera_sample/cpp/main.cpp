// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <iostream>
#include <rs/core/context.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <future>
#include <viewer.h>
#include <rs_streamer.h>
#include <rs/utils/self_releasing_array_data_releaser.h>
#include "rs/core/image_interface.h"
#include "rs/utils/librealsense_conversion_utils.h"
#include "v4l2_streamer.h"

using namespace rs::core;


/*
 *
 * external_camera_sample
 *
 * This sample shows how to use video4linux along with the RealSense(TM) SDK
 * The sample uses v4l2 lib to capture color frames from an external camera (webcam), the color images provided via v4l2 are transformed into a lrs_image.
 * Using
 *
 *
 * TODO: Complete
 */


image_info v4l2format_to_image_info(v4l2_format format)
{
    image_info info;
    info.width = format.fmt.pix.width;
    info.height = format.fmt.pix.height;
    rs::format pix_fmt = convert_to_rs_format(format.fmt.pix.pixelformat);
    info.format =  rs::utils::convert_pixel_format(pix_fmt);
    info.pitch = format.fmt.pix.width * num_bpp(info.format);
    return info;
}

int main()
{
    constexpr unsigned int time_to_run_in_seconds = 5;

    rs_streamer depth_streamer;
    if(depth_streamer.init() == false)
    {
        std::cerr << "Failed to initialize rs_streamer (make sure device is connected)" << std::endl;
        return -1;
    }


    std::queue<std::shared_ptr<image_interface>> depth_images;
    std::queue<std::shared_ptr<image_interface>> color_images;

    auto depth_frames_callback = [&depth_images](rs::frame f)
    {
        static auto p_depth_viewer = std::make_shared<rs::utils::viewer>(1, 628, 468, nullptr, "Depth Viewer");

        std::cout << "new depth frame" << std::endl;
        image_interface* depth_image = image_interface::create_instance_from_librealsense_frame(f, image_interface::flag::any);
        std::shared_ptr<image_interface> p_depth_image = rs::utils::get_shared_ptr_with_releaser(depth_image);
        depth_images.push(p_depth_image);
        depth_image->add_ref(); //since we're passing it to show_image (which will call release)
        p_depth_viewer->show_image(depth_image);
    };


    v4l_streamer external_camera;

    auto callback = [&color_images, &depth_images](void* buffer, v4l2_buffer buffer_info, v4l2_format v4l2format)
    {
        static auto p_color_viewer = std::make_shared<rs::utils::viewer>(1, 640, 480, nullptr, "Color Viewer");
        static int num_frames = 0;

        image_info image_info = v4l2format_to_image_info(v4l2format);
        uint32_t buffer_size = image_info.pitch * image_info.height;
        uint8_t* copied_buffer = new uint8_t[buffer_size];
        memcpy(copied_buffer, buffer, buffer_size);
        rs::utils::self_releasing_array_data_releaser* data_releaser = new rs::utils::self_releasing_array_data_releaser(copied_buffer);
        image_interface::image_data_with_data_releaser data_container(copied_buffer, data_releaser);
        double time_stamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        uint64_t frame_number = num_frames++;
        timestamp_domain time_stamp_domain = timestamp_domain::camera;

        image_interface* color_image = image_interface::create_instance_from_raw_data(&image_info,
                                                                                data_container,
                                                                                stream_type::color,
                                                                                image_interface::flag::any,
                                                                                time_stamp,
                                                                                frame_number,
                                                                                time_stamp_domain);

        std::shared_ptr<image_interface> p_color_image = rs::utils::get_shared_ptr_with_releaser(color_image);
        color_images.push(p_color_image);
        color_image->add_ref(); //since we're passing it to show_image (which will call release)
        p_color_viewer->show_image(color_image);
        if(!depth_images.empty() && !color_images.empty())
        {
            depth_images.pop();
            color_images.pop();
        }
    };

    try
    {
        depth_streamer.start(rs::stream::depth, rs::format::z16, 628u, 468u, 30u, depth_frames_callback);
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
    external_camera.stop_streaming();
    int size = depth_images.size();
    for (int i=0; i< size; i++)
    {
        depth_images.pop();
    }
    depth_streamer.stop();

    return 0;

}
