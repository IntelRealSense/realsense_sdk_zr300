// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/** 
* \file pipeline_async.h
* @brief Describes the \c rs::core::pipeline_async and \c rs::core::pipeline_async_impl classes.
*/

#pragma once
#include "rs/core/pipeline_async_interface.h"

namespace rs
{
    namespace core
    {
        /**
        * @brief Forward declaration for the actual pipeline implementation as part of the pimpl pattern.
        */
        class pipeline_async_impl;

        /**
         * @brief Instantiation class, as part of the pimpl pattern for \c pipeline_async_interface.
         *
         * For complete class documentation, see \c pipeline_async_interface.
         */
        class pipeline_async : public pipeline_async_interface
        {
        public:
            /**
             * @brief Constructor to initialize a pipeline async interface.
             *
             * @param[in] playback_file_path Path to playback file
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
            pipeline_async_impl * m_pimpl; /**<The actual pipeline async implementation. */
        };
    }
}

