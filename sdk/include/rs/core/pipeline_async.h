// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/** 
* \file pipeline_async.h
* @brief Describes the \c rs::core::pipeline_async and \c rs::core::pipeline_async_impl classes.
*/

#pragma once
#include "rs/core/pipeline_async_interface.h"

#ifdef WIN32
#ifdef realsense_pipeline_EXPORTS
#define  DLL_EXPORT __declspec(dllexport)
#else
#define  DLL_EXPORT __declspec(dllimport)
#endif /* realsense_pipeline_EXPORTS */
#else /* defined (WIN32) */
#define DLL_EXPORT
#endif

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
        class DLL_EXPORT pipeline_async : public pipeline_async_interface
        {
        public:
            /**
             * @brief Constructor to initialize a pipeline async interface with a live camera, currently connected to the platform.
             */
            pipeline_async();

            /**
             * @brief The testing_mode enum expresses the pipeline testing modes for record and playback.
             */
            enum class testing_mode
            {
                playback,   /** The streaming source will be a playback file */
                record      /** The streaming source will be a device which is currently connected to the platform, and the streaming output will
                                be recorded to a file */
            };

            /**
             * @brief Constructor to initialize a pipeline for testing using record and playback.
             *
             * @param[in] mode            Select the pipeline testing mode, streaming from a playback file or record mode, which streams from a live camera
             *                            and records the output to a file.
             * @param[in] file_path       The input file path for playback mode or record mode output file path.
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
            pipeline_async_impl * m_pimpl; /**<The actual pipeline asynchronous implementation. */
        };
    }
}

