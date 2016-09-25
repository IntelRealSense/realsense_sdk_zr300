// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once

#include <map>
#include <memory>
#include <vector>
#include <tuple>

#include "rs/core/pipeline_common_interface.h"

namespace rs
{
    namespace core
    {
        class context_interface;

        /**
         * @brief The pipeline_common class
         * TODO : document
         */
        class pipeline_common : public virtual pipeline_common_interface
        {
        public:
            pipeline_common(const char * playback_file_path = nullptr);

            virtual status add_cv_module(video_module_interface * cv_module) override;
            virtual status query_cv_module(uint32_t index, video_module_interface ** cv_module) override;
            virtual status query_available_config(uint32_t index, pipeline_common_interface::pipeline_config & available_config) override;
            virtual status set_config(const pipeline_common_interface::pipeline_config & pipeline_config) override;
            virtual status query_current_config(pipeline_common_interface::pipeline_config & current_pipeline_config) override;
            virtual status reset() override;
        protected:
            std::unique_ptr<context_interface> m_context;

            std::vector<std::tuple<video_module_interface *,
                                   video_module_interface::actual_module_config,
                                   video_module_interface::supported_module_config>> get_cv_module_configurations();

            virtual ~pipeline_common();

        private:
            std::vector<video_module_interface *> m_cv_modules;
            std::map<video_module_interface *, std::pair<video_module_interface::actual_module_config,
                                                         video_module_interface::supported_module_config>> m_modules_configs;

            status get_intersecting_modules_config(std::vector<pipeline_common_interface::pipeline_config> & intersecting_modules_configs);
            status filterout_configs_unsupported_the_context(std::vector<pipeline_common_interface::pipeline_config> & configs);
            bool is_theres_satisfying_module_config(video_module_interface * cv_module,
                                                    const video_module_interface::actual_module_config & given_config,
                                                    video_module_interface::actual_module_config &satisfying_config,
                                                    video_module_interface::supported_module_config &satisfying_config_extended_info);

        };
    }
}

