// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

//Pipeline Async sample
//This sample demonstrates an application usage of an asynchronized pipeline. The pipeline simplifies the user interaction with
//computer vision modules. It abstracts the camera configuration and streaming, the video modules triggering and threading, and
//lets the application focus on the computer vision output of the modules. The pipeline can manage computer vision modules, which
//implement the video module interface. The pipeline is the consumer of the video module interface, while the application
//consumes the module specific interface, which completes the video module interface. The async pipeline provides the user main loop,
//which runs on the calling thread, and computer vision modules callbacks, which are triggered on different threads. In this sample
//an example computer vision module, the max depth value module is used to demonstrate the pipeline usage.

#include <iostream>
#include <thread>
#include <memory>
#include <librealsense/rs.hpp>

#include "rs_sdk.h"
#include "rs/cv_modules/max_depth_value_module/max_depth_value_module.h"

using namespace std;
using namespace rs::core;
using namespace rs::utils;
using namespace rs::cv_modules;

class my_pipeline_callback_handler : public pipeline_async_interface::callback_handler
{
public:
    void on_new_sample_set(const correlated_sample_set &sample_set) override
    {
        //get a unique managed ownership of the image by calling add_ref and wrapping the it with a unique_ptr
        //with a custom deleter which calls release.
        rs::utils::unique_ptr<image_interface> depth_image = sample_set.get_unique(stream_type::depth);

        if(!depth_image)
        {
            cerr<<"ERROR : got empty depth image"<<endl;
            return;
        }

        cout<<"got depth image, frame number : " << depth_image->query_frame_number() <<endl;

        //do something with the depth image...
    }

    void on_cv_module_process_complete(video_module_interface * cv_module) override
    {
        auto max_depth_module = dynamic_cast<rs::cv_modules::max_depth_value_module*>(cv_module);
        auto max_depth_data = max_depth_module->get_max_depth_value_data();

        cout<<"max depth value : "<< max_depth_data.max_depth_value << ", frame number :"<< max_depth_data.frame_number <<endl;

        //do something with the module output...
    }

    void on_error(status status) override
    {
        cerr<<"ERROR : got pipeline error status : "<< status <<endl;
    }

    virtual ~my_pipeline_callback_handler() {}
};

int main () try
{
    //create the cv module, implementing both the video_module_interface and a specific cv module interface.
    //the module must outlive the pipeline
    std::unique_ptr<max_depth_value_module> module(new max_depth_value_module());

    //create an async pipeline
    std::unique_ptr<pipeline_async_interface> pipeline(new pipeline_async());

    //add the module to the pipeline
    if(pipeline->add_cv_module(module.get()) < status_no_error)
    {
        throw std::runtime_error("failed to add cv module to the pipeline");
    }

    //create a user defined callback handler
    std::unique_ptr<my_pipeline_callback_handler> pipeline_callbacks_handler(new my_pipeline_callback_handler());

    //optionally, get a configuration from the module and set it explicitly.
    for(auto config_index = 0;; config_index++)
    {
        video_module_interface::supported_module_config supported_config = {};
        if(module->query_supported_module_config(config_index, supported_config) < status_no_error)
        {
            throw std::runtime_error("can't find a valid module configuration");
        }

        if(pipeline->set_config(supported_config) == status_no_error)
        {
            break;
        }
    }

    //start the pipeline streaming
    if(pipeline->start(pipeline_callbacks_handler.get()) < status_no_error)
    {
        throw std::runtime_error("failed to start pipeline");
    }

    //sleep to let the cv module get some samples
    std::this_thread::sleep_for(std::chrono::seconds(5));

    //stop the pipeline streaming
    if(pipeline->stop() < status_no_error)
    {
        throw std::runtime_error("failed to stop pipeline");
    }

    return EXIT_SUCCESS;
}
catch (std::exception &e)
{
    cerr<<e.what()<<endl;
    return EXIT_FAILURE;
}
