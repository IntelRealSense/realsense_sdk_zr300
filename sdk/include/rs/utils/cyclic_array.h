// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/** @file cyclic_array.h
    @brief Implements the template for cyclic array of objects of type T
    T has to have default constructor and move constructor.
  */


#pragma once
#include <stdexcept>

namespace rs
{
    namespace utils
    {
        /**
        * @class cyclic_array
        * @brief The class implements the cyclic arrayc of elements of type T. The class
        *        does not allocates new memory per insert. Instead of it, the space for
        *        the elements is allocated when creating a cyclic array, and elements pushed to it
        *        are actually moved using std::move.
        */
        template <class T>
        class cyclic_array {
          public:
            /**
            * @brief The constructor creates cyclic array of -capacity- elements.
            *        Default value is 0, while values lower than 1 are not legal and will cause exception
            *        when trying to push new element to this cyclic array.
            * @param[in]  capacity       Max number of elements in the cyclic array.
            */
            explicit cyclic_array(unsigned int capacity = 0) : m_head(0), m_tail(0), m_contents_size(0), m_array_size(capacity), m_array(capacity)
            {
            }

            /**
            * @brief The function MOVES the new_element to the cyclic array. The original copy
            *        may not be safe to use further, depending on Move CTor behaviour.
            *        If number of elements in the array is equal to its max size, the first (oldest) element
            *        will be overwritten with the new one (new_element).
            * @param[in]  new_element       The element that has to be inserted to the end of the cyclic array.
            */
            void push_back(T& new_element)
            {

                if (m_array_size == 0)
                    throw std::out_of_range("Can not push to the array of size 0!");

                // move head to next element if the array is full and we
                // are going to add another element, so now the head
                // will point to the next element, and we can
                // safely add new element
                if (m_tail == m_head && m_contents_size != 0)
                {
                    m_head++;
                    m_head = m_head % m_array_size;
                    m_contents_size--;
                }

                m_array[m_tail] = std::move(new_element);
                m_tail++;
                m_contents_size++;

                m_tail = m_tail % m_array_size;

            }

            /**
            * @brief The function removes the first (oldest) element from the cyclic array. The current size of the
            *        cyclic arrays decreased by 1. The function does nothing if there are no elements in the array.
            */

            void pop_front()
            {
                if (m_contents_size == 0) return;

                m_array[m_head] = std::move(m_empty_object);

                m_head++;
                m_head = m_head % m_array_size;
                m_contents_size--;
            }

            /**
            * @brief The function returns the reference to the first (oldest) element in the cyclic array.
            *        The function throws out-of-range exception, if the cyclic array is empty.
            * @return T&     Reference to the oldest object in the array.
            */
            T& front()
            {
                if (m_contents_size == 0)
                {
                    throw std::out_of_range("Can not reference an empty array!");
                }

                return m_array[m_head];
            }

            /**
            * @brief The function returns the number of elements in the cyclic array.
            * @return Number of elements in the cyclic array.
            */
            unsigned int size() { return m_contents_size; }


        private:
            std::vector<T> m_array;             /**< vector of elements of type T, holding the cyclic array */
            T             m_empty_object;       /**< empty object of type T, which is copied to the element's place, once the element is poped */
            unsigned int  m_array_size;         /**< maximum number of elements in the cyclic array (size of the queue) */
            unsigned int  m_head;               /**< cyclic index of the first actual element (head of the queue) */
            unsigned int  m_tail;               /**< cyclic index of the last  actual element (tail of the queue) */
            unsigned int  m_contents_size;      /**< number of actual elements in the cyclic array (current length of the queue) */
        };
    }
}
