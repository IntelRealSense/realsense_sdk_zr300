// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <map>
#include <memory>
#include <tuple>
#include <librealsense/rs.hpp>
#include "codec_interface.h"

namespace rs
{
    namespace core
    {
        namespace compression
        {
            class encoder
            {
            public:
                encoder(std::vector<std::tuple<rs_stream, rs_format, bool, float>> configuration);
                ~encoder();

                uint8_t * encode_frame(file_types::frame_info &info, const uint8_t * input, uint32_t &output_size);
                file_types::compression_type get_compression_type(rs_stream stream);

            private:
                file_types::compression_type compression_policy(rs_stream stream, rs_format format);
                void add_codec(rs_stream stream, rs_format format, float compression_level);
                std::map<rs_stream,std::shared_ptr<codec_interface>> m_codecs;
            };
        }
    }
}
