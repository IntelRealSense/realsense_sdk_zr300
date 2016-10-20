// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "rs_sdk_version.h"
#include "rs/cv_modules/max_depth_value_module/max_depth_value_module.h"
#include "max_depth_value_module_impl.h"

using namespace rs::core;
using namespace rs::utils;

namespace rs
{
    namespace cv_modules
    {
        max_depth_value_module::max_depth_value_module(uint64_t milliseconds_added_to_simulate_larger_computation_time):
            m_pimpl(new max_depth_value_module_impl(milliseconds_added_to_simulate_larger_computation_time))
        {}

        int32_t max_depth_value_module::query_module_uid()
        {
            return m_pimpl->query_module_uid();
        }

        status max_depth_value_module::query_supported_module_config(int32_t idx, supported_module_config &supported_config)
        {
            return m_pimpl->query_supported_module_config(idx, supported_config);
        }

        status max_depth_value_module::query_current_module_config(actual_module_config &module_config)
        {
            return m_pimpl->query_current_module_config(module_config);
        }

        status max_depth_value_module::set_module_config(const actual_module_config &module_config)
        {
            return m_pimpl->set_module_config(module_config);
        }

        status max_depth_value_module::process_sample_set_sync(correlated_sample_set *sample_set)
        {
            return m_pimpl->process_sample_set_sync(sample_set);
        }

        status max_depth_value_module::process_sample_set_async(correlated_sample_set *sample_set)
        {
            return m_pimpl->process_sample_set_async(sample_set);
        }

        status max_depth_value_module::register_event_hander(video_module_interface::processing_event_handler *handler)
        {
            return m_pimpl->register_event_hander(handler);
        }

        status max_depth_value_module::unregister_event_hander(video_module_interface::processing_event_handler *handler)
        {
            return m_pimpl->unregister_event_hander(handler);
        }

        video_module_control_interface * max_depth_value_module::query_video_module_control()
        {
            return m_pimpl->query_video_module_control();
        }

        max_depth_value_module::max_depth_value_output_data max_depth_value_module::get_max_depth_value_data()
        {
            return m_pimpl->get_max_depth_value_data();
        }

        max_depth_value_module::~max_depth_value_module()
        {
            delete m_pimpl;
        }
    }
}

