// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "correlated_sample_set.h"
#include "types.h"

namespace rs
{
    namespace core
    {
        class video_module_interface;

        class video_module_control_interface
        {
        public:
            /**
             * @brief Return the module unique id
             * @return int32_t          the module unique id.
             */
            virtual int32_t query_module_uid() = 0;

            /**
             * @brief The control_event_handler class
             * Handles messages related to the module configuration control.
             */
            class control_event_handler
            {
            public:
                virtual void fatal_error(video_module_control_interface * sender) = 0;
                virtual void profile_change_request(video_module_control_interface * sender) = 0;
                virtual ~control_event_handler() {}
            };

            /**
             * @brief register event hander for control messages
             * @param[in]  handler      the event handler
             * @return STATUS_NO_ERROR          Successful execution.
             */
            virtual status register_event_hander(control_event_handler * handler) = 0;

            /**
             * @brief unregister event hander for control messages
             * @param[in]  handler      the event handler
             * @return STATUS_NO_ERROR          Successful execution.
             * @return STATUS_HANDLE_INVALID    failed to unregister handler
            */
            virtual status unregister_event_hander(control_event_handler * handler) = 0;

            /**
             * @brief light reset of the module when the pipeline changes configuration.
             * @return PXC_STATUS_NO_ERROR    Successful execution.
             */
            virtual status reset() = 0;

            /**
             * @brief Pausing samples processing, no more samples will be sent to the module until resume is called.
             * @return PXC_STATUS_NO_ERROR    Successful execution.
             */
            virtual status pause() = 0;

            /**
             * @brief Resuming samples processing, called after pause was called.
             * @return PXC_STATUS_NO_ERROR    Successful execution.
             */
            virtual status resume() = 0;

            virtual ~video_module_control_interface() {}
        };

    }
}
