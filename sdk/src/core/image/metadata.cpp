// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "metadata.h"

namespace rs
{
    namespace core
    {    
        int32_t metadata::query_metadata(int32_t idx) const
        {
            return 0;
        }

        void metadata::detach_metadata(int32_t id)
        {

        }

        status metadata::attach_buffer(int32_t id, uint8_t *buffer, int32_t size)
        {
            return status_feature_unsupported;
        }

        int32_t metadata::query_buffer_size(int32_t id)
        {
            return 0;
        }

        status metadata::query_buffer(int32_t id, uint8_t *buffer, int32_t size)
        {
            return status_feature_unsupported;
        }

        metadata::~metadata() {}

        metadata_interface * metadata_interface::create_instance()
        {
            return new metadata();
        }
    }
}


