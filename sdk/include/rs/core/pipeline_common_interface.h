// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs/core/types.h"
#include "rs/core/status.h"
#include "rs/core/video_module_interface.h"

namespace rs
{
    namespace core
    {   
        /**
        TODO : document
        */
        class pipeline_common_interface
        {
        public:
            struct pipeline_config
            {
                pipeline_config():module_config({}){}
                video_module_interface::actual_module_config module_config;
                bool is_parallel_cv_processing;
                uint32_t deviceId;
            };

            virtual status add_cv_module(video_module_interface * cv_module) = 0;
            virtual status query_cv_module(uint32_t index, video_module_interface ** cv_module) const = 0;
            virtual status query_available_config(uint32_t index, pipeline_config & available_config) const = 0;
            virtual status set_config(const pipeline_config & config) = 0;
            virtual status query_current_config(pipeline_config & current_pipeline_config) const = 0;
            virtual status reset() = 0;

        protected:
            virtual ~pipeline_common_interface() {}
        };
    }
}

