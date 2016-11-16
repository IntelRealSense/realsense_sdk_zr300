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
#include "rs_streamer.h"
#include <rs/utils/self_releasing_array_data_releaser.h>
#include "rs/core/image_interface.h"
#include "rs/utils/librealsense_conversion_utils.h"
#include "v4l2_streamer.h"
#include "rs/utils/samples_time_sync_interface.h"

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
rs::utils::unique_ptr<rs::utils::samples_time_sync_interface> create_depth_and_external_color_sync()
{
    int streams_fps[static_cast<int>(stream_type::max)] = {0};
    streams_fps[static_cast<int>(rs::core::stream_type::color)] = 30;
    streams_fps[static_cast<int>(rs::core::stream_type::depth)] = 30;

    int motions_fps[static_cast<int>(motion_type::max)] = {0};

    return rs::utils::get_unique_ptr_with_releaser(
        rs::utils::samples_time_sync_interface::create_instance(
            streams_fps, motions_fps, "external device syc"));
}

void process_frame(rs::core::correlated_sample_set &sample_set)
{
    static int num_frames = 0;
    num_frames++;
    std::cout << num_frames << ": Found correlated sample set" << std::endl;
    std::cout << "\t Color = " << sample_set[stream_type::color]->query_frame_number() << std::endl;
    std::cout << "\t Depth = " << sample_set[stream_type::depth]->query_frame_number() << std::endl;
    
    std::cout << sample_set[stream_type::color]->release() << std::endl;
    std::cout << sample_set[stream_type::depth]->release() << std::endl;
}

int main()
{
    constexpr unsigned int time_to_run_in_seconds = 100;
    
    rs_streamer depth_streamer;
    if(depth_streamer.init() == false)
    {
        std::cerr << "Failed to initialize rs_streamer (make sure device is connected)" << std::endl;
        return -1;
    }

    rs::utils::unique_ptr<rs::utils::samples_time_sync_interface> external_color_rs_depth_sync = create_depth_and_external_color_sync();

    auto depth_frames_callback = [&external_color_rs_depth_sync](rs::frame f)
    {

        static int num_frames = 0;
        num_frames++;
        std::cout << "depth callback #" << num_frames << std::endl;
        std::shared_ptr<image_interface> depth_image = rs::utils::get_shared_ptr_with_releaser(
            image_interface::create_instance_from_librealsense_frame(f, image_interface::flag::any));
        std::cout << "depth image ref count on creation = " << depth_image->ref_count() << std::endl;
        rs::core::correlated_sample_set sample_set = {};
        if(external_color_rs_depth_sync->insert(depth_image.get(), sample_set))
        {
            process_frame(sample_set);
        }

        //p_depth_viewer->show_image(depth_image);
    };


    v4l_streamer external_camera;

    auto callback = [&external_color_rs_depth_sync](void* buffer, v4l2_buffer buffer_info, v4l2_format v4l2format)
    {
        static rs::utils::viewer color_viewer(1, 640, 480, nullptr, "Color Viewer");
        static int num_frames = 0;
        num_frames++;
        std::cout << "color callback #" << num_frames << std::endl;
        image_info image_info = v4l2format_to_image_info(v4l2format);
        uint32_t buffer_size = image_info.pitch * image_info.height;
        uint8_t* copied_buffer = new uint8_t[buffer_size];
        memcpy(copied_buffer, buffer, buffer_size);
        rs::utils::self_releasing_array_data_releaser* data_releaser = new rs::utils::self_releasing_array_data_releaser(copied_buffer);
        image_interface::image_data_with_data_releaser data_container(copied_buffer, data_releaser);
        double time_stamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        uint64_t frame_number = num_frames;
        timestamp_domain time_stamp_domain = timestamp_domain::camera;
        std::shared_ptr<image_interface> color_image = rs::utils::get_shared_ptr_with_releaser(image_interface::create_instance_from_raw_data(&image_info, data_container,
                            stream_type::color, image_interface::flag::any, time_stamp, frame_number,time_stamp_domain));

        rs::core::correlated_sample_set sample_set = {};
        if(external_color_rs_depth_sync->insert(color_image.get(), sample_set))
        {
            process_frame(sample_set);
        }
        std::cout << "before show image color " << std::endl;
        color_viewer.show_image(color_image);
        std::cout << "after show image color " << std::endl;
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
    depth_streamer.stop();
    external_color_rs_depth_sync->flush();
    
    return 0;

}
