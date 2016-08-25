// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once

namespace rs
{
    namespace utils
    {
        /**
         * @brief The release_self_base class
         * the release_self_base class provides ABI safe release operation for single non ref counted object.
         * calling to release will delete the object from the context of the intializing side. This removes the
         * need to supplay additional ABI safe object deleter function.
         */
        template<typename T>
        class release_self_base : public T
        {
        public:
            virtual int release() const override
            {
                delete(this);
                return 0;
            }
        protected:
            virtual ~release_self_base () {}
        };
    }
}
