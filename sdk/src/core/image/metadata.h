// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs/utils/ref_count_base.h"
#include "rs/core/metadata_interface.h"
#include <map>
#include <mutex>
#include <vector>
namespace rs
{
    namespace core
    {
        /**
         * @brief The metadata class
         * see complete metadata documantation in the interface declaration.
         */
        class metadata : public metadata_interface
        {
        public:
            metadata() = default;
            virtual ~metadata() = default;
            bool is_metadata_available(metadata_type id) const override;
            uint32_t query_buffer_size(metadata_type id) const override;
            uint32_t get_metadata(metadata_type id, uint8_t* buffer) const override;
            status add_metadata(metadata_type id, uint8_t* buffer, uint32_t size) override;
            status remove_metadata(metadata_type id) override;
        private:
            bool exists(metadata_type id) const;

            std::map<metadata_type, std::vector<uint8_t>> m_data;
            mutable std::mutex m_mutex;
        };
    }
}
