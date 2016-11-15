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
        *
        * Forward declaration for the actual pipeline implementation as part of the pimpl pattern.
        */
        class pipeline_async_impl;

        /**
         * @class The pipeline_async class
         *
         * An instantiation class of the pipeline_async_interface, for the complete class documantion see pipeline_async_interface.h.
         */
        class pipeline_async : public pipeline_async_interface
        {
        public:
            /**
             * @brief pipeline_async constructor to initialize a pipeline async interface.
             *
             * @param[in] playback_file_path    path to a playback file.
             */
            pipeline_async(const char * playback_file_path = nullptr);

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
            pipeline_async_impl * m_pimpl; /** the actual pipeline async implementation. */
        };
    }
}

