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
            smart_ptr(): m_object(nullptr), m_reference_count(new reference_count_wrapper(1)) {}
            smart_ptr(T * object): m_object(object), m_reference_count(new reference_count_wrapper(1)) {}
            smart_ptr(const smart_ptr<T>& other) : m_object(other.m_object), m_reference_count(other.m_reference_count)
            {
                m_reference_count->add_ref();
            }

            T& operator*() const
            {
                return *m_object;
            }

            T* get() const
            {
                return m_object;
            }

            T* operator->() const
            {
                return m_object;
            }

            explicit operator bool() const
            {
                return m_object != nullptr;
            }

            void reset()
            {
                if(m_reference_count->decrement_ref() == 0)
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
                operator=(other);
            }

            smart_ptr<T>& operator=(T * other)
            {
                if(m_object == other)
                    return *this;

                //mutex?
                reset();

                m_object = other;
                m_reference_count = new reference_count_wrapper(1);

                return *this;
            }

            smart_ptr<T>& operator=(const smart_ptr<T> & other)
            {
                if(this == &other)
                    return *this;
                //mutex?
                reset();

                m_object = other.m_object;
                m_reference_count = other.m_reference_count;
                m_reference_count->add_ref();

                return *this;
            }

            bool operator==(const smart_ptr<T> &other)
            {
                return m_object == other.m_object;
            }

            bool operator==(const T & other)
            {
                return m_object == other;
            }

            bool operator!=(const smart_ptr<T> &other)
            {
                return m_object != other.m_object;
            }

            bool operator!=(const T &other)
            {
                return m_object != other;
            }

            ~smart_ptr()
            {
                //mutex?
                reset();
            }

        private:
            T * m_object;

            //internal reference count handling class
            class reference_count_wrapper
            {
            public:
                reference_count_wrapper() = delete;
                reference_count_wrapper(int counter) : m_reference_count(counter) {}
                void add_ref()
                {
                    __sync_add_and_fetch(&m_reference_count, 1);
                }
                int decrement_ref()
                {
                    return __sync_sub_and_fetch(&m_reference_count, 1);
                }
            private:
                int m_reference_count;
            };

            reference_count_wrapper * m_reference_count;
        };

    }
}
