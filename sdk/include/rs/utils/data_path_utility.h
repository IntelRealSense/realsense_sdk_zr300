// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/** 
* \file data_path_utility.h
* @brief Describes the \c rs::utils::data_path class.
*/

#pragma once

#ifdef WIN32
#define PATH_MAX 4096
#include <Windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>
#endif

#include "rs/utils/log_utils.h"

namespace rs
{
    namespace utils
    {

        /**
         * @brief Provides a way to retrieve the SDK files on the system.
         *
         * SDK files are retrieved so that SDK modules can access files that
         * they require for their operations without being aware of the OS file system structure.
         * 
         * Call the methods once, during initialization.
         */
        class data_path
        {
        public:
            /**
             * @brief Checks if the data folder exists and stores a path to it.
             *
             * The data folder is supposed to be located in one of the following places: SDK module local directory
             * or some directory specified by environment variable - RS_SDK_DATA_PATH or in the /opt/intel directory under sdk folder.
             * If the environment variable is defined, the data folder should always be found.
             */
            data_path()
            {
                m_data_path = std::string("Cannot find path to 'data' folder.");
                m_is_succeed = false;

                get_exec_dir();
                std::string test_local(m_exec_dir + "/data");
                if(is_dir_exists(test_local))
                {
                    m_data_path = test_local;
                    m_is_succeed = true;
                    return;
                }

                char* dpath = getenv("RS_SDK_DATA_PATH");
                if(dpath != nullptr)
                {
                    m_data_path = std::string(dpath);
                    m_is_succeed = true;
                    return;
                }
                else
                {
                    LOG_INFO("Cannot find RS_SDK_DATA_PATH environment variable.");
                }

                std::string test_opt("/opt/intel/rssdk/data");
                if(is_dir_exists(test_opt))
                {
                    m_data_path = test_opt;
                    m_is_succeed = true;
                    return;
                }

            }

            /**
             * @brief Provides a path to the data files.
             *
             * The requested path string is copied to the user defined buffer. The user passes the buffer length so the method can
             * validate that the buffer is long enough. If the buffer is null or the length is too short, the method does not output
             * the path and instead just returns the required output buffer length.
             *
             * The caller can use this method to retrieve the required output buffer length before allocating the buffer and call
             * the method again with a suitable buffer provided.
             * @param[out] buf      Buffer to return the path to the data folder in
             * @param[in] length    Provided buffer length
             * @return int         Actual length of the string containing a path to data folder
             * @return -1          Data folder was not found
             */
            int get_path(char * buf, size_t length)
            {
                if(!m_is_succeed)  return -1;
                if(!buf || length < m_data_path.length()) return static_cast<int>(m_data_path.length());

                strlcpy(buf, m_data_path.c_str(), length);
                return static_cast<int>(m_data_path.length());
            }

        private:
            /**
             * @brief Checks if the directory specified by the specified pathname exists.
             *
             * @param[in] pathname  Path to the directory
             * @return true        Directory exists
             * @return false       Directory does not exist
             */
            bool is_dir_exists(std::string pathname)
            {
                //cross-platform way to check if directory exists
                struct stat info;
                char message[PATH_MAX];

                if( stat( pathname.c_str(), &info ) != 0 )
                {
                    snprintf( message, PATH_MAX, "Cannot access %s\n", pathname.c_str() );
                    LOG_WARN(message);
                    return 0;
                }
                else if( info.st_mode & S_IFDIR ) // S_ISDIR() doesn't exist on my windows
                {
                    snprintf( message, PATH_MAX, "%s is a directory\n", pathname.c_str() );
                    LOG_WARN(message);
                    return 1;
                }
                else
                {
                    snprintf( message, PATH_MAX, "%s is not a directory\n", pathname.c_str() );
                    LOG_WARN(message);
                    return 0;
                }
            }

            /**
             * @brief Gets SDK module executable's directory.
             *
             * Populate internal variable that specify the executable's directory.
             */
            void * get_exec_dir()
            {
                char buff[PATH_MAX];
                memset(buff, 0, PATH_MAX);
        #ifdef WIN32
                HMODULE hm = NULL;

                if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                        (LPCSTR) buff,
                                        &hm))
                {
                    m_exec_dir = std::string("GetModuleHandle returned incorrect value");
                    LOG_WARN(m_exec_dir.c_str());
                }
                GetModuleFileNameA(hm, buff, sizeof(buff));

        #else
                Dl_info dl_info;
                if(0 != dladdr((void*)"my_idlopen_init", &dl_info))
                {
                    strlcpy(buff, dl_info.dli_fname, PATH_MAX);
                    fprintf(stderr, "module %s loaded\n", dl_info.dli_fname);
                }
                else
                {
                    ssize_t size = readlink("/proc/self/exe", buff, PATH_MAX-1);
                }
        #endif
                if (strlen(buff) > 0)
                {
                    *strrchr(buff, '/') = '\0';
                    m_exec_dir = std::string(buff);
                }
                else
                {
                    m_exec_dir = std::string("Cannot define executable location ");
                    LOG_WARN(m_exec_dir.c_str());
                }

                return NULL;
            }

            size_t strlcpy(char *d, char const *s, size_t n)
            {
                return snprintf(d, n, "%s", s);
            }

            std::string m_exec_dir;
            std::string m_data_path;
            bool m_is_succeed;
        };
    }
}
