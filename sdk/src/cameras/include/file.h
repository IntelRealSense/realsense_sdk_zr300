// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include <fstream>
#include <stdint.h>
#include "status.h"

namespace rs
{
    namespace core
    {
        enum open_file_option
        {
            read = 1,
            write = 2
        };

        enum class move_method
        {
            begin = 0,
            current = 1,
            end = 2,
        };

        class file
        {
        public:
            virtual status open(const std::string& filename, open_file_option mode)
            {
                switch(mode)
                {
                    case open_file_option::read: m_file.open(filename, std::ios::in | std::ios::binary); break;
                    case open_file_option::write: m_file.open(filename, std::ios::out | std::ios::binary); break;
                    default:  // just to avoid warning
                        break;

                }
                return m_file.is_open() ? status_no_error : status_file_open_failed;
            }

            virtual status close()
            {
                if(m_file.is_open())
                    m_file.close();
                return m_file.is_open() ? status_file_close_failed : status_no_error;
            }

            virtual status read_bytes(void* data, unsigned int number_of_bytes_to_read, unsigned int& number_of_bytes_read)
            {
                m_file.read((char*)data, number_of_bytes_to_read);
                number_of_bytes_read = m_file.gcount();
                return m_file ? status_no_error : status_file_read_failed;
            }

            virtual status write_bytes(const void* data, unsigned int number_of_bytes_to_write, unsigned int& number_of_bytes_written)
            {
                m_file.write((char*)data, number_of_bytes_to_write);
                if(m_file) number_of_bytes_written = number_of_bytes_to_write;
                return m_file ? status_no_error : status_file_write_failed;
            }

            virtual status set_position(int64_t distance_to_move, core::move_method method, int64_t* new_file_pointer = NULL)
            {
                switch(method)
                {
                    case move_method::begin: m_file.seekp(distance_to_move, std::ios::beg); break;
                    case move_method::current: m_file.seekp(distance_to_move, std::ios::cur); break;
                    case move_method::end: m_file.seekp(distance_to_move, std::ios::end); break;
                }
                if(new_file_pointer != NULL) *new_file_pointer = m_file.tellp();
                return m_file ? status_no_error : status_file_read_failed;
            }

            virtual status get_position(uint64_t* new_file_pointer)
            {
                if(new_file_pointer != NULL) *new_file_pointer = m_file.tellp();
                return m_file && new_file_pointer != NULL ? status_no_error : status_file_read_failed;
            }

            virtual void reset()
            {
                m_file.clear();
                m_file.seekp(0, std::ios::beg);
            }

            ~file()
            {
                m_file.close();
            }

        private:
            std::fstream m_file;
        };
    }
}
