// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs/core/release_interface.h"
#include <memory>

namespace rs
{
    namespace core
    {
        /**
        * @brief The ref_count_interface class
        * the ref count interface class provides ABI safe interface extension for other classes.
        * classes that inherit the ref_count_interface are restricted to call add_ref when the object is shared
        * across library boundries and release it when it stops using it.
        * the interface is completely const to allow const inheriting objects to change ref count.
        */
        class ref_count_interface : public release_interface
        {
        public:
            virtual int add_ref() const = 0;
            virtual int ref_count() const = 0;
        protected:
            //force deletion using the release function
            virtual ~ref_count_interface() {}
        };
    }
}
