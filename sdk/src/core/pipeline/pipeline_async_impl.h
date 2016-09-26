// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <vector>
#include <functional>
#include <memory>
#include <mutex>
#include <librealsense/rs.hpp>

#include "rs/core/pipeline_async_interface.h"

#include "samples_consumer_base.h"
#include "streaming_device_manager.h"

namespace rs
{
    namespace core
    {
        /**
         * @brief The pipeline_async_impl class
         * TODO : document
         */
        class pipeline_async_impl : public pipeline_async_interface
        {
        public:
            pipeline_async_impl(const char * playback_file_path = nullptr);
            virtual status add_cv_module(video_module_interface * cv_module) override;
            virtual status query_cv_module(uint32_t index, video_module_interface ** cv_module) const override;
            virtual status query_available_config(uint32_t index, pipeline_async_interface::pipeline_config & available_config) const override;
            virtual status set_config(const pipeline_async_interface::pipeline_config & pipeline_config) override;
            virtual status query_current_config(pipeline_async_interface::pipeline_config & current_pipeline_config) const override;
            virtual status reset() override;
            virtual status start(callback_handler * app_callbacks_handler) override;
            virtual status stop() override;
            virtual ~pipeline_async_impl();
        private:
            enum class state
            {
                unconfigured,
                configured,
                streaming
            };
            state m_current_state;
            mutable std::mutex m_state_lock;

            std::unique_ptr<context_interface> m_context;

            std::vector<video_module_interface *> m_cv_modules;
            std::map<video_module_interface *,
                     std::pair<video_module_interface::actual_module_config,
                               video_module_interface::supported_module_config>> m_modules_configs;
            pipeline_async_interface::pipeline_config m_pipeline_config;

            std::mutex m_samples_consumers_lock;
            std::vector<std::shared_ptr<samples_consumer_base>> m_samples_consumers;
            std::unique_ptr<streaming_device_manager> m_streaming_device_manager;

            status get_intersecting_modules_config(std::vector<pipeline_async_interface::pipeline_config> & intersecting_modules_configs) const;
            bool is_theres_satisfying_module_config(video_module_interface * cv_module,
                                                    const video_module_interface::actual_module_config & given_config,
                                                    video_module_interface::actual_module_config &satisfying_config,
                                                    video_module_interface::supported_module_config &satisfying_config_extended_info);
            std::vector<std::tuple<video_module_interface *,
                                   video_module_interface::actual_module_config,
                                   video_module_interface::supported_module_config>> get_cv_module_configurations();

            void non_blocking_set_sample(std::shared_ptr<correlated_sample_set> sample_set);
            void resources_reset();
        };
    }
}

