// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "disk_read_base.h"

namespace rs
{
    namespace playback
    {
        class disk_read : public disk_read_base
        {
        public:
            disk_read(const char *file_name) : disk_read_base(file_name) {}
            virtual ~disk_read(void);
        protected:
            virtual rs::core::status read_headers() override;
            virtual void index_next_samples(uint32_t number_of_samples) override;
            virtual int32_t size_of_pitches(void) override;
            virtual uint32_t read_frame_metadata(const std::shared_ptr<core::file_types::frame_sample> & frame, unsigned long num_bytes_to_read) override;
        };
    }
}
