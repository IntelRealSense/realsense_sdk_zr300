// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "correlated_sample_set.h"
#include "types.h"

namespace rs
{
    namespace core
    {
        class projection_interface;

        /**
         * @brief video_module_interface class
         * The video_module_interface defines a common interface to access computer vision modules generically. 
         * The interface provides a common way to configure the module with the active device configuration, based on its available configurations. 
         * It provides methods to execute processing of images and motion samples, and query the module's requirements from the caller for successful processing. 
         * The computer vision data, which the module outputs as a result of samples processing, is unique for each module, thus is not generalized by this interface.
         * The user of this interface should use the video module specific interface in order to query the module output data, and configure module specific features. 
         */
        class video_module_interface
        {
        public:
            /**
             * @structure supported_image_stream_config
             * Describes the module requirements of a single camera images stream configuration parameters. The stream_type matches the index in the containing array.
             * The module sets the fields, which are mandatory or optimal for its implementation. All fields are optional, the module may set part of the fields or none.
             * A zero value for each field means don't care on the specific configuration parameter.
             * The module sets the stream as requested by setting is_enabled to true. The user sets the camera configuration according to the requested parameters,
             * and provides stream images to the module, based on this field.
             */
            struct supported_image_stream_config
            {
                sizeI32        size;               /* image resolution */
                float          frame_rate;         /* stream frame rate */
                sample_flags   flags;              /* optional stream flags */
                bool           is_enabled;         /* is the indexed stream requested by the module. The user should provide images of the stream iff this field is set to true */
            };

            /**
            * @structure supported_motion_sensor_config
            * Describes the motion sensors supported configuration requested by a module implementation.
            */
           struct supported_motion_sensor_config
           {
               float         sample_rate;         /* motion sample rate */
               sample_flags  flags;               /* optional sample flags */
               bool          is_enabled;          /* is the indexed motion sensor is enabled, defaults to 0 = false */
           };

            /**
             * @structure supported_module_config
             * Describes the module requirements from the camera, IMU and caller.
             * The requested streams and their (optional) configuration are set to the stream relevant stream_type index in image_streams_configs array.
             * The module sets is_enabled for each stream_type index it requires for processing. 
             * The requested motion sensors and their (optional) configuration are set to the motion sensor relevant motion_type index in motion_sensors_configs array.
             * The module sets is_enabled for each motion_type index it requires for processing.
             * The module may require a specific device name. A zero array means don't care. 
             * The rest of the configuration parameters instruct the caller how to trigger the module for processing samples.
             */
            struct supported_module_config
            {
                enum class time_sync_mode
                {
                    time_synced_input_only,                      /* processing requires time synced samples sets, which include a sample of each enabled stream and motion sensor. 
                                                                    processing should be called only with the full set of samples, and drop any samples that have no match. */
                    time_synced_input_accepting_unmatch_samples, /* processing requires time synced samples sets, which preferably include a sample of each enabled stream and motion sensor. 
                                                                    processing should be called also for a subset of the enabled streams / motion sensors, in case of samples that have no match. */
                    sync_not_required                            /* processing requires minimal latency for each sample, thus requires no time synchronization of the samples. 
                                                                    processing should be called with one or more samples, which are available at the time of calling. */
                };

                supported_image_stream_config  image_streams_configs[static_cast<uint32_t>(stream_type::max)];  /* requested streams to enable, with optional streams parameters. The index is stream_type */
                supported_motion_sensor_config motion_sensors_configs[static_cast<uint32_t>(motion_type::max)]; /* requested motion sample, index is motion_type*/
                char                           device_name[256];                                                /* requested device name - optional request. Zero means don't care. */
                uint32_t                       concurrent_samples_count;                                        /* the maximum number of images the module may hold reference to concurrently. Defines the required camera buffer pool size for the module */
                time_sync_mode                 samples_time_sync_mode;                                          /* the required samples time synchronization mode, for the input to the processing function */
                bool                           async_processing;                                                /* the module processing model: 
                                                                                                                   async processing implies that the module output data is available when processing_event_handler::module_output_ready() is called. 
                                                                                                                   sync processing implies that the module output data may be available when the processing function returns. */

                __inline supported_image_stream_config& operator[](stream_type stream) { return image_streams_configs[static_cast<uint8_t>(stream)];}
                __inline supported_motion_sensor_config &operator[](motion_type motion) { return motion_sensors_configs[static_cast<uint8_t>(motion)];}
            };

            /**
             * @structure actual_image_stream_config
             * Describes the actual image stream configuration, which is applied to the camera. 
             * The stream parameters are required to configure the module, and must be set before module processing is called.
             * The caller sets the stream as active by setting is_enabled to true. 
             */
            struct actual_image_stream_config
            {
                sizeI32                         size;               /* image resolution */
                float                           frame_rate;         /* stream frame rate */
                sample_flags                    flags;              /* stream flags */
                rs::core::intrinsics            intrinsics;         /* camera intrinsic parameters */
                rs::core::extrinsics            extrinsics;         /* sensor rotation and translation from the camera coordinate system origin, which is
                                                                       located at the center of the "depth" sensor (IR sensor or left camera), to the current stream  */
                rs::core::extrinsics            extrinsics_motion;  /* sensor rotation and translation from the IMU coordinate system origin, to the current stream */
                bool                            is_enabled;         /* is the indexed stream enabled in the camera. The user should provide images of the stream iff this field is set to true  */
            };

            /**
             * @structure actual_motion_sensor_config
             * Describes the actual motion sensor configuration, which is applied to the IMU.
             * The sensor parameters are required to configure the module, and must be set before module processing is called.
             * The caller sets the motion sensor as active by setting is_enabled to true.
             */
            struct actual_motion_sensor_config
            {
                float                               sample_rate;   /* motion sensor sample rate */
                sample_flags                        flags;         /* actual motion sensor flags */
                rs::core::motion_device_intrinsics  intrinsics;    /* motion intrinsic data */
                rs::core::extrinsics                extrinsics;    /* motion extrinsics data (see actual_image_stream_config)*/
                bool                                is_enabled;    /* is the indexed motion sensor enabled. The user should provide samples of the sensor iff this field is set to true  */
            };

            /**
             * @structure actual_module_config
             * Describes the actual module configuration, which includes the active camera streams configuration and IMU configuration. 
             * The module configuration must be set before module processing is called.
             */
            struct actual_module_config
            {
                actual_image_stream_config  image_streams_configs[static_cast<uint32_t>(stream_type::max)];  /* actual enabled streams, with applied streams parameters. The index is stream_type*/
                actual_motion_sensor_config motion_sensors_configs[static_cast<uint32_t>(motion_type::max)]; /* actual enabled motion sensors, with applied sensor parameters. The index is motion_type*/
                rs::core::device_info       device_info;                                                     /* the active device info */
                projection_interface *      projection;                                                      /* [OBSOLETE] projection object for mappings between color and depth images. 
                                                                                                                the objects memory is handled by the caller of the video module.*/

                __inline actual_image_stream_config& operator[](stream_type stream) { return image_streams_configs[static_cast<uint32_t>(stream)]; }
                __inline actual_motion_sensor_config& operator[](motion_type motion) { return motion_sensors_configs[static_cast<uint32_t>(motion)]; }
            };

            /**
             * @brief Return the module unique id
             * @return int32_t  the module unique id.
             */
            virtual int32_t query_module_uid() = 0;

            /**
             * @brief Return the supported module configurations.
             * The function is used to enumerate all supported configurations, and allow the user to select a device configuration, 
             * which satisfies the user application, the available device and the module requirements.
             * The supported module configuration lists the input requirements from the camera streams and the motion sensors.
             * It also provides the requirements from the caller, regarding the module processing flow.   
             * @param[in]  idx                    The zero-based index used to retrieve all configurations.
             * @param[out] supported_config       The module configuration descriptor, to be returned.
             * @return status_no_error            Successful execution.
             * @return status_item_unavailable    Specified index configuration descriptor is unavailable.
             */
            virtual status query_supported_module_config(int32_t idx, supported_module_config& supported_config) = 0;

            /**
             * @brief Return the active configuration, which was set to the module. 
             * The module operation is based on this configuration. The module can't process samples before the configuration is set.
             * @param[out] module_config            The module input descriptor, to be returned.
             * @return status_no_error              Successful execution.
             * @return status_data_not_initialized  No configuration was set to the module
             */
            virtual status query_current_module_config(actual_module_config& module_config) = 0;

            /**
             * @brief Set the active configuration from the actual device.
             * The module sets it processing configuration based on the actual camera streams and motion sensors configuration.
             * The user must call this function before calling the module processing function.
             * After module configuration is set, subsequent calls to set configuration should fail, until reset_config is called.
             * @param[in] module_config     The input descriptor with the device information.
             * @return status_no_error      Successful execution.
             * @return status_init_failed   Configuration failed due to module error or module already configured.
             */
            virtual status set_module_config(const actual_module_config& module_config) = 0;

            /**
             * @brief Process the input sample set.
             * The main module processing function. The module takes as input a sample set, which contains samples from the configured active streams.
             * The user should provide the sample set based on the module requirements provided in supported_module_config.samples_time_sync_mode:
             * The sample set should include time synced samples of each enabled stream and motion sensor, or single samples with minimal latency, 
             * as described by time_sync_mode.
             * The user should expect the module output data to be available based on the module processing model, provided in supported_module_config.async_processing:
             * Async processing - The module output data is available when processing_event_handler::module_output_ready() is called.
             * Sync processing - The module output data may be available when the processing function returns.
             * The sample set may include one or more image samples. Each image lifetime is managed by the module, according to its internal logic:
             * If the module requires image access after process_sample_set returns, the module should call add_ref to own (shared ownership) the image memory before the function returns, 
             * and call release to disown the image when it doesn't require further access to the image, or upon resources flush.
             * The user may call add_ref / release of the image for his own logic, independently of the module behavior. 
             * @param[in]  sample_set           The sample set to process
             * @return status_no_error          Successful execution.
             */
            virtual status process_sample_set(const correlated_sample_set & sample_set) = 0;

            /**
             * @brief processing_event_handler class
             * The user provided callback to handle processing events generated by the module.
             * Module with async processing model, which sets the supported_module_config.async_processing flag, sends the user processing notifications.
             * The user calls process_sample_set once or multiple times, and the module calls the module_output_ready function once it has available output data.
             * The user should call video module specific functions to access the actual data in response to this notification.
             * Handles messages related to the module output data flows.
             * Providing the callback is optional. The user may choose other conditions to query the module output - based on video module specific notifications or  
             * any other application logic.
             */
            class processing_event_handler
            {
            public:
                virtual void module_output_ready(video_module_interface * sender, correlated_sample_set * sample) = 0;
                virtual ~processing_event_handler() {}
            };

            /**
             * @brief register event handler for processing messages
             * The user may optionally register a handler, to receive processing events generated by the module.
             * Registration is only relevant for modules with async processing model, as indicated by supported_module_config.async_processing flag.
             * After handler registration, the module sends the user processing notifications in response to a single or multiple calls to process_sample_set.
             * The function may be called before or after module config is set and process_sample_set was called.
             * @param[in]  handler      The user processing event handler
             * @return status_no_error  Successful execution.
             */
            virtual status register_event_handler(processing_event_handler* handler) = 0;

            /**
             * @brief unregister event handler for processing messages
             * The user may optionally unregister the processing handler, to stop receiving processing events generated by the module.
             * When this function returns, no more calls to handler functions will be called.
             * Registration is only relevant for modules with async processing model, as indicated by supported_module_config.async_processing flag.
             * The function may be called after register_event_handler was called, before or after module config is set and process_sample_set was called.
             * @param[in]  handler      The user processing event handler
             * @return status_no_error          Successful execution.
             * @return status_handle_invalid    No matching handler was registered
             */
            virtual status unregister_event_handler(processing_event_handler* handler) = 0;

            /**
             * @brief Flush the input resources provided to the video module 
             * The video module releases any provided resource, that were set as input to its processing function.
             * When the function returns, the video module has released all images references.
             * @return status_no_error    Successful execution.
             */
            virtual status flush_resources() = 0;

            /**
            * @brief Reset the video module configuration
            * Set the module back to unconfigured state, before actual configuration was set. 
            * The module may release resources allocated for the current configuration activation.
            * After this function is called, the user can set the module configuration by calling set_module_config.
            * @return status_no_error    Successful execution.
            */
            virtual status reset_config() = 0;

            virtual ~video_module_interface() {}
        };
    }
}
