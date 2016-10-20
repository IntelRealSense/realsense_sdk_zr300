// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "status.h"
#include "stdint.h"
#include "types.h"

namespace rs
{
    namespace core
    {
        class metadata_interface
        {
        public:
            /**
            @brief The function checks if the metadata is available for the current image
            @param[in] id           The metadata identifier
            @return True if the image contains the requested metadata
            */
            virtual bool is_metadata_available(metadata_type id) const = 0;

            /**
            @brief The function returns the specified metadata buffer size.
            @param[in] id               The metadata identifier.
            @return The metadata buffer size, or zero if the metadata is not available.
            */
            virtual int32_t query_buffer_size(metadata_type id) const = 0;

            /**
            @brief The function copies the specified metadata to the input buffer
            @brief If buffer is null, the function returns the required size of the buffer, and buffer remains null
            @brief If buffer is not null, the buffer will contain a copy of the metadata
            @param[in]  id               The metadata identifier.
            @param[out] buffer           The buffer pointer to which the metadata should be copied.
            @return The size of the buffer for the requested id
            */
            virtual int32_t get_metadata(metadata_type id, uint8_t* buffer) const = 0;

            /**
            @brief The function adds the specified metadata.
            @param[in] id           The metadata identifier.
            @param[in] buffer       The metadata buffer.
            @param[in] size         The metadata buffer size, in bytes.
            @return status_invalid_argument     The metadata identifier already exists
            @return status_handle_invalid       The buffer is invalid
            @return status_buffer_too_small     Buffer size is smaller than or equals to 0
            @return status_no_error             Successful execution.
            */
            virtual status add_metadata(metadata_type id, uint8_t* buffer, int32_t size) = 0;

            /**
            @brief The function removes the specified metadata.
            @param[in] id           The metadata identifier.
            @return status_no_error                Successful execution.
            @return status_item_unavailable        The requested identifier is not found.
            */
            virtual status remove_metadata(metadata_type id) = 0;

            virtual ~metadata_interface() {}
        };
    }
}
