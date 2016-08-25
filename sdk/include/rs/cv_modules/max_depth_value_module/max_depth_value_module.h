// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "rs_core.h"
#include "rs/cv_modules/max_depth_value_module/max_depth_value_module_interface.h"

namespace rs
{
    namespace cv_modules
    {
        //forward declaration for the actual module impl as part of pimpl pattern
        class max_depth_value_module_impl;

        /**
         * @brief The max_depth_value_module instantiation class
         * an example computer vision module that calculates the max depth value.
         * see the interfaces for the complete documantion coverage.
         */
        class max_depth_value_module : public rs::core::video_module_interface, public max_depth_value_module_interface
        {
        public:
            max_depth_value_module(const max_depth_value_module & other) = delete;
            max_depth_value_module & operator=(const max_depth_value_module & other) = delete;
            max_depth_value_module(max_depth_value_module && other) = delete;
            max_depth_value_module & operator=(max_depth_value_module && other) = delete;

            max_depth_value_module(uint64_t milliseconds_added_to_simulate_larger_computation_time = 0);

            // video_module_interface impl
            int32_t query_module_uid() override;
            rs::core::status query_supported_module_config(int32_t idx,
                    rs::core::video_module_interface::supported_module_config &supported_config) override;
            rs::core::status query_current_module_config(rs::core::video_module_interface::actual_module_config &module_config) override;
            rs::core::status set_module_config(const rs::core::video_module_interface::actual_module_config &module_config) override;
            rs::core::status process_sample_set_sync(rs::core::correlated_sample_set *sample_set) override;
            rs::core::status process_sample_set_async(rs::core::correlated_sample_set *sample_set) override;
            rs::core::status register_event_hander(rs::core::video_module_interface::processing_event_handler *handler) override;
            rs::core::status unregister_event_hander(rs::core::video_module_interface::processing_event_handler *handler) override;
            rs::core::video_module_control_interface *query_video_module_control() override;

            // max_depth_value_module_interface impl
            max_depth_value_output_data get_max_depth_value_data() override;

            ~max_depth_value_module();

        private:
            max_depth_value_module_impl * m_pimpl;
        };
    }
}
