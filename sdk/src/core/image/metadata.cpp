// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <mutex>
#include <cstring>
#include "metadata.h"

namespace rs
{
    namespace core
    {
        bool metadata::is_metadata_available(image_metadata id) const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return exists(id);
        }

        int32_t metadata::query_buffer_size(image_metadata id) const
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if(!exists(id))
            {
                return 0;
            }

            auto buffer_size = m_data.at(id).size() * sizeof(std::vector<uint8_t>::value_type);
            return static_cast<int32_t>(buffer_size);
        }

        status metadata::copy_metadata_buffer(image_metadata id, uint8_t* buffer, int32_t buffer_size) const
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if(!exists(id))
            {
                return status_item_unavailable;
            }

            if(buffer == nullptr)
            {
                return status_handle_invalid;
            }

            const std::vector<uint8_t>& md = m_data.at(id);
            int32_t md_size = static_cast<int32_t>(md.size() * sizeof(std::vector<uint8_t>::value_type));

            if(buffer_size < md_size)
            {
                return status_buffer_too_small;
            }

            std::memcpy(buffer, md.data(), buffer_size);
            return status_no_error;

        }

        status metadata::add_metadata(image_metadata id, uint8_t* buffer, int32_t size)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if(exists(id))
            {
                return status_invalid_argument;
            }

            if(buffer == nullptr)
            {
                return status_handle_invalid;
            }

            if(size <= 0)
            {
                return status_buffer_too_small;
            }

            m_data.emplace(id, std::vector<uint8_t>(buffer, buffer + size));
            return status::status_no_error;
        }

        status metadata::remove_metadata(image_metadata id)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if(!exists(id))
            {
                return status::status_item_unavailable;
            }

            m_data.erase(id);
        }

        bool metadata::exists(image_metadata id) const
        {
           return m_data.find(id) != m_data.end();
        }
    }
}


