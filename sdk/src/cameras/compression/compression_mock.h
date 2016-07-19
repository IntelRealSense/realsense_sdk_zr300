// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "compression_interface.h"

namespace rs
{
    namespace core
    {
        class compression : public compression_interface
        {
        public:
            compression() {}
            virtual ~compression(void) {}
            virtual file_types::compression_type compression_policy(rs_stream stream) override { return file_types::compression_type::none; }
            virtual status encode_image(file_types::compression_type type, std::shared_ptr<file_types::frame_sample> &frame, std::vector<uint8_t> &buffer) override { return status_no_error; }
            virtual status decode_image(file_types::compression_type type, std::shared_ptr<file_types::frame_sample> &frame, std::vector<uint8_t> &buffer) override { return status_no_error; }
        };
    }
}
