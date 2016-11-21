// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/**
 * @brief external_camera_sample
 *
 * This sample shows how to use video4linux along with the RealSense(TM) SDK and librealsense
 * The sample uses v4l2 lib to capture color frames from an external camera (webcam).
 * Color frames provided via v4l2 and depth frames provided via librealsense are grouped together into a correlated sample set
 * using a samples_time_sync_interface for external devices.
 *
 * The correlated sample set can be passed to computer vision (cv) modules to be processed together, however this sample does not use
 * any specific cv module and instead, simply calls process_samples(...) to demonstrate the flow up to this point
 */

#include <iostream>
#include <rs/core/context.h>
#include <linux/videodev2.h>
#include <future>
#include <rs/utils/self_releasing_array_data_releaser.h>
#include <rs/utils/librealsense_conversion_utils.h>
#include <rs/utils/samples_time_sync_interface.h>
#include <viewer.h>
#include "v4l2_streamer.h"
#include "rs_streamer.h"


using namespace rs::core;

rs::format convert_to_rs_format(uint32_t v4l_format)
{
    switch (v4l_format)
    {
        case V4L2_PIX_FMT_Z16                 : return rs::format::z16;
        case V4L2_PIX_FMT_YUYV                : return rs::format::yuyv;
        case V4L2_PIX_FMT_RGB24               : return rs::format::rgb8;
        case V4L2_PIX_FMT_BGR24               : return rs::format::bgr8;
        case V4L2_PIX_FMT_ARGB32              : return rs::format::rgba8;
        case V4L2_PIX_FMT_ABGR32              : return rs::format::bgra8;
        case V4L2_PIX_FMT_Y16                 : return rs::format::y16;
        case V4L2_PIX_FMT_Y10                 : return rs::format::raw10;
        default : return (rs::format) -1;
    }
}


class lambda_releaser : public rs::core::release_interface
{
public:
    lambda_releaser(std::function<void()> func) : m_lambda(func){}
    int release() const override
    {
        m_lambda();
        delete(this);
    }
protected:
    ~lambda_releaser () {}
    
private:
    std::function<void()> m_lambda;
};

int main()
{
    //Number of seconds to stream images from the cameras
    constexpr unsigned int time_to_run_in_seconds = 5;
    
    //Create a realsense wrapper for streaming depth images
    // This is an over-simplified wrapper that provides asynchronous streaming using callbacks
    rs_streamer depth_streamer;
    
    //Initializing the realsense streamer before streaming
    if (depth_streamer.init() == false) {
        std::cerr << "Failed to initialize rs_streamer (make sure device is connected)" << std::endl;
        return -1;
    }
    
    //Creating a samples_time_sync_interface for synchronizing external device images with other images
    // This sample requires that color images from an external device will be synchronized with depth images from librealsense
    
    //There is no need for motions to be synchronized in this sample
    int motions_fps[static_cast<int>(motion_type::max)] = {0};
    
    //Adding color and depth with 30 fps each as required for synchronization
    int streams_fps[static_cast<int>(stream_type::max)] = {0};
    streams_fps[static_cast<int>(rs::core::stream_type::color)] = 30;
    streams_fps[static_cast<int>(rs::core::stream_type::depth)] = 30;
    
    //creating an external device sync utility
    // Wrapping it with a shared_ptr to automatically call release() on the instance upon destruction
    std::shared_ptr<rs::utils::samples_time_sync_interface>
        external_color_rs_depth_sync = rs::utils::get_shared_ptr_with_releaser(
        rs::utils::samples_time_sync_interface::create_instance(
            streams_fps, motions_fps, rs::utils::samples_time_sync_interface::external_device_name));
    
    // The callback captures the sync utility (by value, since it is a shared_ptr) so that it can insert new images in to it
    auto sync_and_process_sample = [external_color_rs_depth_sync](std::shared_ptr<image_interface> image)
    {
        //Passing the new image to the sync utility, if a matching image exists
        // then sample_set will include both depth and color images
        
        //Initializing an empty sample set
        rs::core::correlated_sample_set sample_set = {};
        if (external_color_rs_depth_sync->insert(image.get(), sample_set)) {
            //Found a correlation between depth and color images
            //At this point the sample set can be passed to middlewares to process the samples
            
            std::cout << "Processing sample set" << std::endl;
            std::cout << "\t Color = " << sample_set[stream_type::color]->query_frame_number() << std::endl;
            std::cout << "\t Depth = " << sample_set[stream_type::depth]->query_frame_number() << std::endl;
            
            //After all processing is completed, the owner of the samples set, i.e this callback, is responsible
            // for releasing the images held by the sample set
            sample_set[stream_type::color]->release();
            sample_set[stream_type::depth]->release();
        }
    };
    
    std::mutex depth_callback_lock;
    bool is_still_accepting_callbacks = true;
    //Creating a callback object that will be invoked whenever a depth frame is available
    auto depth_frames_callback =
        [&sync_and_process_sample, &depth_callback_lock, &is_still_accepting_callbacks](rs::frame f)
        {
            //Converting the received frame to an image using create_instance_from_librealsense_frame
            // Wrap the image with a shared_ptr for automatic release
            std::shared_ptr<image_interface> depth_image = rs::utils::get_shared_ptr_with_releaser(
                image_interface::create_instance_from_librealsense_frame(f, image_interface::flag::any));
            
            std::lock_guard<std::mutex> lock(depth_callback_lock);
            if (is_still_accepting_callbacks) {
                sync_and_process_sample(depth_image);
            }
        };
    
    v4l_streamer external_camera;
    if (external_camera.init() == false) {
        std::cerr << "Failed to initialize v4l_streamer" << std::endl;
        return -1;
    }
    
    auto callback = [external_color_rs_depth_sync, &sync_and_process_sample](void *buffer,
                                                                             v4l2_buffer buffer_info,
                                                                             v4l2_format v4l2format,
                                                                             std::function<void()> buffer_releaser)
    {
        static rs::utils::viewer viewer(1, 640,480, nullptr, "Color");
        //Converting v4l_format to image_info
        image_info image_info = {
            .width = static_cast<int32_t>(v4l2format.fmt.pix.width),
            .height = static_cast<int32_t>(v4l2format.fmt.pix.height),
            .format = rs::utils::convert_pixel_format(convert_to_rs_format(v4l2format.fmt.pix.pixelformat)),
            .pitch = static_cast<int32_t>(v4l2format.fmt.pix.bytesperline)
        };
        
        //Create an image from the raw buffer received from video4linux, provide a custom releaser of the buffer
        // to allow the image to manage the memory by itself
        uint8_t *copied_buffer = new uint8_t[buffer_info.length];
        memcpy(copied_buffer, buffer, buffer_info.length);
    
        rs::core::release_interface* v4l_buffer_releaser = new lambda_releaser([buffer_releaser](){ buffer_releaser();});
//        rs::utils::self_releasing_array_data_releaser
//            *data_releaser = new rs::utils::self_releasing_array_data_releaser(copied_buffer);
        image_interface::image_data_with_data_releaser data_container(copied_buffer, v4l_buffer_releaser);
        
        //Add a timestamp, frame number and the timestamp domain
        double time_stamp = buffer_info.timestamp.tv_usec;
        uint64_t frame_number = buffer_info.sequence;
        timestamp_domain time_stamp_domain = timestamp_domain::camera;
        
        //Create an image from the raw buffer and its information
        std::shared_ptr<image_interface> color_image = rs::utils::get_shared_ptr_with_releaser(
            image_interface::create_instance_from_raw_data(
                &image_info,
                data_container,
                stream_type::color,
                image_interface::flag::any,
                time_stamp,
                frame_number,
                time_stamp_domain)
        );
        
        //Pass the image to be synchronized and processed
        sync_and_process_sample(color_image);
        viewer.show_image(color_image);
    };
    
    //Start capturing images from both devices
    
    try {
        depth_streamer.start_streaming(depth_frames_callback);
    }
    catch (const rs::error &e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
    
    try {
        external_camera.start_streaming(callback);
    }
    catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
    
    //Since this sample uses async streaming with callbacks, put the main thread to sleep for time_to_run_in_seconds seconds
    // and allow callbacks to be invoked from both cameras
    std::this_thread::sleep_for(std::chrono::seconds(time_to_run_in_seconds));
    
    
    //Stop the external camera
    external_camera.stop_streaming();
    
    {
        //librealsense requires that all buffers are freed before calling stop() on the device so this samples needs to
        // stop holding images before stopping the depth streamer
        std::lock_guard<std::mutex> lock(depth_callback_lock);
        is_still_accepting_callbacks = false;
    }
    
    //Remove any leftover images in the sync utility
    external_color_rs_depth_sync->flush();
    
    //Note that if is_still_accepting_callbacks would not have been set before flush(), a callback from the depth streamer could have arrived,
    // between flush() and stop_streaming(), which would have caused stop_streaming() to enter a deadlock
    depth_streamer.stop_streaming();
    
    return 0;

}

