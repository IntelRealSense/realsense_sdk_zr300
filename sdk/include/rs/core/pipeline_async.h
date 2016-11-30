// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs/core/pipeline_async_interface.h"

namespace rs
{
    namespace core
    {
        /**
        * @class pipeline_async_impl
        * @brief Forward declaration for the actual pipeline implementation as part of the pimpl pattern.
        */
        class pipeline_async_impl;

        /**
         * @class pipeline_async
         * @brief pipeline_async is an instantiation class as part of the pimpl pattern for pipeline_async_interface.
         *
         * for the complete class documantion see pipeline_async_interface.
         */
        class pipeline_async : public pipeline_async_interface
        {
        public:
            /**
             * @brief pipeline_async constructor to initialize a pipeline async interface with a live camera, currently connected to the platform.
             */
            pipeline_async();

            /**
             * @enum testing_mode
             * @brief The testing_mode enum expresses the pipeline testing modes for record and playback.
             */
            enum class testing_mode
            {
                playback,   /** the streaming source will be a playback file */
                record      /** the streaming source will be a device which is currently connected to the platform, and the streaming output will
                                be recorded to a file */
            };

            /**
             * @brief pipeline_async constructor to initialize a pipeline for testing using record and playback.
             *
             * @param[in] mode            select the pipeline testing mode, streaming from a playback file or record mode, which streams from a live camera
             *                            and records the output to a file.
             * @param[in] file_path       the input file path for playback mode or record mode output file path.
             */
            pipeline_async(const testing_mode mode, const char * file_path);

            pipeline_async(const pipeline_async&) = delete;
            pipeline_async& operator= (const pipeline_async&) = delete;
            pipeline_async(pipeline_async&&) = delete;
            pipeline_async& operator= (pipeline_async&&) = delete;

            virtual status add_cv_module(video_module_interface *cv_module) override;
            virtual status query_cv_module(uint32_t index, video_module_interface **cv_module) const override;
            virtual status query_default_config(uint32_t index, video_module_interface::supported_module_config & default_config) const override;
            virtual status set_config(const video_module_interface::supported_module_config & config) override;
            virtual status query_current_config(video_module_interface::actual_module_config & current_config) const override;
            virtual status reset() override;
            virtual status start(callback_handler * app_callbacks_handler) override;
            virtual status stop() override;
            virtual rs::device * get_device() override;
            virtual ~pipeline_async();
        private:
            pipeline_async_impl * m_pimpl; /**< the actual pipeline async implementation. */
        };
    }
}

