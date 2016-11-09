// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense/rs.hpp>
#include <librealsense/rscore.hpp>

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
            virtual void set_compression(rs_stream stream, bool enable, float compression_level) = 0;
        };
    }
}
