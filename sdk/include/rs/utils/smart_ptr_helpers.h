// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/** 
* \file smart_ptr_helpers.h
* @brief Describes helper methods that provide shared pointers and unique pointers with a customized deleter.
*/
  
#pragma once
#include <memory>

// helper functions providing shared_ptr and unique_ptr with a customized deleter.

namespace rs
{
    namespace utils
    {
        /**
        * @brief Customized deleter which calls release upon the object destruction.
        */
        template<typename T>
        struct releaser
        {
            void operator()(T *p) const { if (p) p->release(); }
        };

        /**
        * @brief
        *  Uses \c unique_ptr under the \c rs::utils namespace, containing a customized deleter which calls the object release method.
        */
        template <typename T>
        using unique_ptr = std::unique_ptr<T, releaser<T>>;


        /**
        * @brief
        *  Wraps the given pointer with \c unique_ptr with the customized deleter.
        */
        template<typename T>
        inline unique_ptr<T> get_unique_ptr_with_releaser(T* object)
        {
            return unique_ptr<T>(object);
        }

        /**
        * @brief
        *  Wraps the given const pointer with \c unique_ptr with the customized deleter.
        */
        template<typename T>
        inline unique_ptr<const T> get_unique_ptr_with_releaser(const T* object)
        {
            return unique_ptr<const T>(object);
        }

        /**
        * @brief
        *  Wraps the given pointer with \c shared_ptr with the customized deleter.
        */
        template<typename T>
        inline std::shared_ptr<T> get_shared_ptr_with_releaser(T* object)
        {
            return std::move(get_unique_ptr_with_releaser(object));
        }

        /**
        * @brief
        *  Wraps the given const pointer with \c shared_ptr with the customized deleter.
        */
        template<typename T>
        inline std::shared_ptr<const T> get_shared_ptr_with_releaser(const T* object)
        {
            return std::move(get_unique_ptr_with_releaser(object));
        }
    }
}
