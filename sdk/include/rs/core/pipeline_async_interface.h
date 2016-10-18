// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs/core/correlated_sample_set.h"
#include "rs/core/video_module_interface.h"

namespace rs
{
    //forward declaration of the device handle;
    class device;

    namespace core
    {
         /**
         * @brief The pipeline_async_interface class
         * the async pipeline simplifies the user interaction with cv modules.
         */
        class pipeline_async_interface
        {
        public:
            /**
             * @brief The handler class
             * Async callbacks handler for the application to implement to get pipeline async calls
             */
            class callback_handler
            {
            public:
                virtual void on_new_sample_set(correlated_sample_set * sample_set) {}
                virtual void on_cv_module_process_complete(int32_t unique_module_id) {}
                virtual void on_status(rs::core::status status) {}
                virtual ~callback_handler() {}
            };

            virtual status add_cv_module(video_module_interface * cv_module) = 0;
            virtual status query_cv_module(uint32_t index, video_module_interface ** cv_module) const = 0;
            virtual status query_available_config(uint32_t index, video_module_interface::supported_module_config & available_config) const = 0;
            virtual status set_config(const video_module_interface::supported_module_config & config) = 0;
            virtual status query_current_config(video_module_interface::actual_module_config & current_config) const = 0;
            virtual status reset() = 0;
            virtual status start(callback_handler * app_callbacks_handler) = 0;
            virtual status stop() = 0;

            //TODO : define specific allowed methods
            virtual rs::device * get_device_handle() = 0;

            virtual ~pipeline_async_interface() {}
        };
    }
}

