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
            bool is_metadata_available(image_metadata id) const override;
            int32_t query_buffer_size(image_metadata id) const override;
            status copy_metadata_buffer(image_metadata id, uint8_t* buffer, int32_t buffer_size) const override;
            status add_metadata(image_metadata id, uint8_t* buffer, int32_t size) override;
            status remove_metadata(image_metadata id) override;
        private:
            bool exists(image_metadata id) const;

            std::map<image_metadata, std::vector<uint8_t>> m_data;
            mutable std::mutex m_mutex;
        };
    }
}
