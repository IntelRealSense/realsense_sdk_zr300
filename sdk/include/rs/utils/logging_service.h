// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/** 
* \file logging_service.h
* @brief Describes the \c logging_service and \c empty_logger classes.
*/

#pragma once

#include "rs/core/status.h"


/**
* @class logging_service
*
* @brief 
* Defines the interface for the logger.
*/
class logging_service
{
public:
    /**
     * @brief Defines the log level of the logger.
     */
    typedef unsigned int log_level;

    /** @enum log_level_values
     *  @brief Variables representing the logging level. These values may be used in log macros or when defining minimal logging level.
     */
    enum log_level_values
    {
        LEVEL_FATAL = 50000,  /**< For use with fatal log messages. This is the highest log level. */
        LEVEL_ERROR = 40000,  /**< For use with error log messages. */
        LEVEL_WARN  = 30000,  /**< For use with warning log messages. */
        LEVEL_INFO  = 20000,  /**< For use with informative log messages. */
        LEVEL_DEBUG = 10000,  /**< For use with debug log messages. */
        LEVEL_TRACE =  5000,  /**< For use with trace level log messages. */
        LEVEL_VERBOSE =  2500,/**< For use with verbose level log messages. This is the lowest log level. */
    };

    /** @enum config_mode
     *
     *  @brief Variables representing the configuration mode.
     */
    enum config_mode
    {
        CONFIG_DEFAULT              = 0x1, /**< Default configuration mode. Configuration is made via method calls. */
        CONFIG_PROPERTY_FILE_LOG4J  = 0x2, /**< Property file is used for configuration. The file should be in Properties/log4j format. */
        CONFIG_XML_FILE_LOG4J       = 0x4, /**< Property file is used for configuration. The file should be in XML/log4j format. */
    };

    /**
    * @brief Gives logger a name in logger hierarchy.
    *
    *  NULL means root logger.
    *
    * @param[in] name Logger name to use
    */
    virtual rs::core::status   set_logger_name(const wchar_t* name)=0;


    /**
    * @brief Configures the logger from a properties file.
    * @param[in]    config_mode      Configuration mode
    * @param[in]    config           Configuration file name
    * @param[in]    file_watch_delay If non-zero, specifies delay in milliseconds to check if the configuration file was changed 
	                                 (only for \c config_mode::CONFIG_PROPERTY_FILE_LOG4J and \c config_mode::CONFIG_XML_FILE_LOG4J).
    */
    virtual rs::core::status   configure(config_mode config_mode, const wchar_t* config, int file_watch_delay)=0; //

    /**
    * @brief Returns TRUE if the logger is already configured. 
	*
	* Configuration is process-wide for all loggers, call \c configure() once per application.
    */
    virtual bool     is_configured()=0; //

    /**
    * @brief Overwrites level specified in initial configuration.
    * @param[in] level New log level
    */
    virtual rs::core::status   set_level(log_level level)=0;

    /**
    * @brief Returns TRUE if current log level is higher than the parameter level.
    * @param[in] level Level to check
    */
    virtual bool     is_level_enabled(log_level level)=0;

    /**
    * @brief Returns current log level
    */
    virtual log_level    get_level()=0;

    /**
    * @brief Logs a message at the specified log level. 
	*
	* Generally, you should not use this method directly, but only from the \c LOG() macro.
    * @param[in]    level             Log level of the current message
    * @param[in]    message           Message to be logged
    * @param[in]    file_name         Name of the file to be logged
    * @param[in]    line_number       Number of the line to be logged
    * @param[in]    function_name     Name of the method to be logged
    */
    virtual void     log (log_level level, const char*    message, const char* file_name, int line_number, const char* function_name)=0;

    /**
    * @brief Same as \c log(), but for wide chars.
    */
    virtual void     logw(log_level level, const wchar_t* message, const char* file_name, int line_number, const char* function_name)=0;

    virtual ~logging_service() {}
};


/**
* @class empty_logger
* @brief Implements default (empty) logger, with empty implementation of all log methods. Logs to /dev/null.
*/
class empty_logger: public logging_service
{
public:
    virtual rs::core::status   set_logger_name(const wchar_t* /*name*/) { return rs::core::status_no_error; }
    virtual rs::core::status   configure(config_mode /*configMode*/, const wchar_t* /*config*/, int /*fileWatchDelay*/) { return rs::core::status_no_error; }
    virtual bool               is_configured() { return true; }
    virtual rs::core::status   set_level(log_level /*level*/) { return rs::core::status_no_error; }
    virtual bool               is_level_enabled(log_level /*level*/) { return false; }
    virtual log_level              get_level() { return 0; }
    virtual void               log (log_level /*level*/, const char*    /*message*/, const char* /*fileName*/, int /*lineNumber*/, const char* /*functionName*/) { }
    virtual void               logw(log_level /*level*/, const wchar_t* /*message*/, const char* /*fileName*/, int /*lineNumber*/, const char* /*functionName*/) { }
};

