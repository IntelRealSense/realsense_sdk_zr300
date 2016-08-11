// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "correlated_sample_set.h"
#include "types.h"

namespace rs
{
    namespace core
    {
        class video_module_control_interface;
        class projection_interface;

        /**
         * @brief The video_module_interface class
         * the video_module_interface defines a common interface for computer vision modules to
         * expose available configurations, set configuration, and to process images. computer vision module outputs
         * are unique for each module and provided seperatly by the module implemantor.
         */
        class video_module_interface
        {
        public:
            /**
             * @structure supported_image_stream_config
             * Describes an image stream supported configuration requested by a module implementation.
             */
            struct supported_image_stream_config
            {
                sizeI32        min_size;           /* minimum size */
                sizeI32        ideal_size;         /* maximum size */
                float          minimal_frame_rate; /* minimal operetional frame rate */
                float          ideal_frame_rate;   /* frame rate for ideal operation */
                sample_flags   flags;              /* optional samples flags */
                preset_type    preset;             /* stream preset */
                bool           is_enabled;         /* is the indexed stream is enabled, defaults to 0 = false */
            };

            /**
             * @structure supported_motion_sensor_config
             * Describes the motion sensors supported configuration requested by a module implementation.
             */
            struct supported_motion_sensor_config
            {
                float         minimal_frame_rate;  /* minimal operetional frame rate */
                float         ideal_frame_rate;    /* frame rate for ideal operation */
                sample_flags  flags;               /* optional supported flags */
                bool          is_enabled;          /* is the indexed motion sensor is enabled, defaults to 0 = false */
                // range
            };

            /**
             * @structure supported_module_config
             * Describes a supported module configuration
             */
            struct supported_module_config
            {
                enum flags
                {
                    time_synced_input          = 0x1, /* this configuration requires time synced samples set
                                                         with all the requested streams and motion samples to operate. */
                    accept_unmatch_samples     = 0x2, /* complete sample sets are required, but its not mandatory. */
                    async_processing_supported = 0x4, /* this configuration supports async sample set processing */
                    sync_processing_supported  = 0x8  /* this configuration supports sync sample set processing */
                };

                supported_image_stream_config  image_streams_configs[static_cast<uint32_t>(stream_type::max)];  /* requested stream characters, index is stream_type*/
                supported_motion_sensor_config motion_sensors_configs[static_cast<uint32_t>(motion_type::max)]; /* requested motion sample, index is motion_type*/
                char                           device_name[256];                                                /* requested device name */
                uint32_t                       concurrent_samples_count;                                        /* requested number of concurrent samples the module can process */
                flags                          config_flags;                                                    /* mode of operation flags */

                __inline supported_image_stream_config &operator[](stream_type stream) { return image_streams_configs[static_cast<uint8_t>(stream)];}
                __inline supported_motion_sensor_config &operator[](motion_type motion) { return motion_sensors_configs[static_cast<uint8_t>(motion)];}
            };

            /**
             * @structure actual_image_stream_config
             * Describes an actual image stream configuration to configure the module.
             */
            struct actual_image_stream_config
            {
                sizeI32                         size;               /* stream resolution */
                float                           frame_rate;         /* stream frame rate */
                sample_flags                    flags;              /* stream flags */
                preset_type                     preset;             /* stream preset */
                rs::core::intrinsics            intrinsics;         /* stream intrinsic data */
                rs::core::extrinsics            extrinsics;         /* sensor rotation and translation from the camera coordinate system origin,
                                                                       which is located at the center of the "depth" sensor (IR sensor or left camera)  */
                bool                            is_enabled;         /* is the indexed stream is enabled, defaults to 0 = false */
            };

            /**
             * @structure actual_motion_sensor_config
             * Describes an actual motion sensor configuration to configure the module.
             */
            struct actual_motion_sensor_config
            {
                float                        frame_rate;    /* actual configured frame rate */
                sample_flags                 flags;         /* actual motion sensor flags */
                bool                         is_enabled;    /* is the indexed motion sensor is enabled, defaults to 0 = false */
                rs::core::motion_intrinsics  intrinsics;    /* motion intrinsic data */
                rs::core::extrinsics         extrinsics;    /* motion extrinsics data (see actual_image_steam_config)*/
            };

            /**
             * @structure actual_module_config
             * Describes an actual module configuration to configure the module.
             */
            struct actual_module_config
            {
                actual_image_stream_config  image_streams_configs[static_cast<uint32_t>(stream_type::max)];  /* requested stream characters, index is stream_type*/
                actual_motion_sensor_config motion_sensors_configs[static_cast<uint32_t>(motion_type::max)]; /* requested motion sersors, index is motion_type*/
                rs::core::device_info       device_info;                                                     /* requested device info */
                projection_interface *      projection;                                                      /* projection object for mappings between color and
                                                                                                                depth coordinate systems. the objects memory is
                                                                                                                handled outside of the video module.*/

                __inline actual_image_stream_config &operator[](stream_type stream) { return image_streams_configs[static_cast<uint32_t>(stream)]; }
                __inline actual_motion_sensor_config &operator[](motion_type motion) { return motion_sensors_configs[static_cast<uint32_t>(motion)]; }
            };

            /**
             * @brief Return the module unique id
             * @return int32_t          the module unique id.
             */
            virtual int32_t query_module_uid() = 0;

            /**
             * @brief Return the supported module configurations.
             * @param[in]  idx                    The zero-based index used to retrieve all configurations.
             * @param[out] supported_config       The module input descriptor, to be returned.
             * @return STATUS_NO_ERROR            Successful execution.
             * @return STATUS_ITEM_UNAVAILABLE    No specified input descriptor is not available.
             * @return STATUS_HANDLE_INVALID      out parameter is not initialized.
             */
            virtual status query_supported_module_config(int32_t idx, supported_module_config & supported_config) = 0;

            /**
             * @brief Return the active configuration that the module works on.
             * @param[out] module_config          The module input descriptor, to be returned.
             * @return STATUS_NO_ERROR            Successful execution.
             */
            virtual status query_current_module_config(actual_module_config &module_config) = 0;

            /**
             * @brief Set the active configuration from the actual device.
             * @param[in] module_config   The input descriptor with the device information.
             * @return STATUS_NO_ERROR    Successful execution.
             */
            virtual status set_module_config(const actual_module_config &module_config) = 0;

            /**
             * @brief Process sample synchronously (in caller thread).
             * @param[in]  sample_set      The sample set to process
             * @return STATUS_NO_ERROR    Successful execution.
             */
            virtual status process_sample_set_sync(correlated_sample_set * sample_set) = 0;

            /**
             * @brief Process sample asynchronously, must not block.
             * @param[in]  sample_set      The sample set to process
             * @return STATUS_NO_ERROR          Successful execution.
             * @return STATUS_HANDLE_INVALID    data path event handler registration is missing.
             */
            virtual status process_sample_set_async(correlated_sample_set * sample_set) = 0;

            /**
             * @brief The processing_event_handler class
             * Handles messages related to the processing pipline data flow.
             */
            class processing_event_handler
            {
            public:
                virtual void process_sample_complete(video_module_interface * sender, correlated_sample_set * sample) = 0;
                virtual ~processing_event_handler() {}
            };

            /**
             * @brief register event hander for processing messages
             * @param[in]  handler      the event handler
             * @return STATUS_NO_ERROR          Successful execution.
             */
            virtual status register_event_hander(processing_event_handler * handler) = 0;

            /**
             * @brief unregister event hander for processing messages
             * @param[in]  handler      the event handler
             * @return STATUS_NO_ERROR          Successful execution.
             * @return STATUS_HANDLE_INVALID    failed to unregister handler
             */
            virtual status unregister_event_hander(processing_event_handler * handler) = 0;

            /**
             * @brief Returns an optional module controller
             * @return video_module_control_interface     The control object
             */
            virtual video_module_control_interface * query_video_module_control()
            {
                return nullptr;
            }

            virtual ~video_module_interface() {}
        };
    }
}
