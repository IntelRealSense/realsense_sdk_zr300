#pragma once
#include <stdexcept>

namespace rs
{
    namespace utils
    {
        template <class T>
        class cyclic_array {
          public:
            explicit cyclic_array(unsigned int capacity = 0) : m_head(0), m_tail(0), m_contents_size(0), m_array_size(capacity), m_array(capacity)
            {
            }

            // The function MOVES the new_element to the cyclic array. The original copy
            // may not be safe to use further, depending on Move CTor behaviour
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

            void pop_front()
            {
                if (m_contents_size == 0) return;

                m_array[m_head] = std::move(m_empty_object);

                m_head++;
                m_head = m_head % m_array_size;
                m_contents_size--;
            }

            T& front()
            {
                if (m_contents_size == 0)
                {
                    throw std::out_of_range("Can not reference an empty array!");
                }

                return m_array[m_head];
            }

            unsigned int size() { return m_contents_size; }


        private:
            std::vector<T> m_array;
            T             m_empty_object;
            unsigned int  m_array_size;
            unsigned int  m_head;               // points to teh first to return element
            unsigned int  m_tail;               // points to the next cell to insert an element
            unsigned int  m_contents_size;
        };
    }
}
