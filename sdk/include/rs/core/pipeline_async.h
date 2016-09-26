// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs/core/pipeline_async_interface.h"

namespace rs
{
    namespace core
    {
        class pipeline_async_impl;

        /**
        TODO : document
        */
        class pipeline_async : public pipeline_async_interface
        {
        public:
            pipeline_async(const char * playback_file_path = nullptr);

            // pipeline_common_interface
            status add_cv_module(video_module_interface *cv_module) override;
            status query_cv_module(uint32_t index, video_module_interface **cv_module) const override;
            status query_available_config(uint32_t index, pipeline_config &available_config) const override;
            status set_config(const pipeline_config &config) override;
            status query_current_config(pipeline_config &current_pipeline_config) const override;
            status reset() override;

            // pipeline_async_interface
            virtual status start(callback_handler * app_callbacks_handler) override;
            virtual status stop() override;

            virtual ~pipeline_async();
        private:
            pipeline_async_impl * m_pimpl;
        };
    }
}

