// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <vector>
#include <functional>
#include <memory>
#include <mutex>
#include <librealsense/rs.hpp>

#include "rs/core/pipeline_async_interface.h"

#include "pipeline_common.h"
#include "samples_consumer_base.h"
#include "lrs_manager.h"

namespace rs
{
    namespace core
    {
        /**
         * @brief The pipeline_async_impl class
         * TODO : document
         */
        class pipeline_async_impl : public pipeline_async_interface,
                                    public pipeline_common
        {
        public:
            pipeline_async_impl(const char * playback_file_path = nullptr);
            virtual status start(callback_handler * app_callbacks_handler) override;
            virtual status stop() override;
            virtual status reset() override;
            virtual ~pipeline_async_impl();
        private:
            std::mutex samples_consumers_lock;

            rs::device * m_device;
            rs::source m_active_sources;
            std::map<stream_type, std::function<void(rs::frame)>> m_stream_callback_per_stream;
            std::function<void(rs::motion_data)> m_motion_callback;

            std::vector<std::shared_ptr<samples_consumer_base>> m_samples_consumers;

            void non_blocking_set_sample(std::shared_ptr<correlated_sample_set> sample_set);
        };
    }
}

