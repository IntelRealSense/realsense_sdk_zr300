// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once

namespace rs
{
    namespace core
    {
        /**
         * @brief The release_interface class
         * the release_interface provides an abstract way to release memory.
         * an inheriting class should be destructed through a release call instead of deleting it.
         */
        class release_interface
        {
        public:
            virtual int release() const = 0;
        protected:
            //force deletion using the release function
            virtual ~release_interface() {}
        };
    }
}
