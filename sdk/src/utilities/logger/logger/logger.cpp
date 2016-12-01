// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.
#include "rs/utils/logging_service.h"
#include "xlevel.h"

#include <log4cxx/logstring.h>
#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/xml/domconfigurator.h>
#include "fstream"
#include "rs_sdk_version.h"

using namespace log4cxx;
using namespace rs::core;

namespace rs
{
    namespace utils
    {
        class Log4cxx: public logging_service
        {
        public:
            Log4cxx()
            {
                logger = log4cxx::Logger::getRootLogger();
            }
            virtual ~Log4cxx()
            {
            }

            virtual logger_type get_logger_type()
            {
                return logger_type::LOG4CXX_LOGGER;
            }

            virtual status set_logger_name(const wchar_t* name)
            {
                if (name && name[0])
                {
                    this->logger = log4cxx::Logger::getLogger(name);
                }
                else
                {
                    this->logger = log4cxx::Logger::getRootLogger();
                }
                return status_no_error;
            }

            virtual status configure(config_mode configMode, const wchar_t* config, int fileWatchDelay)
            {

                LoggerPtr rootLogger = log4cxx::Logger::getRootLogger();

                /* Workaround for log4cxx not-thread-safe level initialization - pre-create static objects. */
                log4cxx::XLevel::getOff();
                log4cxx::XLevel::getFatal();
                log4cxx::XLevel::getError();
                log4cxx::XLevel::getWarn();
                log4cxx::XLevel::getInfo();
                log4cxx::XLevel::getDebug();
                log4cxx::XLevel::getTrace();
                log4cxx::XLevel::getVerbose();
                log4cxx::XLevel::getAll();

                switch (configMode)
                {
                    case CONFIG_DEFAULT:
                        BasicConfigurator::configure();
                        LOG4CXX_INFO(rootLogger, "Logging initialized with default configuration");
                        return status_no_error;

                    case CONFIG_PROPERTY_FILE_LOG4J:
                        if (fileWatchDelay)
                        {
                            PropertyConfigurator::configureAndWatch(config, fileWatchDelay);
                        }
                        else
                        {
                            PropertyConfigurator::configure(config);
                        }
                        LOG4CXX_INFO(rootLogger, "Loaded logging configuration from file "/* << config*/);
                        return status_no_error;

                    case CONFIG_XML_FILE_LOG4J:
                        if (fileWatchDelay)
                        {
                            xml::DOMConfigurator::configureAndWatch(config, fileWatchDelay);
                        }
                        else
                        {
                            xml::DOMConfigurator::configure(config);
                        }
                        LOG4CXX_INFO(rootLogger, L"Loaded logging configuration from file" << config);
                        return status_no_error;
                }
                return status_feature_unsupported;
            }

            virtual bool is_configured()
            {
                LoggerPtr rootLogger = log4cxx::Logger::getRootLogger();
                return rootLogger->getAllAppenders().size() > 0 ? true : false;
            }

            virtual status set_level(log_level level)
            {
                logger->setLevel(log4cxx::XLevel::toLevel(level));
                return status_no_error;
            }

            virtual bool is_level_enabled(log_level level)
            {
                return logger->isEnabledFor(log4cxx::XLevel::toLevel(level));
            }

            virtual log_level get_level()
            {
                return logger->getEffectiveLevel()->toInt();
            }

            virtual void log(log_level level, const char* message, const char* file_name, int line_number, const char* function_name)
            {
                const std::string msg(message);
                try
                {
                    logger->forcedLog(log4cxx::XLevel::toLevel(level), msg, ::log4cxx::spi::LocationInfo(file_name, function_name, line_number));
                }
                catch(std::exception& ex)
                {
                    //sometimes we get IOException for file lock - ignore
                }
            }

            virtual void logw(log_level level, const wchar_t* message, const char* file_name, int line_number, const char* function_name)
            {
                const std::wstring msg((wchar_t*)message);
                try
                {
                    logger->forcedLog(log4cxx::XLevel::toLevel(level), msg, ::log4cxx::spi::LocationInfo(file_name, function_name, line_number));
                }
                catch(std::exception& ex)
                {
                    //sometimes we get IOException for file lock - ignore
                }
            }

        protected:
            log4cxx::Logger* logger;
        };
    }
}

extern "C" status GetLoggerInstance(rs::utils::logging_service **instance)
{
    if (!instance) return status_handle_invalid;
    *instance = new rs::utils::Log4cxx();
    return (*instance) ? status_no_error : status_alloc_failed;
}

extern "C" void GetLibMajorMinorVersion(int* maj, int* min )
{
    *maj = SDK_VER_MAJOR;
    *min = SDK_VER_MINOR;
}

