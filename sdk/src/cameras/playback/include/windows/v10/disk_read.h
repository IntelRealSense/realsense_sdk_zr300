// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "disk_read_base.h"

namespace rs
{
    namespace playback
    {
        namespace windows
        {
            namespace v10
            {
                class disk_read : public disk_read_base
                {

                public:
                    disk_read(const char *file_name) : disk_read_base(file_name), m_time_stamp_base(0) {}
                    virtual ~disk_read(void);

                protected:
                    virtual rs::core::status read_headers() override;
                    virtual void index_next_samples(uint32_t number_of_samples) override;
                    virtual int32_t size_of_pitches(void) override;
                    void handle_ds_projection(std::vector<uint8_t> &projection_data);
                    rs::core::status get_image_offset(rs_stream stream, int64_t & offset);
                    virtual uint32_t read_frame_metadata(const std::shared_ptr<core::file_types::frame_sample> & frame, unsigned long num_bytes_to_read) override;
                private:
                    uint64_t m_time_stamp_base;
                };
            }
        }
    }
}
