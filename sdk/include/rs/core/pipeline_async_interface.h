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
         * The async pipeline simplifies the user interaction with computer vision modules which implement the video module interface.
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
                /**
                 * @brief user callback called for every new sample set.
                 * @param sample_set  A complete sample set, containing time synced or unsynced samples depending on the pipeline configuration.
                 */
                virtual void on_new_sample_set(const correlated_sample_set & sample_set) {}

                /**
                 * @brief user callback called for every cv module processing completion.
                 * @param cv_module  The cv module which finished processing.
                 */
                virtual void on_cv_module_process_complete(video_module_interface * cv_module) {}
                /**
                 * @brief user callback triggered to notify the user on async flow failures.
                 * @param status  The error code.
                 */
                virtual void on_status(rs::core::status status) {}

                virtual ~callback_handler() {}
            };

            /**
             * @brief Adds an user initialized cv module. The cv module lifetime most outlive the pipeline.
             * @param cv_module[in]          The given cv module.
             * @return status_invalid_state  CV modules can't be added after the pipeline is configured or streaming, to add
             *                               more cv modules initialize or reset the pipeline.
             * @return status_param_inplace  The given cv module was already added to the pipeline.
             * @return status_no_error       The cv module was successfully added to the pipeline.
             */
            virtual status add_cv_module(video_module_interface * cv_module) = 0;

            /**
             * @brief Retrieve a cv module for a given index.
             * @param index[in]                  The cv module index in the pipeline.
             * @param cv_module[out]             The cv module in the given index.
             * @return status_value_out_of_range The given index is out of range.
             * @return status_handle_invalid     The given cv module handler is invalid.
             * @return status_no_error           The cv module in the given index was successfully set on the cv_module parameter.
             */
            virtual status query_cv_module(uint32_t index, video_module_interface ** cv_module) const = 0;

            /**
             * @brief The pipeline default configuration for a device in a given index.
             * @param index[in]                   A device index.
             * @param default_config[out]         The pipeline default configuration for the given index
             * @return status_value_out_of_range  The given index is out of range.
             * @return status_no_error            The default configuration retrieved successfully.
             */
            virtual status query_default_config(uint32_t index, video_module_interface::supported_module_config & default_config) const = 0;

            /**
             * @brief Configure the pipeline with a manual configuration. the configuration must satisfy the requested device and all the added
             * cv module. A successful configuration enables the device streams, and configures each cv_module. a configured pipeline can be
             * resetted with a new set config or by calling reset.
             * @param config[in]                 A pipeline manual configuration.
             * @return status_item_unavailable   Requested device is unavailable.
             * @return status_match_not_found    The device does not support this configuration.
             * @return status_invalid_state      set_config can be called only when the device is not streaming.
             * @return status_no_error           The pipeline was configured successfully.
             */
            virtual status set_config(const video_module_interface::supported_module_config & config) = 0;

            /**
             * @brief Gets the current actual device configuration.
             * @param current_config[out]    The current pipeline configuration.
             * @return status_invalid_state  An unconfigured pipeline can't retrieve the current configuration.
             * @return status_no_error       The current pipeline configuration was successfully retrieved.
             */
            virtual status query_current_config(video_module_interface::actual_module_config & current_config) const = 0;

            /**
             * @brief Start streaming and delivering samples to the added cv module.
             * @param app_callbacks_handler[in]  A user defined callback handler.
             * @return status_invalid_state      A streaming pipeline can't be started, stop streaming or reset to start streaming again.
             * @return status_device_failed      The device failed to start.
             * @return status_no_error           The pipeline started successfully.
             */
            virtual status start(callback_handler * app_callbacks_handler) = 0;

            /**
             * @brief Stop streaming and stop delivering samples to the added cv modules.
             * @return status_invalid_state  The pipeline can't stop streaming if its currently not streaming.
             * @return status_no_error       The pipeline stopped successfully. The pipeline will still contain the last configuration.
             */
            virtual status stop() = 0;

            /**
             * @brief Reset the pipeline, removed any set configurations, the pipeline is back in initialized state.
             * @return status_no_error  The pipeline resetted successfully.
             */
            virtual status reset() = 0;

            /**
             * @brief Returns the current device, Lets the user set, get, or get info of device options. calling the following device API
             * will cause unexpected behaviors : enable\disable streams, enable\disable motion tracking, set frame callback, start, stop,
             * wait\poll for frames.
             * @return rs::device  The current device, null if the pipeline is not configured.
             */
            virtual rs::device * get_device() = 0;

            virtual ~pipeline_async_interface() {}
        };
    }
}
