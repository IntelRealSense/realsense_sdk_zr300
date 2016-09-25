// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs/core/pipeline_common_interface.h"
#include "rs/core/correlated_sample_set.h"

namespace rs
{
    namespace core
    {
        /**
        TODO : document
        */
        class pipeline_async_interface :  public virtual pipeline_common_interface
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

            virtual status start(callback_handler * app_callbacks_handler) = 0;
            virtual status stop() = 0;

            virtual ~pipeline_async_interface() {}
        };
    }
}

