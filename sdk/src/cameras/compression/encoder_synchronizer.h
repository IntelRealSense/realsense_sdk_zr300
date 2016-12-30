// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "encoder.h"

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
            class stream_encoder;

            class DLL_EXPORT encoder_synchronizer
            {
            public:
                encoder_synchronizer(std::shared_ptr<encoder> encoder);
                ~encoder_synchronizer();

                void add_stream(rs_stream stream, uint32_t buffer_size, uint32_t concurrency = 2);
                void clear();
                void encode_sample_and_lock(std::shared_ptr<core::file_types::frame_sample> sample);
                uint8_t* get_next_data(rs_stream stream, uint32_t& data_size);
                void release_locked_sample(rs_stream stream);
                file_types::compression_type get_compression_type(rs_stream stream);

            private:
                std::shared_ptr<encoder> m_encoder;
                std::map<rs_stream, std::shared_ptr<stream_encoder>> m_encoders;
                std::mutex m_mutex;
            };
        }
    }
}
