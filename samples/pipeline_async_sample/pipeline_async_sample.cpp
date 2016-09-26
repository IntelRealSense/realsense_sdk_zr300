// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

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

class pipeline_handler : public pipeline_async_interface::callback_handler
{
public:
    pipeline_handler(const std::shared_ptr<max_depth_value_module>& max_depth_module) :
        m_max_depth_module(max_depth_module) {}

    void on_new_sample_set(correlated_sample_set * sample_set) override
    {
        if(!sample_set)
        {
            cerr<<"ERROR : got empty samples set"<<endl;
            return;
        }

        //its important to set the sample_set in a scoped unique ptr with a releaser since all the ref counted samples in the
        //samples set need to be released out of this scope even if this module is not using them, otherwise there will
        //be a memory leak.
        auto scoped_sample_set = get_unique_ptr_with_releaser(sample_set);

        auto depth_image = scoped_sample_set->take_shared(stream_type::depth);

        if(!depth_image)
        {
            cerr<<"ERROR : got empty depth image"<<endl;
            return;
        }

        cout<<"got depth image, frame number : " << depth_image->query_frame_number() <<endl;

        //do something with the depth image...
    }

    void on_cv_module_process_complete(int32_t unique_module_id) override
    {
        if(m_max_depth_module->query_module_uid() == unique_module_id)
        {
            auto max_depth_data = m_max_depth_module->get_max_depth_value_data();

            cout<<"frame number : "<<max_depth_data.frame_number<<" have max depth value : "<< max_depth_data.max_depth_value<<endl;

            //do something with the max depth value...
        }

        // check unique module id for other cv modules...
    }

    void on_status(status status) override
    {
        cout<<"got pipeline status : "<< status <<endl;
    }

    virtual ~pipeline_handler() {}
private:
    std::shared_ptr<max_depth_value_module> m_max_depth_module;
};

int main () try
{
    //create the cv module, implementing both the video_module_interface and a specific cv module interface.
    //the module must outlive the pipeline
    std::shared_ptr<max_depth_value_module> module = std::make_shared<max_depth_value_module>();

    //create an async pipeline
    std::unique_ptr<pipeline_async_interface> pipeline(new pipeline_async());

    if(pipeline->add_cv_module(module.get()) < status_no_error)
    {
        throw std::runtime_error("failed to add cv module to the pipeline");
    }

    pipeline_common_interface::pipeline_config pipeline_config = {};
    if(pipeline->query_available_config(0, pipeline_config) < status_no_error)
    {
        throw std::runtime_error("failed to query available config from the pipeline");
    }

    if(pipeline->set_config(pipeline_config) < status_no_error)
    {
        throw std::runtime_error("failed to set configuration on the pipeline");
    }

    std::unique_ptr<pipeline_handler> pipeline_callbacks_handler(new pipeline_handler(module));

    if(pipeline->start(pipeline_callbacks_handler.get()) < status_no_error)
    {
        throw std::runtime_error("failed to start pipeline");
    }

    //sleep to let the cv module get some samples
    std::this_thread::sleep_for(std::chrono::seconds(5));

    if(pipeline->stop() < status_no_error)
    {
        throw std::runtime_error("failed to start pipeline");
    }

    return EXIT_SUCCESS;
}
catch (std::exception &e)
{
    cerr<<e.what()<<endl;
    return EXIT_FAILURE;
}

