// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <vector>
#include "lz4_codec.h"
#include "rs/utils/log_utils.h"
#include "lz4.h"

namespace rs
{
    namespace core
    {
        namespace compression
        {

            lz4_codec::lz4_codec() : m_compression_level(0)
            {

            }

            lz4_codec::lz4_codec(record::compression_level compression_level) : m_compression_level(0)
            {
                switch (compression_level)
                {
                    case record::compression_level::low: m_compression_level = 100; break;
                    case record::compression_level::medium:  m_compression_level = 17; break;
                    case record::compression_level::high: m_compression_level = 0; break;
                    default: m_compression_level = 0; break;
                }
            }

            lz4_codec::~lz4_codec(void)
            {
                LOG_FUNC_SCOPE();
            }

            std::shared_ptr<file_types::frame_sample> lz4_codec::decode(std::shared_ptr<file_types::frame_sample> frame, uint8_t * input, uint32_t input_size)
            {
                LOG_FUNC_SCOPE();

                auto rv = std::shared_ptr<file_types::frame_sample>(
                new file_types::frame_sample(frame.get()), [](file_types::frame_sample* f){delete[] f->data; delete f;});

                int frame_size = frame->finfo.stride * frame->finfo.height;
                auto data = new uint8_t[frame_size];
                auto read = LZ4_decompress_fast (reinterpret_cast<char*>(input), reinterpret_cast<char*>(data), frame_size);
                rv->data = data;
                return rv;
            }

            status lz4_codec::encode(file_types::frame_info &info, const uint8_t * input, uint8_t * output, uint32_t &output_size)
            {
                LOG_FUNC_SCOPE();

                if (!input)
                {
                    LOG_ERROR("input data is null");
                    return status::status_process_failed;
                }

                int input_size = info.stride * info.height;
                output_size = LZ4_compress_fast(reinterpret_cast<const char*>(input), reinterpret_cast<char*>(output), input_size, input_size, m_compression_level);
                return status::status_no_error;
            }
        }
    }
}
