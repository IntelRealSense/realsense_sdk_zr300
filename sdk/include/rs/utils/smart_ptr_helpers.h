// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <memory>

namespace rs
{
    namespace utils
    {
        /**
        * helper functions providing shared_ptr and unique_ptr with a custom releaser
        */
        template<typename T>
        struct releaser
        {
            void operator()(T *p) const { if (p) p->release(); }
        };

        template <typename T>
        using unique_ptr = std::unique_ptr<T, releaser<T>>;

        template<typename T>
        inline unique_ptr<T> get_unique_ptr_with_releaser(T* object)
        {
            return unique_ptr<T>(object);
        }

        template<typename T>
        inline unique_ptr<const T> get_unique_ptr_with_releaser(const T* object)
        {
            return unique_ptr<const T>(object);
        }

        template<typename T>
        inline std::shared_ptr<T> get_shared_ptr_with_releaser(T* object)
        {
            return std::move(get_unique_ptr_with_releaser(object));
        }

        template<typename T>
        inline std::shared_ptr<const T> get_shared_ptr_with_releaser(const T* object)
        {
            return std::move(get_unique_ptr_with_releaser(object));
        }
    }
}
