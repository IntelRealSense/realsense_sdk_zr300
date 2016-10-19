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
            virtual status query_default_config(uint32_t index, video_module_interface::supported_module_config & default_config) const override;
            virtual status set_config(const video_module_interface::supported_module_config & config) override;
            virtual status query_current_config(video_module_interface::actual_module_config & current_config) const override;
            virtual status reset() override;
            virtual status start(callback_handler * app_callbacks_handler) override;
            virtual status stop() override;
            virtual rs::device * get_device() override;

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
            std::mutex m_samples_consumers_lock;
            std::unique_ptr<context_interface> m_context;
            std::vector<video_module_interface *> m_cv_modules;
            rs::device * m_device;
            rs::utils::unique_ptr<projection_interface> m_projection;
            std::map<video_module_interface *, std::tuple<video_module_interface::actual_module_config,
                                                          bool,
                                                          video_module_interface::supported_module_config::time_sync_mode>> m_modules_configs;
            video_module_interface::actual_module_config m_actual_pipeline_config;
            video_module_interface::supported_module_config::time_sync_mode m_user_requested_time_sync_mode;
            std::vector<std::shared_ptr<samples_consumer_base>> m_samples_consumers;
            std::unique_ptr<streaming_device_manager> m_streaming_device_manager;

            void non_blocking_sample_callback(std::shared_ptr<correlated_sample_set> sample_set);
            void resources_reset();
            rs::device * get_device_from_config(const video_module_interface::supported_module_config & config) const;
            bool is_there_a_satisfying_module_config(video_module_interface * cv_module,
                                                     const video_module_interface::supported_module_config & given_config,
                                                     video_module_interface::supported_module_config &satisfying_config) const;
            bool is_there_a_satisfying_device_mode(rs::device * device,
                                                   const video_module_interface::supported_module_config& given_config) const;
            status enable_device_streams(rs::device * device,
                                    const video_module_interface::supported_module_config& given_config);
            const video_module_interface::supported_module_config get_hardcoded_superset_config() const;
            status set_config_unsafe(const video_module_interface::supported_module_config & config);
            status set_minimal_supported_configuration();
            const video_module_interface::actual_module_config create_actual_config_from_supported_config(
                    const video_module_interface::supported_module_config & supported_config,
                    device *device) const;

        };
    }
}

