// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <map>
#include <memory>
#include <tuple>
#include <librealsense/rs.hpp>
#include "codec_interface.h"
#include "rs/record/record_device.h"

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
            class DLL_EXPORT encoder
            {
            public:
                encoder();
                ~encoder();

                status encode_frame(file_types::frame_info &info, const uint8_t * input, uint8_t * output, uint32_t &output_size);
                file_types::compression_type get_compression_type(rs_stream stream);
                void add_codec(rs_stream stream, rs_format format, record::compression_level compression_level);

            private:
                file_types::compression_type compression_policy(rs_stream stream, rs_format format);
                std::map<rs_stream,std::shared_ptr<codec_interface>> m_codecs;
            };
        }
    }
}
