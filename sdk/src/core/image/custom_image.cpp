// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "custom_image.h"

using namespace rs::utils;

namespace rs
{
    namespace core
    {
        custom_image::custom_image(image_info * info,
                                   const void * data,
                                   stream_type stream,
                                   image_interface::flag flags,
                                   uint64_t time_stamp,
                                   smart_ptr<metadata_interface> metadata,
                                   smart_ptr<data_releaser_interface> data_releaser)
            : image_base(metadata), m_info(*info), m_data(data), m_stream(stream), m_flags(flags),
              m_time_stamp(time_stamp), m_data_releaser(data_releaser)
        {

        }

        image_info custom_image::query_info() const
        {
            return m_info;
        }

        uint64_t custom_image::query_time_stamp() const
        {
            return m_time_stamp;
        }

        image_interface::flag custom_image::query_flags() const
        {
            return m_flags;
        }

        const void * custom_image::query_data() const
        {
            return m_data;
        }

        stream_type custom_image::query_stream_type() const
        {
            return m_stream;
        }

        custom_image::~custom_image()
        {
            if(m_data_releaser != nullptr)
            {
                m_data_releaser->release();
            }
        }
    }
}


