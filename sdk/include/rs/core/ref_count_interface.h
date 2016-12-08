// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/** 
* \file ref_count_interface.h
* @brief Describes the \c rs::core::ref_count_interface class.
*/

#pragma once
#include "rs/core/release_interface.h"

namespace rs
{
    namespace core
    {
        /**
        * @brief Provides an ABI-safe interface extension for inheriting classes.
        *
        * Classes that inherit from \c ref_count_interface are restricted to call \c rs::core::ref_count_interface::add_ref() when the object is shared
        * across library boundaries and release it when it stops using it.
        * The interface is completely \c const to allow \c const inheriting objects to change the reference count.
        */
        class ref_count_interface : public release_interface
        {
        public:
            /**
             * @brief Adds +1 to the object reference count.
             * @return int Reference count
             */
            virtual int add_ref() const = 0;

            /**
             * @brief Gets the current object reference count.
             * @return int Reference count
             */
            virtual int ref_count() const = 0;
        protected:
            //force deletion using the release function
            virtual ~ref_count_interface() {}
        };
    }
}
