// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.
#include "rs/utils/log_utils.h"
#include "rs/core/status.h"
#include "fstream"
#include "dlfcn.h"
#include "stdio.h"
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <string>
#include <iostream>
#include <string.h>

using namespace std;
using namespace rs::core;

rs::utils::log_util logger;

namespace rs
{
    namespace utils
    {
        log_util::log_util(wchar_t* name)
        {
            m_logger = &m_empty_logger;


            std::string nameStr;
            if (name)
            {
                char buffer[1024];
                wcstombs(buffer, name,sizeof(buffer));
                std::string tmp(buffer);
                nameStr = tmp;
            }

            if (!name || !name[0])    //if the name of the logger is not hardcoded specified
            {
                string tmp("/proc/self/comm");
                std::ifstream comm(tmp.c_str());

                if (comm.is_open())   //get our process name
                {
                    char buffer[1024];
                    comm.getline(buffer, 1024);
                    string tmp(buffer);
                    nameStr = tmp;
                }
            }

            // try to open shared library with RSLogger object
            void *handle;
            struct passwd *pw = getpwuid(getuid());

            const char *homedir = pw->pw_dir;
            string tmp(homedir);
            string tmp1 = "librs_logger.so";

            handle = dlopen(tmp1.c_str(), RTLD_NOW);
            if (!handle)
            {
                fputs(dlerror(), stderr);
                return;
            }

            status (*func)(logging_service**);

            /* Resolve the method from the shared library */
            func = (status (*)(logging_service**))dlsym( handle, "GetLoggerInstance" );

            if (!func)
            {
                fputs(dlerror(), stderr);
                return;
            }

            //now try to get logger instance
            logging_service* new_logger;

            status sts = (*func)(&new_logger);
            if (sts != status_no_error)
            {
                return;
            }

            m_logger = new_logger;

            string configFile = tmp+"/RSLogs/rslog.properties";

            // Check if logger configured (need to configure only first logger in process)
            if (!m_logger->is_configured())
            {
                const size_t cSize = configFile.length()+1;
                wchar_t* wc = new wchar_t[cSize];
                memset(wc, 0, sizeof(wchar_t)*(cSize));
                mbstowcs (wc, configFile.c_str(), cSize);
                m_logger->configure(logging_service::CONFIG_PROPERTY_FILE_LOG4J, wc , 0);

                if (!m_logger->is_configured()) // init on config file failed
                {
                    delete m_logger;
                    m_logger = &m_empty_logger;
                }
            }

            const size_t cSize = nameStr.length()+1;
            wchar_t* wc = new wchar_t[cSize];
            memset(wc, 0, sizeof(wchar_t)*(cSize));
            mbstowcs (wc, nameStr.c_str(), cSize);
            m_logger->set_logger_name(wc);

        }

        log_util::~log_util()
        {

            if (m_logger != &m_empty_logger)
            {
                m_logger = &m_empty_logger;
            }
        }
    }
}
