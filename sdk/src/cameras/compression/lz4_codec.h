// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <thread>
#include <map>
#include <lz4frame.h>
#include "codec_interface.h"

namespace rs
{
    namespace core
    {
        namespace compression
        {
            class lz4_codec : public codec_interface
            {
            public:
                lz4_codec();
                lz4_codec(float compression_level);
                virtual ~lz4_codec();

                virtual status encode(file_types::frame_info &info, const uint8_t * input, uint8_t * output, uint32_t &output_size);
                virtual std::shared_ptr<file_types::frame_sample> decode(std::shared_ptr<file_types::frame_sample> frame, uint8_t * input, uint32_t input_size);
                virtual file_types::compression_type get_compression_type() { return file_types::compression_type::lz4; }
            private:
                LZ4F_compressionContext_t m_comp_context;
                LZ4F_decompressionContext_t m_decomp_context;
                LZ4F_preferences_t m_lz4_preferences;
            };
        }
    }
}
