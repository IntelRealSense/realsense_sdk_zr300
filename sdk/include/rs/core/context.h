// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense/rs.hpp>

namespace rs
{
    namespace core
    {
        /**
        For Context documentation see rs.hpp(librealsense).
        */
        class context
        {
        protected:
            rs_context * handle;
            context(const context &) = delete;
            context & operator = (const context &) = delete;
        public:
            context()
            {
                rs_error * e = nullptr;
                handle = rs_create_context(7, &e);
                rs::error::handle(e);
            }

            virtual ~context()
            {
                rs_delete_context(handle, nullptr);
            }

            virtual int get_device_count() const
            {
                rs_error * e = nullptr;
                auto r = rs_get_device_count(handle, &e);
                rs::error::handle(e);
                return r;
            }

            virtual rs::device * get_device(int index)
            {
                rs_error * e = nullptr;
                auto r = rs_get_device(handle, index, &e);
                rs::error::handle(e);
                return (rs::device *)r;
            }
        };
    }
}
