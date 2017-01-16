// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/** 
* \file ref_count_base.h
* @brief Describes the \c rs::utils::ref_count_base template.
**/ 

#pragma once
#include <atomic>

namespace rs
{
    namespace utils
    {
         /**
         * @brief Implements atomic reference counting operations.
         */
        template<typename T>
        class ref_count_base : public T
        {
        public:
            ref_count_base(const ref_count_base<T> &) = delete;
            ref_count_base& operator= (const ref_count_base<T> &) = delete;
            ref_count_base(ref_count_base<T> &&) = delete;
            ref_count_base& operator= (ref_count_base<T> &&) = delete;

            /**
             * @brief Constructor: Any reference counted inheriting class is initialized with 1.
             */
            ref_count_base() : m_ref_count(1) {}

            /**
             * @brief Increments the reference count by 1.
             * @return Reference count after the method operation
             */
            virtual int add_ref() const override
            {
                return ++m_ref_count;
            }

            /**
             * @brief Decrements the reference count by 1; if this is the last instance, deletes this instance.
             * @return Reference count after the method operation
             */
            virtual int release() const override
            {
                int post_fetched_ref_count = --m_ref_count;
                if(post_fetched_ref_count == 0)
                {
                   delete(this);
                   return 0; // must return after the object's memory returned to the os.
                }
                return post_fetched_ref_count;
            }

            /**
             * @brief Gets the current reference count.
             * @return int Count
             */
            virtual int ref_count() const override
            {
                return m_ref_count;
            }
        protected:
            virtual ~ref_count_base() {}
        private:
            mutable std::atomic<int> m_ref_count;
        };
    }
}
