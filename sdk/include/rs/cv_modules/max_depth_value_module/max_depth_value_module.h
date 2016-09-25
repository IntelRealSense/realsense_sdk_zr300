// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs_core.h"
#include "rs/cv_modules/max_depth_value_module/max_depth_value_output_interface.h"

#ifdef WIN32 
#ifdef realsense_max_depth_value_module_EXPORTS
#define  DLL_EXPORT __declspec(dllexport)
#else
#define  DLL_EXPORT __declspec(dllimport)
#endif /* realsense_log_utils_EXPORTS */
#else /* defined (WIN32) */
#define DLL_EXPORT
#endif

namespace rs
{
    namespace cv_modules
    {
        //forward declaration for the actual module impl as part of pimpl pattern
        class DLL_EXPORT max_depth_value_module_impl;

        /**
         * @brief The max_depth_value_module instantiation class
         * an example computer vision module that calculates the max depth value.
         * see the interfaces for the complete documantion coverage.
         */
        class DLL_EXPORT max_depth_value_module : public rs::core::video_module_interface,
                                                  public max_depth_value_output_interface
        {
        public:
            max_depth_value_module(uint64_t m_milliseconds_added_to_simulate_larger_computation_time = 0);

            // video_module_interface interface
            int32_t query_module_uid() override;
            core::status query_supported_module_config(int32_t idx, supported_module_config &supported_config) override;
            core::status query_current_module_config(actual_module_config &module_config) override;
            core::status set_module_config(const actual_module_config &module_config) override;
            core::status process_sample_set_sync(core::correlated_sample_set *sample_set) override;
            core::status process_sample_set_async(core::correlated_sample_set *sample_set) override;
            core::status register_event_hander(processing_event_handler *handler) override;
            core::status unregister_event_hander(processing_event_handler *handler) override;
            core::video_module_control_interface *query_video_module_control() override;

            // max_depth_value_output_interface interface
            max_depth_value_output_data get_max_depth_value_data() override;

            ~max_depth_value_module();
        private:
            max_depth_value_module_impl * m_pimpl;
        };
    }
}
