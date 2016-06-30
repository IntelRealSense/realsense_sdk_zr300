// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.


#pragma once

#include "rs/core/status.h"

//using namespace rs::core;

class logging_service
{
public:
    typedef unsigned int Level;
    enum
    {
        LEVEL_FATAL = 50000,
        LEVEL_ERROR = 40000,
        LEVEL_WARN  = 30000,
        LEVEL_INFO  = 20000,
        LEVEL_DEBUG = 10000,
        LEVEL_TRACE =  5000,
        LEVEL_VERBOSE =  2500,
    };

    enum ConfigMode
    {
        CONFIG_DEFAULT              = 0x1,
        CONFIG_PROPERTY_FILE_LOG4J  = 0x2,
        CONFIG_XML_FILE_LOG4J       = 0x4,
    };

    /**
    @brief Give logger a name in loggers hierarchy. NULL means root logger. Name may contain dots like class and namespace hierarchy in C#
    @param[in]    name       The logger to use.
    */
    virtual rs::core::status   set_logger_name(const wchar_t* name)=0;


    /**
    @brief Configures the logger from properties file.
    @param[in]    config_mode      The mode of configure.
    @param[in]    config           Name of config file.
    @param[in]    file_watch_delay If fileWatchDelay non-zero, specifies delay in ms to check if config file changed (only for CONFIG_PROPERTY_FILE_LOG4J and CONFIG_XML_FILE_LOG4J)
    */
    virtual rs::core::status   configure(ConfigMode config_mode, const wchar_t* config, int file_watch_delay)=0; //

    /**
    @brief Returns TRUE if the logger is already configured. Configuration is process-wide for all loggers, call Configure() once per application
    */
    virtual bool     is_configured()=0; //

    /**
    @brief Overwrite level specified in initial configuration.
    @param[in]    level    New log level.
    */
    virtual rs::core::status   set_level(Level level)=0; // overwrite level specified in initial configuration

    /**
    @brief Returns TRUE if current log level is higher than the parameter level.
    @param[in]    level   The level to check
    */
    virtual bool     is_level_enabled(Level level)=0;

    /**
    @brief Returns current log level
    */
    virtual Level    get_level()=0;

    /**
    @brief Logs a message at specified log level. You generally should not use this function directly, but from LOG() macro
    @param[in]    level             The log level of the current message.
    @param[in]    message           The message to be logged
    @param[in]    file_name         The name of the file to be logged
    @param[in]    line_number       The number of the line to be logged
    @param[in]    function_name     The name of the function to be logged
    */
    virtual void     log (Level level, const char*    message, const char* file_name, int line_number, const char* function_name)=0;

    /**
    @brief Same as log(..), but for wide chars
    */
    virtual void     logw(Level level, const wchar_t* message, const char* file_name, int line_number, const char* function_name)=0;

    // performance tracing
    virtual void        task_begin(Level level, const char* task_name)=0;
    virtual void        task_end(Level level, const char* task_name)=0;

    virtual ~logging_service() {}
};

// Utility class for tasks in performance tracing
class trace_task
{
public:
    trace_task(logging_service *logger, logging_service::Level level, const char* taskName)
    {
        if (logger->is_level_enabled(level) && taskName[0])
        {
            this->logger = logger;
            this->level = level;
            this->taskName = taskName;
            logger->task_begin(level, taskName);
        }
        else
        {
            this->logger = 0;
        }
    }
    ~trace_task()
    {
        if (this->logger)
        {
            logger->task_end(level, taskName);
        }
    }
private:
    logging_service*         logger;
    logging_service::Level   level;
    const char*     taskName;
};


class empty_logger: public logging_service
{
public:
    virtual rs::core::status   set_logger_name(const wchar_t* /*name*/) { return rs::core::status_no_error; }
    virtual rs::core::status   configure(ConfigMode /*configMode*/, const wchar_t* /*config*/, int /*fileWatchDelay*/) { return rs::core::status_no_error; }
    virtual bool               is_configured() { return true; }
    virtual rs::core::status   set_level(Level /*level*/) { return rs::core::status_no_error; }
    virtual bool               is_level_enabled(Level /*level*/) { return false; }
    virtual Level              get_level() { return 0; }
    virtual void               log (Level /*level*/, const char*    /*message*/, const char* /*fileName*/, int /*lineNumber*/, const char* /*functionName*/) { }
    virtual void               logw(Level /*level*/, const wchar_t* /*message*/, const char* /*fileName*/, int /*lineNumber*/, const char* /*functionName*/) { }
    virtual void               task_begin(Level /*level*/, const char* /*task_name*/) { }
    virtual void               task_end(Level /*level*/, const char* /*task_name*/) { }
};

