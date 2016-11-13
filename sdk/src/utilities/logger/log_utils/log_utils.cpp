// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.
#include "rs_sdk_version.h"
#include "rs/utils/log_utils.h"
#include "rs/core/status.h"
#include "fstream"
#include "stdio.h"
#include <string>
#include <iostream>
#include <string.h>
#ifdef WIN32
#include <shlobj.h>
#else
#include <pwd.h>
#include "dlfcn.h"
#include <sys/types.h>
#include <unistd.h>
#endif

using namespace std;
using namespace rs::core;

DLL_EXPORT rs::utils::log_util logger;

namespace rs
{
	namespace utils
	{
		log_util::log_util(wchar_t* name)
		{
#ifdef WIN32
			m_logger = &m_empty_logger;


			std::string nameStr;
			if (name)
			{
				char buffer[1024];
				wcstombs(buffer, name, sizeof(buffer));
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
			char path[1024];
			HRESULT res = SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path);
			if (res != S_OK)
			{
				cerr << res << endl;
				return;
			}
			HINSTANCE handle = LoadLibrary("librs_logger.dll");
			if (!handle)
			{
				cerr << GetLastError() << endl;
				return;
			}
			string tmp(path);
			status(*func)(logging_service**);
			func = (status(*)(logging_service**))GetProcAddress(handle, "GetLoggerInstance");

			//now try to get logger instance
			logging_service* new_logger;
			status sts = (*func)(&new_logger);
			if (sts != status_no_error)
			{
				return;
			}

			m_logger = new_logger;
			string configFile = tmp + "/RSLogs/rslog.properties";

			// Check if logger configured (need to configure only first logger in process)
			if (!m_logger->is_configured())
			{
				const size_t cSize = configFile.length() + 1;
				wchar_t* wc = new wchar_t[cSize];
				memset(wc, 0, sizeof(wchar_t)*(cSize));
				mbstowcs(wc, configFile.c_str(), cSize);
				m_logger->configure(logging_service::CONFIG_PROPERTY_FILE_LOG4J, wc, 0);
				delete[] wc;
				if (!m_logger->is_configured()) // init on config file failed
				{
					delete m_logger;
					m_logger = &m_empty_logger;
				}
			}

			const size_t cSize = nameStr.length() + 1;
			wchar_t* wc = new wchar_t[cSize];
			memset(wc, 0, sizeof(wchar_t)*(cSize));
			mbstowcs(wc, nameStr.c_str(), cSize);
			m_logger->set_logger_name(wc);
			delete[] wc;
			FreeLibrary(handle);
#else
			m_logger = &m_empty_logger;


			std::string name_string;
			if (name)
			{
				char buffer[1024];
				wcstombs(buffer, name, sizeof(buffer));
				std::string tmp(buffer);
				name_string = tmp;
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
					name_string = tmp;
				}
			}

			// try to open shared library with RSLogger object
			void *handle;
			struct passwd *pw = getpwuid(getuid());

            string rs_logger_lib_name = "librealsense_logger.so";

			handle = dlopen(rs_logger_lib_name.c_str(), RTLD_NOW);

			if (!handle)
			{
				char* error_message = dlerror();
				if (error_message)
					fputs(error_message, stderr);
				return;
			}

            status(*get_logger_instance_func)(logging_service**);
            void(*check_version_func)(int*, int*);

            check_version_func = (void(*)(int*, int*))dlsym(handle, "GetLibMajorMinorVersion");
            if (!check_version_func)
            {
                fputs("realsense_logger version does not match - logging disabled\n", stderr);
                return;
            }

            int maj = -1, min = -1;
             (*check_version_func)(&maj, &min);

            if ((maj != SDK_VER_MAJOR) ||
                (maj == 0 && min != SDK_VER_MINOR))
            {
                fputs("realsense_logger version does not match - logging disabled\n", stderr);
                return;
            }

			/* Resolve the method from the shared library */
            get_logger_instance_func = (status(*)(logging_service**))dlsym(handle, "GetLoggerInstance");
            if (!get_logger_instance_func)
			{
				char* error_message = dlerror();
				if (error_message)
					fputs(error_message, stderr);
				return;
			}

			//now try to get logger instance
			logging_service* new_logger;

            status sts = (*get_logger_instance_func)(&new_logger);
			if (sts != status_no_error)
			{
				return;
			}

			m_logger = new_logger;

			char* properties_location = getenv("REALSENSE_SDK_LOG_PATH");

			string config_file_path;

			if (properties_location)
			{
				config_file_path = properties_location;
			}
			else
			{
				const char *homedir = pw->pw_dir;
				string home_directory(homedir);

				config_file_path = home_directory + "/realsense/logs/";

			}

			config_file_path += "/rslog.properties";

			// Check if logger configured (need to configure only first logger in process)
			if (!m_logger->is_configured())
			{
				const size_t cSize = config_file_path.length() + 1;

				wchar_t* wc = new wchar_t[cSize];
				memset(wc, 0, sizeof(wchar_t)*(cSize));
				mbstowcs(wc, config_file_path.c_str(), cSize);
				m_logger->configure(logging_service::CONFIG_PROPERTY_FILE_LOG4J, wc, 0);
				delete[] wc;
				if (!m_logger->is_configured()) // init on config file failed
				{
					delete m_logger;
					m_logger = &m_empty_logger;
				}
			}

			const size_t cSize = name_string.length() + 1;
			wchar_t* wc = new wchar_t[cSize];
			memset(wc, 0, sizeof(wchar_t)*(cSize));
			mbstowcs(wc, name_string.c_str(), cSize);
			m_logger->set_logger_name(wc);
			delete[] wc;
#endif
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
