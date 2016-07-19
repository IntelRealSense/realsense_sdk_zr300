// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <memory>
#include <vector>
#include <librealsense/rs.hpp>
#include "rs/core/status.h"
#include "include/file_types.h"

namespace rs
{
    namespace core
    {
        class compression_interface
        {
        public:
            compression_interface() {}
            virtual ~compression_interface(void) {}
            virtual file_types::compression_type compression_policy(rs_stream stream) = 0;
            virtual status encode_image(file_types::compression_type type, std::shared_ptr<file_types::frame_sample> &frame, std::vector<uint8_t> &buffer) = 0;
            virtual status decode_image(file_types::compression_type type, std::shared_ptr<file_types::frame_sample> &frame, std::vector<uint8_t> &buffer) = 0;
        };
    }
}
