// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "status.h"
#include "stdint.h"

namespace rs
{
    namespace core
    {
        enum class metadata_type
        {
            actual_exposure = 0,
            custom          = 0x10000
        };

        /**
        * @brief An interface for accessing an image's metadata storage.
        * This interface is available via the image interface.
        * An image stores a single metadata storage, which includes zero or one metadata item per type.
        */
        class metadata_interface
        {
        public:
            /**
             * @brief The function checks if the specified metadata is available for the current image
             * @param[in] id           The metadata identifier
             * @return True if the image contains the requested metadata
             */
            virtual bool is_metadata_available(metadata_type id) const = 0;

            /**
             * @brief The function returns the specified metadata buffer size.
             * @param[in] id               The metadata identifier.
             * @return The metadata buffer size, or zero if the metadata is not available.
             */
            virtual uint32_t query_buffer_size(metadata_type id) const = 0;

            /**
             * @brief The function copies the specified metadata to buffer.
             * If buffer is null, the function returns the required size of the buffer, and buffer remains null.
             * If buffer is not null, the buffer will contain a copy of the metadata.
             * The function assumes that the buffer size is at least the required size.
             * @param[in]  id               The metadata identifier.
             * @param[out] buffer           The buffer pointer to which the metadata should be copied.
             * @return The size of the buffer for the requested id
             */
            virtual uint32_t get_metadata(metadata_type id, uint8_t* buffer) const = 0;

            /**
             * @brief The function attaches a copy of the input buffer to the metadata storage.
             * If the specified metadata type already exists in the storage, the function fails and the storage does not change.
             * To replace the specified metadata remove_metadata should be called before calling this function for the same specified metadata
             * @param[in] id           The metadata identifier.
             * @param[in] buffer       The metadata buffer.
             * @param[in] size         The metadata buffer size, in bytes.
             * @return status_invalid_argument     The metadata identifier already exists
             * @return status_handle_invalid       The buffer is invalid
             * @return status_buffer_too_small     Buffer size equals to 0
             * @return status_no_error             Successful execution.
             */
            virtual status add_metadata(metadata_type id, uint8_t* buffer, uint32_t size) = 0;

            /**
             * @brief The function removes the specified metadata from the metadata storage.
             * If the specified metadata type doesn't exists in the storage, the function fails and the storage does not change.
             * @param[in] id						The metadata identifier.
             * @return status_no_error				Successful execution.
             * @return status_item_unavailable		The requested identifier is not found.
             */
            virtual status remove_metadata(metadata_type id) = 0;

            virtual ~metadata_interface() = default;
        };
    }
}
