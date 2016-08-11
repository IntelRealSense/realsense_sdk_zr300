// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once

namespace rs
{
    namespace utils
    {
        /**
        *@class smart_ptr
        * a reference counted pointer template class implementation without the standard library
        * due to dynamicly loaded module complied with different compilers and memory de\allocation
        * across shared objects.
        */
        template <typename T>
        class smart_ptr
        {
        public:
            smart_ptr() noexcept : m_object(nullptr),
                m_reference_count(nullptr) {}

            smart_ptr(decltype(nullptr)) noexcept : smart_ptr() {}

            //explicit ensures also no nullptr object => always need to allocate ref count
            explicit smart_ptr(T * object) : m_object(object),
                m_reference_count(new int(1)) {}

            smart_ptr(const smart_ptr<T>& other) noexcept : m_object(other.m_object),
                m_reference_count (other.m_reference_count)
            {
                if(m_reference_count)
                {
                    add_ref();
                }
            }

            smart_ptr(smart_ptr<T>&& other) noexcept : m_object(other.m_object),
                m_reference_count(other.m_reference_count)
            {
                other.m_object = nullptr;
                other.m_reference_count = nullptr;
            }

            /**
             * templated K smart_ptr friend class and alias constructor allows assignments of
             * smart ptr with derived type into a smart ptr of base type.
             */
            template<typename K> friend class smart_ptr;
            template <typename K>
            smart_ptr(const smart_ptr<K>& other) : m_object(other.m_object),
                                             m_reference_count(other.m_reference_count)
            {
                if(m_reference_count)
                {
                    add_ref();
                }
            }

            smart_ptr<T>& operator=(const smart_ptr<T> & other) noexcept
            {
                if(this == &other)
                    return *this;

                reset();
                m_object = other.m_object;
                m_reference_count = other.m_reference_count;
                if(m_reference_count  != nullptr)
                {
                    add_ref();
                }
                return *this;
            }

            smart_ptr<T>& operator=(smart_ptr<T>&& other) noexcept
            {
                if(this == &other)
                    return *this;

                reset();

                m_object = other.m_object;
                other.m_object = nullptr;

                m_reference_count = other.m_reference_count;
                other.m_reference_count = nullptr;

                return *this;
            }

            void reset() noexcept
            {
                if(m_reference_count != nullptr && decrement_ref() == 0)
                {
                    delete m_reference_count;
                    if(m_object != nullptr)
                    {
                        delete m_object;
                    }
                }

                m_reference_count = nullptr;
                m_object  = nullptr;
            }

            void reset(T * other)
            {
                if(m_object == other)
                    return;

                reset();
                m_object = other;

                m_reference_count = new int(1);
            }

            T& operator*() const
            {
                return *m_object;
            }

            T* get() const noexcept
            {
                return m_object;
            }

            T* operator->() const noexcept
            {
                return m_object;
            }

            explicit operator bool() const noexcept
            {
                return m_object != nullptr;
            }

            bool operator==(const smart_ptr<T> &other) const noexcept
            {
                return m_object == other.m_object;
            }

            bool operator==(const T & other) const noexcept
            {
                return m_object == other;
            }

            bool operator!=(const smart_ptr<T> &other) const noexcept
            {
                return !(m_object == other.m_object);
            }

            bool operator!=(const T &other) const noexcept
            {
                return !(m_object == other);
            }

            int use_count() const noexcept
            {

                return m_reference_count == nullptr ? 0 : peek();
            }

            bool unique() const noexcept
            {
                return use_count() == 1;
            }

            void swap(smart_ptr<T> & other) noexcept
            {
                auto temp_object = other.m_object;
                other.m_object = m_object;
                m_object = temp_object;

                auto temp_ref_count = other.m_reference_count;
                other.m_reference_count = m_reference_count;
                m_reference_count = temp_ref_count;
            }

            ~smart_ptr()
            {
                reset();
            }

        private:
            T * m_object;
            int * m_reference_count;

            inline int add_ref() noexcept
            {
                //return the value after addition
                return __sync_add_and_fetch(m_reference_count, 1);
            }
            inline int decrement_ref() noexcept
            {
                //return the value after substraction
                return __sync_sub_and_fetch(m_reference_count, 1);
            }
            inline int peek() const noexcept
            {
                return *m_reference_count;
            }
        };

    }
}
