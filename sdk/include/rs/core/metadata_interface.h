// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "status.h"
#include "stdint.h"

namespace rs
{
    namespace core
    {
        class metadata_interface
        {
        public:
            /**
            @brief The function retrieves the identifiers of all available meta data.
            @param[in] idx          The zero-based index to retrieve all identifiers.
            @return the metadata identifier, or zero if not available.
            */
            virtual int32_t query_metadata(int32_t idx) const = 0;

            /**
            @brief The function detaches the specified metadata.
            @param[in] id           The metadata identifier.
            @return PXC_STATUS_NO_ERROR                Successful execution.
            @return PXC_STATUS_ITEM_UNAVAILABLE        The metadata is not found.
            */
            virtual void detach_metadata(int32_t id) = 0;

            /**
            @brief The function attaches the specified metadata.
            @param[in] id           The metadata identifier.
            @param[in] buffer       The metadata buffer.
            @param[in] size         The metadata buffer size, in bytes.
            @return PXC_STATUS_NO_ERROR                Successful execution.
            */
            virtual status attach_buffer(int32_t id, uint8_t * buffer, int32_t size) = 0;

            /**
            @brief The function returns the specified metadata buffer size.
            @param[in] id           The metadata identifier.
            @return the metadata buffer size, or zero if the metadata is not available.
            */
            virtual int32_t query_buffer_size(int32_t id) = 0;

            /**
            @brief The function retrieves the specified metadata.
            @param[in] id           The metadata identifier.
            @param[in] buffer       The buffer pointer to retrieve the metadata.
            @param[in] size         The buffer size in bytes.
            @return PXC_STATUS_NO_ERROR         Successful execution.
            */
            virtual status query_buffer(int32_t id, uint8_t * buffer, int32_t size) = 0;

            virtual ~metadata_interface() {}

            static metadata_interface * create_instance();
        };
    }
}
