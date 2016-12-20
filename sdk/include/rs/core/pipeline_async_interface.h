// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/** 
* \file pipeline_async_interface.h
* @brief Describes the \c rs::device, \c rs::core::pipeline_async_interface, and \c rs::core::pipeline_async_interface::callback_handler classes.
*/
  
#pragma once
#include "rs/core/correlated_sample_set.h"
#include "rs/core/video_module_interface.h"

namespace rs
{
     /**
     * @brief Forward declaration of \c rs::device.
     */
    class device;

    namespace core
    {
         /** 
         * @brief Utility to simplify the user interaction with computer vision modules and the device.
         *
         * The pipeline abstracts the camera configuration and streaming,
         * and the video modules triggering and threading. It lets the application focus on the computer vision output of the modules.
         * The pipeline can manage computer vision modules, which implement the video module interface. The pipeline is the consumer of the
         * video module interface, while the application consumes the module specific interface, which completes the video module interface.
         * The asynchronous pipeline provides the user application main loop, which runs on the calling thread, and computer vision modules callbacks, which
         * are triggered on different threads.
         */
        class pipeline_async_interface
        {
        public:
            /**
            * @brief Callback handler for the pipeline to interact with the user.
            *
            * The pipeline user can implement the callback handler to be notified asynchronously about pipeline and computer vision module events. 
            * The callbacks are triggered by the pipeline on a different thread than the main streaming loop thread.
            * 
			* The callback execution might be blocking the next camera samples delivery to the application or the video module, and might result in
            * samples drop. To avoid the drop, the callback should be short. See details in the documentation for the specific callback.
            */
            class callback_handler
            {
            public:
                /**
                * @brief User callback to handle new sample set.
                *
                * The callback is called for every new sample set that is received from the camera streams, based on the user configuration set
                * in the \c set_config() method. The pipeline provides the sample set based on the user requirements provided in \c supported_module_config.samples_time_sync_mode:
                * The sample set includes time-synced samples of each enabled stream and motion sensor, or single samples with minimal latency,
                * as described by the \c time_sync_mode parameter of the requested configuration that is passed into \c set_config(). If \c set_config() is not called, 
                * time-synchronized samples of each enabled stream and motion sensor are used by default.
				*
                * To maintain the validity of the samples, set images on other threads. For deferred processing by the application, the user must
                * call \c rs::core::ref_count_interface::add_ref() for each relevant image. The user is responsible to release every image for which the reference counter was incremented.
                * The callback execution blocks the next camera samples delivery to the application, and might result in samples drop.
                * To avoid samples drops, the callback should be short.
                * @param sample_set Sample set containing time-synchronized samples or unsynchronized sample, depending on the pipeline configuration
                */
                virtual void on_new_sample_set(const correlated_sample_set & sample_set) {}

                /**
                * @brief User callback to handle computer vision module processing complete.
				*
				* The callback is called upon computer vision module processing completion of a single sample or a sample set, based on the
                * module configuration and behavior. New module output may be available, based on the processing of the last samples from the camera device.
				*
                * When the callback is triggered, the video module might have new output available for the
                * user. The user may query the module directly (not through the pipeline) for the new output. The module output should include
                * a reference to the relevant sample or image, so the output can be correlated to samples received through the
				* \c on_new_sample_set() method.
                * 
				* For a synchronous processing video module, the callback execution blocks the next camera samples delivery to the video
                * module, and might result in frames drop.
                * 
				* For an asynchronous processing video module, the callback execution might be re-entrant, or blocked by a previous call to this
                * callback, based on the video module internal behavior.
                * To avoid samples drops, the callback should be short.
                * @param cv_module The computer vision module that finished processing
                */
                virtual void on_cv_module_process_complete(video_module_interface * cv_module) {}
                
                /**
                * @brief User callback to handle pipeline asynchronous errors
                *
                * The callback is called to notify the user about pipeline or video modules failures that occur during asynchronous streaming.
                * @param[in] status Error code
                */
                virtual void on_error(rs::core::status status) {}

                virtual ~callback_handler() {}
            };

            /**
            * @brief Adds a computer vision module to the pipeline. 
            *
            * The user has to create and initialize the computer vision module before calling this method. The computer vision module's
            * lifetime must outlive the pipeline's lifetime. The pipeline doesn't own the video module, and doesn't destruct it. It is the
            * user's responsibility to manage the video module memory. Once a video module is attached to the pipeline, the pipeline configuration
            * depends on the module - the pipeline selects a device configuration that satisfies the module, and the pipeline streaming loop
            * triggers the module processing method.
			*
            * If the user calls the \c set_config() method to select the device configuration, subsequent calls to this method will fail until pipeline
            * reset is called.
			*
            * After a module was added to the pipeline, the pipeline will handle the module configuration through \c video_module_interface.
            * Explicit state changing \c video_module_interface calls are forbidden.
            * @param[in] cv_module          Given computer vision module to attach to the pipeline
            * @return status_invalid_state  Computer vision modules cannot be added after the pipeline is configured or streaming. To add more computer
            *                               vision modules, initialize or reset the pipeline
            * @return status_param_inplace  The given computer vision module was already added to the pipeline
            * @return status_no_error       The computer vision module was successfully added to the pipeline
            */
            virtual status add_cv_module(video_module_interface * cv_module) = 0;

            /**
            * @brief Retrieves a computer vision module for a given index.
            *
            * The method allows the user to enumerate all the attached computer vision modules to the pipeline.
            * The video modules memory is managed by the user, not the pipeline. This method merely provides the pointer to the video
            * module implementation.
            * @param[in] index                  Computer vision module index in the pipeline
            * @param[out] cv_module             Computer vision module in the given index
            * @return status_value_out_of_range The given index is out of range
            * @return status_handle_invalid     The given computer vision module handler is invalid
            * @return status_no_error           The computer vision module in the given index was successfully set on the \c cv_module parameter
            */
            virtual status query_cv_module(uint32_t index, video_module_interface ** cv_module) const = 0;

            /**
            * @brief Retrieves the pipeline default configuration for a device.
            *
            * The method provides the preferred configuration of a device detected on the platform by the pipeline. 
            * The configuration is usually selected as a common configuration, which satisfies all computer vision modules provided by the SDK.
            * The device index provides the user the ability to enumerate all devices, and select the device to activate by the pipeline. 
            * The user should call the \c set_config() method with the device name and the preferred configuration, if the specific selection is required.
            * @param[in]  index                  Device index.
            * @param[out] default_config         Pipeline default configuration for the given index
            * @return status_value_out_of_range  The given index is out of range.
            * @return status_no_error            The default configuration was retrieved successfully
            */
            virtual status query_default_config(uint32_t index, video_module_interface::supported_module_config & default_config) const = 0;

            /**
            * @brief Optionally selects the camera configuration explicitly.
            *
            * The method allows the user to select the camera configuration explicitly, instead of internal pipeline selection. The method
            * must be called before pipeline start is called, otherwise it fails. If this method is not called, upon pipeline start the
            * pipeline selects the camera configuration and active streams, based on the video modules requirements.
			*
            * The pipeline can only operate if the camera configuration satisfies all the added video modules, and is supported by a detected
            * device on the platform. The method fails if the requested configuration doesn't satisfy one of the above. After this method
            * is called, no more video modules can be added through add_cv_module. A successful configuration enables the device streams, and
            * configures each computer vision module. The configuration may be set multiple times, overriding previous configurations, until
            * pipeline start is called. Once the pipeline is streaming, configuration change requires pipeline stop. A configured pipeline can
            * be reset with a new set config or by calling reset. The configuration includes additional fields to define how the captured samples
            * are delivered to the user.
			*
            * The pipeline provides the sample set based on the user requirements provided in \c supported_module_config.samples_time_sync_mode.
            * The sample set should include time synced samples of each enabled stream and motion sensor, or single samples with minimal latency,
            * as described by \c time_sync_mode.
            * @param[in] config                 Camera configuration
            * @return status_item_unavailable   The requested device is unavailable
            * @return status_match_not_found    The device does not support this configuration
            * @return status_invalid_state      \c set_config() can be called only when the device is not streaming
            * @return status_no_error           The pipeline was configured successfully
            */
            virtual status set_config(const video_module_interface::supported_module_config & config) = 0;

            /**
            * @brief Returns the current actual device configuration.
            *
            * The method output is valid only if the pipeline configuration was set explicitly, after the \c set_config() method was called, 
            * or the pipeline selected configuration implicitly, after start was called. 
            * @param[out] current_config    Current pipeline configuration
            * @return status_invalid_state  The pipeline is not configured with any active configuration
            * @return status_no_error       The current pipeline configuration was successfully retrieved
            */
            virtual status query_current_config(video_module_interface::actual_module_config & current_config) const = 0;

            /**
            * @brief Starts the pipeline main streaming loop.
            *
            * The pipeline streaming loop captures samples from the camera, and delivers them to the pipeline user and the attached computer
            * vision modules, according to each module requirements and threading model. When start is called, the pipeline applies the user
            * selected configuration to the device, or selects a common configuration, which satisfies all attached video modules. During the
            * loop execution, the pipeline calls the user callbacks, if provided. The streaming loop runs until the pipeline is stopped or reset.
            * Starting the pipeline is possible only if in idle state. If the pipeline is started, the user must call stop or reset before
            * calling start again.
            * @param[in] app_callbacks_handler  A user defined optional callbacks handler
            * @return status_invalid_state      The pipeline is in streaming state
            * @return status_device_failed      The device failed to start
            * @return status_no_error           The pipeline started successfully
            */
            virtual status start(callback_handler * app_callbacks_handler) = 0;

            /**
            * @brief Stops the pipeline main streaming loop
            *
            * The pipeline stops delivering samples to the attached computer vision modules, stops the device streaming and releases the device
            * resources used by the pipeline. It is the user's responsibility to release any image reference owned by its application. The pipeline
            * moves to a configured state, it can be reconfigured or restarted at this state. To add or remove cv modules the user must call reset.
            * Calling start after stop will use the last configuration. The method can be called only if the pipeline state is streaming.
            * @return status_invalid_state  The pipeline state is not streaming
            * @return status_no_error       The pipeline stopped successfully. The pipeline will still contain the last configuration
            */
            virtual status stop() = 0;

            /**
            * @brief Resets the pipeline.
            *
            * The method clears any selected camera configuration, and removes all attached computer vision modules. 
            * After this method returns, the pipeline is back to its initial state.
            * The user may add video modules, set pipeline configuration and call pipeline start again after this call.
            * @return status_no_error  The pipeline reset successfully
            */
            virtual status reset() = 0;

            /**
            * @brief Returns the active or selected camera device.
            *
            * The device handle provides the user access to the camera to set, get, or get information relating to device options. 
            * Since the pipeline controls the device streams configuration, activation state and samples reading, calling the following device API
            * results in unexpected behavior: 
    		* <ul>
            * <li>enable/disable streams <li>enable/disable motion tracking <li>set frame callback <li>start <li>stop <li>wait/poll for frames
			* </ul>
            * The returned device is valid only if the pipeline configuration was set explicitly after the \c set_config() method was called,
            * or the pipeline selected configuration implicitly, after start was called.
            * @return rs::device The active or selected device; null if the pipeline is not configured
            */
            virtual rs::device * get_device() = 0;

            virtual ~pipeline_async_interface() {}
        };
    }
}
