// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <vector>
#include "lz4_codec.h"
#include "rs/utils/log_utils.h"
#include <iostream>
#define BUF_SIZE (16*1024)
#define LZ4_HEADER_SIZE 19
#define LZ4_FOOTER_SIZE 4

namespace rs
{
    namespace core
    {
        namespace compression
        {

            lz4_codec::lz4_codec() : m_comp_context(nullptr), m_decomp_context(nullptr)
            {
                auto sts = LZ4F_createDecompressionContext(&m_decomp_context, LZ4F_VERSION);
                if (LZ4F_isError(sts))
                {
                    LOG_ERROR("Failed to create context: error " << sts);
                    throw std::runtime_error("failed to create LZ4 decoder context");
                }
            }

            lz4_codec::lz4_codec(float compression_level) : m_comp_context(nullptr), m_decomp_context(nullptr)
            {
                int cl = (int)(compression_level / 100.0 * 16);
                LZ4F_frameInfo_t frameInfo = { LZ4F_max1MB, LZ4F_blockIndependent, LZ4F_noContentChecksum, LZ4F_frame, 0, { 0, 0 } };
                m_lz4_preferences = {
                    frameInfo,
                    cl,                  /* compression level */
                    0,                   /* autoflush */
                    { 0, 0, 0, 0 },      /* reserved, must be set to 0 */
                };
                auto sts = LZ4F_createCompressionContext(&m_comp_context, LZ4F_VERSION);
                if (LZ4F_isError(sts))
                {
                    LOG_ERROR("Failed to create context: error " << sts);
                    throw std::runtime_error("failed to create LZ4 encoder context");
                }
            }

            lz4_codec::~lz4_codec(void)
            {
                LOG_FUNC_SCOPE();
                if(m_comp_context)
                    LZ4F_freeCompressionContext(m_comp_context);
                if(m_decomp_context)
                    LZ4F_freeDecompressionContext(m_decomp_context);
            }

            std::shared_ptr<file_types::frame_sample> lz4_codec::decode(std::shared_ptr<file_types::frame_sample> frame, uint8_t * input, uint32_t input_size)
            {
                LOG_SCOPE();

                size_t size = input_size;
                auto rv = std::shared_ptr<file_types::frame_sample>(
                new file_types::frame_sample(frame.get()), [](file_types::frame_sample* f){delete[] f->data; delete f;});

                size = input_size;
                size_t frame_size = frame->finfo.stride * frame->finfo.height;
                auto data = new uint8_t[frame_size];
                auto sts = LZ4F_decompress(m_decomp_context, data, &frame_size, input, &size, NULL);
                if (LZ4F_isError(sts))
                {
                    LOG_ERROR("Failed to decompress frame: error " << sts);
                    delete[] data;
                    return nullptr;
                }
                rv->data = data;
                return rv;
            }

            status lz4_codec::encode(file_types::frame_info &info, const uint8_t * input, uint8_t * output, uint32_t &output_size)
            {
                LOG_FUNC_SCOPE();

                size_t n, count_out, offset = 0;

                if (!input) {
                    LOG_ERROR("Not enough memory");
                    return status::status_process_failed;
                }

                size_t input_size = info.stride * info.height;
                auto size = LZ4F_compressBound(input_size, &m_lz4_preferences) + LZ4_HEADER_SIZE + LZ4_FOOTER_SIZE;

                if (!output)
                {
                    LOG_ERROR("Not enough memory");
                    return status::status_process_failed;
                }

                n = offset = count_out = LZ4F_compressBegin(m_comp_context, output, size, &m_lz4_preferences);
                if (LZ4F_isError(n))
                {
                    LOG_ERROR("Failed to start compression: error " << n);
                    return status::status_process_failed;
                }

                n = LZ4F_compressUpdate(m_comp_context, output + offset, size - offset, input, input_size, NULL);
                if (LZ4F_isError(n))
                {
                    LOG_ERROR("Compression failed: error " << n);
                    return status::status_process_failed;
                }

                output_size = (uint32_t)LZ4F_compressEnd(m_comp_context, output + offset, size - offset, NULL);
                if (LZ4F_isError(n))
                {
                    printf("Failed to end compression: error %zu", n);
                    return status::status_process_failed;
                }
                output_size += (uint32_t)offset;

                return status::status_no_error;
            }
        }
    }
}
