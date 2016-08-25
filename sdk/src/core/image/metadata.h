// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs/utils/ref_count_base.h"
#include "rs/core/metadata_interface.h"

namespace rs
{
    namespace core
    {
        /**
         * @brief The metadata class
         * see complete metadata documantation in the interface declaration.
         */
        class metadata : public rs::utils::ref_count_base<metadata_interface>
        {
        public:
            int32_t query_metadata(int32_t idx) const;
            void detach_metadata(int32_t id);
            status attach_buffer(int32_t id, uint8_t *buffer, int32_t size);
            int32_t query_buffer_size(int32_t id);
            status query_buffer(int32_t id, uint8_t *buffer, int32_t size);
        protected:
            virtual ~metadata();
        };
    }
}
