// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/** 
* \file release_interface.h
* @brief Describes the \c rs::core::release_interface class.
*/


#pragma once

namespace rs
{
    namespace core
    {
        /**
        * 
        * @brief Provides an abstract way to release the inheriting object memory.
        *
        * An inheriting class should be destructed through a release function call instead of directly deleting it.
        */
        class release_interface
        {
        public:
            /**
            * @brief Releases the object according to its internal logic.
            *
            * @return int Current object reference count if the object is reference counted.
            */
            virtual int release() const = 0;
        protected:
            //force deletion using the release function
            virtual ~release_interface() {}
        };
    }
}
