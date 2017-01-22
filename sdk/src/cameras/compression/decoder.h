// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <map>
#include <memory>
#include <librealsense/rs.hpp>
#include "codec_interface.h"

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
            class DLL_EXPORT decoder
            {
            public:
                decoder(std::map<rs_stream,file_types::compression_type> configuration);
                ~decoder();

                std::shared_ptr<file_types::frame_sample> decode_frame(std::shared_ptr<file_types::frame_sample> frame, uint8_t * input, uint32_t input_size);

            private:
                void add_codec(rs_stream stream_type, file_types::compression_type compression_type);
                std::map<rs_stream,std::shared_ptr<codec_interface>> m_codecs;
            };
        }
    }
}
