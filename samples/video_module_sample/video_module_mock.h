// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs_core.h"

namespace rs
{
    namespace mock
    {
        /**
         * @brief The video_module_mock class
         * see the video_module_interface for complete documentation.
         */
        class video_module_mock : virtual public rs::core::video_module_interface
        {
        private:
            rs::core::video_module_interface::actual_module_config m_current_module_config;
            rs::core::video_module_interface::processing_event_handler * m_processing_handler;
        public:
            video_module_mock();
            int32_t query_module_uid();
            rs::core::status query_supported_module_config(int32_t idx,
                    rs::core::video_module_interface::supported_module_config &supported_config);
            rs::core::status query_current_module_config(rs::core::video_module_interface::actual_module_config &module_config);
            rs::core::status set_module_config(const rs::core::video_module_interface::actual_module_config &module_config);
            rs::core::status process_sample_set_sync(rs::core::correlated_sample_set *sample_set);
            rs::core::status process_sample_set_async(rs::core::correlated_sample_set *sample_set);
            rs::core::status register_event_hander(rs::core::video_module_interface::processing_event_handler *handler);
            rs::core::status unregister_event_hander(rs::core::video_module_interface::processing_event_handler *handler);
            rs::core::video_module_control_interface *query_video_module_control();
        };
    }
}
