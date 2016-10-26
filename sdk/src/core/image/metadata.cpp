// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <mutex>
#include <cstring>
#include "metadata.h"

namespace rs
{
    namespace core
    {
        bool metadata::is_metadata_available(metadata_type id) const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return exists(id);
        }

        uint32_t metadata::query_buffer_size(metadata_type id) const
        {
            return get_metadata(id, nullptr);
        }

        uint32_t metadata::get_metadata(metadata_type id, uint8_t* buffer) const
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if(!exists(id))
            {
                return 0;
            }

            const std::vector<uint8_t>& md = m_data.at(id);
            int32_t buffer_size = static_cast<int32_t>(md.size());

            if(buffer != nullptr)
            {
                std::memcpy(buffer, md.data(), buffer_size);
            }

            return buffer_size;
        }

        status metadata::add_metadata(metadata_type id, const uint8_t* buffer, uint32_t size)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if(buffer == nullptr)
            {
                return status_handle_invalid;
            }

            if(size == 0)
            {
                return status_invalid_argument;
            }

            if(exists(id))
            {
                return status_key_already_exists;
            }

            m_data.emplace(id, std::vector<uint8_t>(buffer, buffer + size));
            return status::status_no_error;
        }

        status metadata::remove_metadata(metadata_type id)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if(!exists(id))
            {
                return status::status_item_unavailable;
            }

            m_data.erase(id);
            return status::status_no_error;
        }

        bool metadata::exists(metadata_type id) const
        {
           return m_data.find(id) != m_data.end();
        }
    }
}


