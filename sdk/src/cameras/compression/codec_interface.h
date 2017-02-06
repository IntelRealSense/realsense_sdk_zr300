// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <memory>
#include "rs/core/status.h"
#include "include/file_types.h"

#ifdef WIN32 
#ifdef realsense_compression_EXPORTS
#define  DLL_EXPORT __declspec(dllexport)
#else
#define  DLL_EXPORT __declspec(dllimport)
#endif /* realsense_compression_EXPORTS */
#else /* defined (WIN32) */
#define DLL_EXPORT
#endif

namespace rs
{
    namespace core
    {
        namespace compression
        {
            class DLL_EXPORT codec_interface
            {
            public:
                codec_interface() {}
                virtual ~codec_interface() {}

                virtual file_types::compression_type get_compression_type() = 0;
                virtual status encode(file_types::frame_info &info, const uint8_t * input, uint8_t * output, uint32_t &output_size) = 0;
                virtual std::shared_ptr<file_types::frame_sample> decode(std::shared_ptr<file_types::frame_sample> frame, uint8_t * input, uint32_t input_size) = 0;
            };
        }
    }
}
