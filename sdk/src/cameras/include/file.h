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

            virtual status read_bytes(void* data, unsigned int numberOfBytesToRead, unsigned int& numberOfBytesRead)
            {
                m_file.read((char*)data, numberOfBytesToRead);
                numberOfBytesRead = static_cast<unsigned int>(m_file.gcount());
                return numberOfBytesToRead == numberOfBytesRead ? status_no_error : status_file_read_failed;
            }

            virtual status write_bytes(const void* data, unsigned int numberOfBytesToWrite, unsigned int& numberOfBytesWritten)
            {
                unsigned int before = static_cast<unsigned int>(m_file.tellp());
                m_file.write((char*)data, numberOfBytesToWrite);
                unsigned int curr = static_cast<unsigned int>(m_file.tellp());
                numberOfBytesWritten = curr - before;
                return numberOfBytesToWrite == numberOfBytesWritten ? status_no_error : status_file_write_failed;
            }

            virtual status set_position(int64_t distanceToMove, move_method moveMethod, int64_t* newFilePointer = NULL)
            {
                int before = static_cast<int>(m_file.tellp());
                switch(moveMethod)
                {
                    case move_method::begin: m_file.seekp(distanceToMove, std::ios::beg); break;
                    case move_method::current: m_file.seekp(distanceToMove, std::ios::cur); break;
                    case move_method::end: m_file.seekp(distanceToMove, std::ios::end); break;
                }
                int curr = static_cast<int>(m_file.tellp());
                if(newFilePointer != NULL) *newFilePointer = curr;
                return abs(static_cast<int>(distanceToMove)) == abs(curr - before) ? status_no_error : status_file_read_failed;
            }

            virtual status get_position(uint64_t* newFilePointer)
            {
                if(newFilePointer != NULL) *newFilePointer = m_file.tellp();
                return newFilePointer != NULL ? status_no_error : status_file_read_failed;
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
