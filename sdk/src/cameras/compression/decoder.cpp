// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "decoder.h"
#include "lz4_codec.h"
#include "rs/utils/log_utils.h"

namespace rs
{
    namespace core
    {
        namespace compression
        {

            decoder::decoder(std::map<rs_stream,file_types::compression_type> configuration)
            {
                for(auto config : configuration)
                {
                    add_codec(config.first, config.second);
                }
            }

            decoder::~decoder()
            {

            }

            void decoder::add_codec(rs_stream stream_type, file_types::compression_type compression_type)
            {
                if(m_codecs.find(stream_type) != m_codecs.end()) return;
                auto & codec = m_codecs[stream_type];
                switch (compression_type)
                {
                    case file_types::compression_type::lz4: codec   = std::shared_ptr<codec_interface>(new lz4_codec()); break;
                    default: codec                                  = nullptr; break;
                }
            }

            std::shared_ptr<file_types::frame_sample> decoder::decode_frame(std::shared_ptr<file_types::frame_sample> frame, uint8_t *input, uint32_t input_size)
            {
                LOG_SCOPE();

                return m_codecs[frame->finfo.stream]->decode(frame, input, input_size);
            }
        }
    }
}
