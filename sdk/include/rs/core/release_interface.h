// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once

namespace rs
{
    namespace core
    {
        /**
         * @class release_interface
         * @brief release_interface provides an abstract way to release the inheriting object memory.
         *
         * an inheriting class should be destructed through a release function call instead of directly deleting it.
         */
        class release_interface
        {
        public:
            /**
             * @brief release the object according to its internal logic
             *
             * @return the current object referece count if the object is reference counted.
             */
            virtual int release() const = 0;
        protected:
            //force deletion using the release function
            virtual ~release_interface() {}
        };
    }
}
