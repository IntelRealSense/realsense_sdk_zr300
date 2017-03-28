// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.


/** 
* \file video_module_interface.h
* @brief Describes the \c rs::core::video_module_interface class.
*/
  
#pragma once
#include "correlated_sample_set.h"
#include "types.h"

namespace rs
{
    namespace core
    {
         /**
         * @brief Forward declaration of \c rs::core::projection_interface.
         */
        class projection_interface;

        /**
        * @brief Defines a common interface to access computer vision modules generically.
        *
        * The interface provides a common way to configure the module with the active device configuration, based on its available configurations. 
        * It provides methods to execute processing of images and motion samples, and query the module's requirements from the caller for successful processing. 
        * The computer vision data, which the module outputs as a result of samples processing, is unique for each module, thus is not generalized by this interface.
        * The user of this interface should use the video module specific interface in order to query the module output data, and configure module specific features. 
        */
        class video_module_interface
        {
        public:
            /**
            * @brief Describes the module requirements of a single camera images stream configuration parameters.
            *
            * The stream_type matches the index in the containing array. The module sets the fields, which are mandatory or optimal for its implementation.
            * All fields are optional: the module may set part of the fields or none. A zero value for each field means that the specific
            * configuration parameter can be ignored. The module sets the stream as requested by setting \c is_enabled to true. The user sets the camera configuration
            * according to the requested parameters, and provides stream images to the module, based on this field.
            */
            struct supported_image_stream_config
            {
                sizeI32        size;               /**< Image resolution */
                float          frame_rate;         /**< Stream frame rate */
                sample_flags   flags;              /**< Optional stream flags */
                bool           is_enabled;         /**< Specifies whether the indexed stream is requested by the module. The user should provide images of the stream if this field is set to true. */
            };

            /**
            * @brief Describes the motion sensors supported configuration requested by a module implementation.
            */
           struct supported_motion_sensor_config
           {
               float         sample_rate;         /**< Motion sample rate */
               sample_flags  flags;               /**< Optional sample flags */
               bool          is_enabled;          /**< Specifies whether the indexed motion sensor is enabled, defaults to 0 = false */
           };

            /**
            * @brief Describes the module requirements from the camera, IMU and caller.
            *
            * The requested streams and their (optional) configuration are set to the stream relevant \c stream_type index in the \c image_streams_configs array.
            * The module sets \c is_enabled for each \c stream_type index it requires for processing. 
            * The requested motion sensors and their (optional) configuration are set to the motion sensor relevant \c motion_type index in the \c motion_sensors_configs array.
            * The module sets \c is_enabled for each \c motion_type index it requires for processing.
            * The module might require a specific device name. A zero array means it can be ignored. 
            * The rest of the configuration parameters instruct the caller how to trigger the module for processing samples.
            */
            struct supported_module_config
            {
                /**
                * @brief Defines the configuration samples processing mode, how samples should be delivered to the CV module.
                */
                enum class time_sync_mode
                {
                    time_synced_input_only,                      /**< Processing requires time synced samples sets, which include a sample of each enabled stream and motion sensor.
                                                                      Processing should be called only with the full set of samples, and drop any samples that have no match. */
                    time_synced_input_accepting_unmatch_samples, /**< Processing requires time synced samples sets, which preferably include a sample of each enabled stream and motion sensor.
                                                                      Processing should be called also for a subset of the enabled streams or motion sensors, in cases of samples that have no match. */
                    sync_not_required                            /**< Processing requires minimal latency for each sample, thus it requires no time synchronization of the samples.
                                                                      Processing should be called with one or more samples, which are available at the time of calling. */
                };

                supported_image_stream_config  image_streams_configs[static_cast<uint32_t>(stream_type::max)];  /**< Requested streams to enable, with optional streams parameters. The index is \c stream_type.*/
                supported_motion_sensor_config motion_sensors_configs[static_cast<uint32_t>(motion_type::max)]; /**< Requested motion sample. The index is \c motion_type. */
                char                           device_name[256];                                                /**< Requested device name - optional request. Null terminated empty string is ignored. */
                uint32_t                       concurrent_samples_count;                                        /**< The maximum number of images the module may hold reference to concurrently. Defines the required camera buffer pool size for the module. */
                time_sync_mode                 samples_time_sync_mode;                                          /**< The required samples time synchronization mode, for the input to the processing method */
                bool                           async_processing;                                                /**< The module processing model:
                                                                                                                     async processing implies that the module output data is available when \c processing_event_handler::module_output_ready() is called; 
                                                                                                                     sync processing implies that the module output data might be available when the processing method returns. */

                /**
                * @brief Gets a stream configuration reference by stream type.
                *
                * @param[in] stream \c stream_type to which a reference to a \c supported_image_stream_config will be returned
                * @return supported_image_stream_config& Supported image stream configuration
                */
                inline supported_image_stream_config& operator[](stream_type stream) { return image_streams_configs[static_cast<uint8_t>(stream)];}

                /**
                * @brief Gets a motion configuration reference by motion type.
                *
                * @param[in] motion \c motion_type to which a reference to a \c supported_motion_sensor_config will be returned.
                * @return supported_motion_sensor_config & Supported motion sensor config
                */
                inline supported_motion_sensor_config &operator[](motion_type motion) { return motion_sensors_configs[static_cast<uint8_t>(motion)];}
            };

            /**
            * @brief Describes the actual image stream configuration, which is applied to the camera.
            *
            * The stream parameters are required to configure the module, and must be set before module processing is called.
            * The caller sets the stream as active by setting \c is_enabled to true. 
            */
            struct actual_image_stream_config
            {
                sizeI32                         size;               /**< Image resolution */
                float                           frame_rate;         /**< Stream frame rate */
                sample_flags                    flags;              /**< Stream flags */
                rs::core::intrinsics            intrinsics;         /**< Camera intrinsic parameters */
                rs::core::extrinsics            extrinsics;         /**< Sensor rotation and translation from the camera coordinate system origin, which is
                                                                         located at the center of the depth sensor (IR sensor in case there is one IR sensor, 
                                                                         or left IR sensor in case there are two IR sensors), to the current stream  */
                rs::core::extrinsics            extrinsics_motion;  /**< Sensor rotation and translation from the current stream, to the IMU coordinate system origin */
                bool                            is_enabled;         /**< Specifies whether the indexed stream is enabled in the camera. The user should provide images of the stream if this field is set to true.  */
            };

            /**
            * @brief Describes the actual motion sensor configuration, which is applied to the IMU.
            *
            * The sensor parameters are required to configure the module, and must be set before module processing is called.
            * The caller sets the motion sensor as active by setting \c is_enabled to true.
            */
            struct actual_motion_sensor_config
            {
                float                               sample_rate;   /**< Motion sensor sample rate */
                sample_flags                        flags;         /**< Actual motion sensor flags */
                rs::core::motion_device_intrinsics  intrinsics;    /**< Motion intrinsic data */
                rs::core::extrinsics                extrinsics;    /**< Motion extrinsics data (see \c actual_image_stream_config)*/
                bool                                is_enabled;    /**< Specifies whether the indexed motion sensor is enabled. The user should provide samples of the sensor if this field is set to true.  */
            };

            /**
            * @brief Describes the actual module configuration, which includes the active camera streams configuration and IMU configuration.
            *
            * The module configuration must be set before module processing is called.
            */
            struct actual_module_config
            {
                actual_image_stream_config  image_streams_configs[static_cast<uint32_t>(stream_type::max)];  /**< Actual enabled streams, with applied streams parameters. The index is \c stream_type. */
                actual_motion_sensor_config motion_sensors_configs[static_cast<uint32_t>(motion_type::max)]; /**< Actual enabled motion sensors, with applied sensor parameters. The index is \c motion_type. */
                rs::core::device_info       device_info;                                                     /**< Active device info */
                projection_interface *      projection;                                                      /**< [OBSOLETE] projection object for mappings between color and depth images.
                                                                                                                  The object's memory is handled by the caller of the video module.*/

                /**
                * @brief Gets a stream config reference by stream type.
                *
                * @param[in] stream \c stream_type to which a reference to an \c actual_image_stream_config will be returned
                * @return actual_image_stream_config& Actual image stream configuration
                */
                inline actual_image_stream_config& operator[](stream_type stream) { return image_streams_configs[static_cast<uint32_t>(stream)]; }

                /**
                * @brief Gets a motion config reference by motion type.
                *
                * @param[in] motion \c motion_type to which a reference to an \c actual_motion_sensor_config will be returned
                * @return actual_motion_sensor_config& Actual motion sensor configuration
                */
                inline actual_motion_sensor_config& operator[](motion_type motion) { return motion_sensors_configs[static_cast<uint32_t>(motion)]; }
            };

            /**
            * @brief Returns the module unique id
            *
            * @return int32_t Id
            */
            virtual int32_t query_module_uid() = 0;

            /**
            * @brief Returns the supported module configurations.
            *
            * The method is used to enumerate all supported configurations, and allow the user to select a device configuration, 
            * which satisfies the user application, the available device, and the module requirements.
            * The supported module configuration lists the input requirements from the camera streams and the motion sensors.
            * It also provides the requirements from the caller, regarding the module processing flow.   
            * @param[in]  idx                    Zero-based index used to retrieve all configurations
            * @param[out] supported_config       Module configuration descriptor to be returned
            * @return status_no_error            Successful execution
            * @return status_item_unavailable    The specified index configuration descriptor is unavailable.
            */
            virtual status query_supported_module_config(int32_t idx, supported_module_config& supported_config) = 0;

            /**
            * @brief Returns the active configuration, which was set to the module. 
            *
            * The module operation is based on this configuration. The module cannot process samples before the configuration is set.
            * @param[out] module_config            Module input descriptor to be returned
            * @return status_no_error              Successful execution
            * @return status_data_not_initialized  No configuration was set to the module.
            */
            virtual status query_current_module_config(actual_module_config& module_config) = 0;

            /**
            * @brief Sets the active configuration from the actual device.
            *
            * The module sets its processing configuration based on the actual camera streams and motion sensors configuration.
            * The user must call this method before calling the module processing method.
            * After module configuration is set, subsequent calls to set configuration should fail, until \c reset_config is called.
            * @param[in] module_config     Input descriptor with the device information
            * @return status_no_error      Successful execution
            * @return status_init_failed   Configuration failed due to module error or module already configured
            */
            virtual status set_module_config(const actual_module_config& module_config) = 0;

            /**
            * @brief Processes the input sample set.
            *
            * The main module processing method. The module takes as input a sample set, which contains samples from the configured active streams.
            * The user should provide the sample set based on the module requirements provided in \c supported_module_config.samples_time_sync_mode:
            * The sample set should include time synced samples of each enabled stream and motion sensor, or single samples with minimal latency, 
            * as described by \c time_sync_mode.
            * The user should expect the module output data to be available based on the module processing model, provided in \c supported_module_config.async_processing:
            * <ul>
            * <li>Async processing: The module output data is available when \c processing_event_handler::module_output_ready() is called.
            * <li>Sync processing: The module output data might be available when the processing method returns.
            * </ul>
            * The sample set may include one or more image samples. Each image lifetime is managed by the module, according to its internal logic:
            * If the module requires image access after \c process_sample_set() returns, the module should call \c rs::core::ref_count_interface::add_ref() to own (that is share ownership of) 
			* the image memory before the method returns, 
            * and call \c release() to disown the image when it does not require further access to the image, or upon resources flush.
            * Users may call \c add_ref() or release of the image for their own logic, independently of the module behavior.
            * @param[in]  sample_set   Sample set to process
            * @return status_no_error  Successful execution
            */
            virtual status process_sample_set(const correlated_sample_set & sample_set) = 0;

            /**
            * @brief User-provided callback to handle processing events generated by modules and the device.
            *
            * Module with async processing model, which sets the \c supported_module_config.async_processing flag and sends the user processing notifications.
            * The user calls \c process_sample_set() once or multiple times, and the module calls the \c module_output_ready() method once it has available output data.
            * The user should call video module specific methods to access the actual data in response to this notification.
            *
            * The method handles messages related to the module output data flows.
            *
            * Providing the callback is optional. The user may choose other conditions to query the module output, based on video module specific notifications or  
            * any other application logic.
            */
            class processing_event_handler
            {
            public:
                /**
                * @brief Callback notification due to available CV module output data.
                *
                * @param sender    CV module which has a ready output
                * @param sample    Input sample set that was processed to generate the CV module output data
                */
                virtual void module_output_ready(video_module_interface * sender, correlated_sample_set * sample) = 0;

                virtual ~processing_event_handler() {}
            };

            /**
            * @brief Registers event handler for processing messages.
            *
            * The user may optionally register a handler, to receive processing events generated by the module.
            * Registration is only relevant for modules with async processing model, as indicated by the \c supported_module_config.async_processing flag.
            * After handler registration, the module sends the user processing notifications in response to a single or multiple calls to \c process_sample_set().
			*
            * The method may be called before or after module config is set and \c process_sample_set() was called.
            * @param[in]  handler      User processing event handler
            * @return status_no_error  Successful execution
            */
            virtual status register_event_handler(processing_event_handler* handler) = 0;

            /**
            * @brief Unregisters event handler for processing messages.
            *
            * The user may optionally unregister the processing handler, to stop receiving processing events generated by the module.
            * When this method returns, no more calls to handler methods will be called.
            * Registration is only relevant for modules with async processing model, as indicated by the \c supported_module_config.async_processing flag.
            * The method may be called after \c register_event_handler() was called, before or after module configuration is set and \c process_sample_set() was called.
            * @param[in]  handler      User processing event handler
            * @return status_no_error          Successful execution
            * @return status_handle_invalid    No matching handler was registered.
            */
            virtual status unregister_event_handler(processing_event_handler* handler) = 0;

            /**
            * @brief Flushes the input resources provided to the video module. 
            *
            * The video module releases any provided resource that were set as input to its processing method.
            * When the method returns, the video module has released all image references.
            * @return status_no_error    Successful execution
            */
            virtual status flush_resources() = 0;

            /**
            * @brief Resets the video module configuration.
            *
            * Set the module back to its unconfigured state, that is, as it was before actual configuration was set. 
            * The module may release resources allocated for the current configuration activation.
            * After this method is called, the user can set the module configuration by calling \c set_module_config().
            * @return status_no_error    Successful execution
            */
            virtual status reset_config() = 0;

            virtual ~video_module_interface() {}
        };
    }
}
