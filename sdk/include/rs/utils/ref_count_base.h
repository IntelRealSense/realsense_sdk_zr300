// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs/core/ref_count_interface.h"
#include <atomic>

namespace rs
{
    namespace utils
    {
         /**
         * @brief The ref_count_base class
         * The ref_count_base class implements atomic reference counting operations.
         */
        template<typename T>
        class ref_count_base : public T
        {
        public:
            ref_count_base(const ref_count_base<T> &) = delete;
            ref_count_base& operator= (const ref_count_base<T> &) = delete;
            ref_count_base(ref_count_base<T> &&) = delete;
            ref_count_base& operator= (ref_count_base<T> &&) = delete;

            ref_count_base() : m_ref_count(1) { }

            virtual int add_ref() const override
            {
                return ++m_ref_count;
            }

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
