// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense/rs.hpp>
#include <librealsense/rscore.hpp>
#include "rs/record/record_device.h"

namespace rs
{
    namespace record
    {
        class device_interface : public rs_device
        {
        public:
            virtual ~device_interface() {}
            virtual void pause_record() = 0;
            virtual void resume_record() = 0;
            virtual bool set_compression(rs_stream stream, record::compression_level compression_level) = 0;
            virtual record::compression_level get_compression(rs_stream stream) = 0;
        };
    }
}
