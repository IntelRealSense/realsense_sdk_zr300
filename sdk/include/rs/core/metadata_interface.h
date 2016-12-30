// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/**
* \file metadata_interface.h
* @brief Describes the \c rs::core::metadata_interface class.
*/

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
            actual_fps      = 1,
            custom          = 0x10000
        };

        /**
        * @brief Interface for accessing an image's metadata storage.
        *
        * This interface is available via the image interface.
        * An image stores a single metadata storage, which includes zero or one metadata item per type.
        */
        class metadata_interface
        {
        public:
            /**
             * @brief Checks if the specified metadata is available for the current image.
             *
             * @param[in] id Metadata identifier
             * @return bool True if the image contains the requested metadata
             */
            virtual bool is_metadata_available(metadata_type id) const = 0;

            /**
            * @brief Returns the specified metadata buffer size.
            * @param[in] id Metadata identifier
            * @return uint32_t Metadata buffer size, or zero if the metadata is not available
            */
            virtual uint32_t query_buffer_size(metadata_type id) const = 0;

            /**
             * @brief Copies the specified metadata to a buffer.
             * 
			 * <ul>
             * <li>If buffer is null, the method returns the required size of the buffer, and buffer remains null.
             * <li>If buffer is not null, the buffer will contain a copy of the metadata.
			 * </ul>
             * The method assumes that the buffer size is at least the required size.
             * @param[in]  id       Metadata identifier
             * @param[out] buffer   Buffer pointer to which the metadata should be copied
             * @return uint32_t The size of the buffer for the requested id
             */
            virtual uint32_t get_metadata(metadata_type id, uint8_t* buffer) const = 0;

            /**
             * @brief The method attaches a copy of the input buffer to the metadata storage.
             * 
             * If the specified metadata type already exists in the storage, the method fails and the storage does not change.
             * To replace the specified metadata remove_metadata should be called before calling this method for the same specified metadata
             * @param[in] id        Metadata identifier
             * @param[in] buffer    Metadata buffer
             * @param[in] size      Metadata buffer size, in bytes
             * @return status_key_already_exists   The metadata identifier already exists.
             * @return status_handle_invalid       The buffer is invalid.
             * @return status_invalid_argument     Buffer size equals 0.
             * @return status_no_error             Successful execution
             */
            virtual status add_metadata(metadata_type id, const uint8_t* buffer, uint32_t size) = 0;

            /**
             * @brief Removes the specified metadata from the metadata storage.
             * 
             * If the specified metadata type does not exist in the storage, the method fails and the storage does not change.
             * @param[in] id					Metadata identifier
             * @return status_no_error			Successful execution
             * @return status_item_unavailable  The requested identifier is not found.
             */
            virtual status remove_metadata(metadata_type id) = 0;

            virtual ~metadata_interface() = default;
        };
    }
}
