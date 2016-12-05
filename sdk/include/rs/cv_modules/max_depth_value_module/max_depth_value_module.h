// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.


/** 
* \file max_depth_value_module.h
* @brief Describes the \c rs::cv_modules::max_depth_value_module and \c rs::cv_modules::max_depth_value_module_impl classes.
**/

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
        /**
        * @brief Forward declaration for the maximum depth value module implementation, as part of the pimpl pattern.
        */
        class DLL_EXPORT max_depth_value_module_impl;

        /**
         * @brief Example computer vision module that calculates the maximum depth value.
         * 
		 * See the interfaces for complete documentation.
         */
        class DLL_EXPORT max_depth_value_module : public rs::core::video_module_interface,
                                                  public max_depth_value_output_interface
        {
        public:
            /**
             * @brief Constructor
             * @param m_milliseconds_added_to_simulate_larger_computation_time  Milliseconds added to simulate larger computation time
             * @param is_async_processing                                       Configures the module in sync or async processing mode
             */
            max_depth_value_module(uint64_t m_milliseconds_added_to_simulate_larger_computation_time = 0,
                                   bool is_async_processing = true);

            max_depth_value_module(const max_depth_value_module&) = delete;
            max_depth_value_module& operator= (const max_depth_value_module&) = delete;
            max_depth_value_module(max_depth_value_module&&) = delete;
            max_depth_value_module& operator= (max_depth_value_module&&) = delete;

            // video_module_interface interface
            int32_t query_module_uid() override;
            core::status query_supported_module_config(int32_t idx, supported_module_config &supported_config) override;
            core::status query_current_module_config(actual_module_config &module_config) override;
            core::status set_module_config(const actual_module_config &module_config) override;
            core::status process_sample_set(const core::correlated_sample_set & sample_set) override;
            core::status register_event_handler(processing_event_handler *handler) override;
            core::status unregister_event_handler(processing_event_handler *handler) override;
            core::status flush_resources() override;
            core::status reset_config() override;

            // max_depth_value_output_interface interface
            max_depth_value_output_data get_max_depth_value_data() override;

            ~max_depth_value_module();
        private:
            max_depth_value_module_impl * m_pimpl;
        };
    }
}
