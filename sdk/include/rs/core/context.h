// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense/rs.hpp>
#include "context_interface.h"

namespace rs
{
    namespace core
    {
        /**
        * @class rs::core::context
        * @brief The rs::core::context provied the same capabilities as the librealsense context.
        *
        * This class implements the context interface for controlling and streaming from a live camera device.        *
        */
        class context : public context_interface
        {
        public:
            context() {}
            virtual ~context() {}

            virtual int get_device_count() const override
            {
                return m_context.get_device_count();
            }

            virtual rs::device * get_device(int index) override
            {
                return m_context.get_device(index);
            }

        protected:
            rs::context m_context;
            context(const context &) = delete;
            context & operator = (const context &) = delete;
        };
    }
}
