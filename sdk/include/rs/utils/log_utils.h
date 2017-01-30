// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/**
* \file log_utils.h
* @brief Describes the \c rs::utils::log_util and \c rs::utils::scope_log classes.
*/

#pragma once
#include "rs/utils/logging_service.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <sstream>

#ifdef WIN32 
#ifdef realsense_log_utils_EXPORTS
#define  DLL_EXPORT __declspec(dllexport)
#else
#define  DLL_EXPORT __declspec(dllimport)
#endif /* realsense_log_utils_EXPORTS */
#else /* defined (WIN32) */
#define DLL_EXPORT
#endif

namespace rs
{
    namespace utils
    {
        /**
        * @brief Creates and holds a logger to be used for logging messages
        */
		class DLL_EXPORT log_util
        {
        public:
            /**
            * @brief Creates a \c log_util object with a name, passed as a parameter.
            * @param[in] name Name of the logger to be created. If NULL, then the logger with the application name will be created.
            */
            log_util(wchar_t* name = NULL);
            virtual ~log_util();

            logging_service* m_logger;      /**< Pointer to an object implementing the \c logging_service interface */
            empty_logger m_empty_logger;    /**< Default (empty) logger, with empty implementation of all log functions. Logs to /dev/null. */
        };
    }
}

extern DLL_EXPORT rs::utils::log_util logger;

#define LOG_LOGGER  logger.m_logger // default logger

#define LOG_LEVEL_FATAL_ERROR	rs::utils::logging_service::level_fatal
#define LOG_LEVEL_ERROR			rs::utils::logging_service::level_error
#define LOG_LEVEL_WARNING		rs::utils::logging_service::level_warn
#define LOG_LEVEL_INFO			rs::utils::logging_service::level_info
#define LOG_LEVEL_DEBUG			rs::utils::logging_service::level_debug
#define LOG_LEVEL_TRACE			rs::utils::logging_service::level_trace
#define LOG_LEVEL_VERBOSE		rs::utils::logging_service::level_verbose

#ifndef __FUNCSIG__
#define __FUNCSIG__   __FUNCTION__
#endif

/* _TRUNCATE */
#if !defined(_TRUNCATE)
#define _TRUNCATE ((size_t)-1)
#endif


/**
* @brief Logs a message with specified log level. Logs as chars.
* @param[in] _level Logging level
*/
#define LOG(_level, ...)            												\
{                                                       							\
if (LOG_LOGGER->is_level_enabled(_level))               							\
    {                                                   							\
        char szBuffer[1024];                            							\
        snprintf(szBuffer, 1024, __VA_ARGS__); 										\
        LOG_LOGGER->log(_level, szBuffer, __FILE__, __LINE__, __FUNCTION__);    	\
    }                                                   							\
}

/**
* @brief Logs a message with specified log level. Log as chars
* @param[in] _logger Logger to use
* @param[in] _level Logging level
*/
#define LOG_CFORMAT(_logger, _level, ...)            						\
{                                                       					\
    if (_logger->is_level_enabled(_level))                					\
    {                                                   					\
        char szBuffer[1024] = "";                       					\
        szBuffer[sizeof(szBuffer) - 1] = 0;             					\
        snprintf(szBuffer,1024, __VA_ARGS__); 								\
        _logger->log(_level, szBuffer, __FILE__, __LINE__, __FUNCSIG__); 	\
    }                                                   					\
}

/**
* @brief Logs a message with specified log level. Logs as wide chars.
* @param[in] _logger  Logger to use
* @param[in] _level   Logging level
* @param[in] _message Message to be logged
*/
#define LOG_STREAM(_logger, _level, _message)        									\
{                                                       								\
    if (_logger->is_level_enabled(_level))                								\
    {                                                   								\
        std::basic_ostringstream<wchar_t> _stream;      								\
        _stream << _message;                            								\
        _logger->logw(_level, _stream.str().c_str(), __FILE__, __LINE__, __FUNCSIG__); 	\
    }                                                   								\
}

#define LOG_VERBOSE(_message)        LOG_STREAM(LOG_LOGGER, LOG_LEVEL_VERBOSE, _message)
#define LOG_TRACE(_message)          LOG_STREAM(LOG_LOGGER, LOG_LEVEL_VERBOSE, _message)
#define LOG_DEBUG(_message)          LOG_STREAM(LOG_LOGGER, LOG_LEVEL_DEBUG, _message)
#define LOG_INFO(_message)           LOG_STREAM(LOG_LOGGER, LOG_LEVEL_INFO,  _message)
#define LOG_WARN(_message)           LOG_STREAM(LOG_LOGGER, LOG_LEVEL_WARNING,  _message)
#define LOG_ERROR(_message)          LOG_STREAM(LOG_LOGGER, LOG_LEVEL_ERROR, _message)
#define LOG_FATAL(_message)          LOG_STREAM(LOG_LOGGER, LOG_LEVEL_FATAL_ERROR, _message)

#define LOG_VERBOSE_VAR(_var)        LOG_STREAM(LOG_LOGGER, LOG_LEVEL_VERBOSE, #_var " = " << _var)
#define LOG_TRACE_VAR(_var)          LOG_STREAM(LOG_LOGGER, LOG_LEVEL_TRACE, #_var " = " << _var)
#define LOG_DEBUG_VAR(_var)          LOG_STREAM(LOG_LOGGER, LOG_LEVEL_DEBUG, #_var " = " << _var)
#define LOG_INFO_VAR(_var)           LOG_STREAM(LOG_LOGGER, LOG_LEVEL_INFO,  #_var " = " << _var)
#define LOG_WARN_VAR(_var)           LOG_STREAM(LOG_LOGGER, LOG_LEVEL_WARNING,  #_var " = " << _var)
#define LOG_ERROR_VAR(_var)          LOG_STREAM(LOG_LOGGER, LOG_LEVEL_ERROR, #_var " = " << _var)
#define LOG_FATAL_VAR(_var)          LOG_STREAM(LOG_LOGGER, LOG_LEVEL_FATAL_ERROR, #_var " = " << _var)
#define LOG_LEVEL_VAR(_level, _var)  LOG_STREAM(LOG_LOGGER, _level, #_var " = " << _var)

#define LOG_VERBOSE_CFORMAT(...)     LOG_CFORMAT(LOG_LOGGER, LOG_LEVEL_VERBOSE, __VA_ARGS__)
#define LOG_TRACE_CFORMAT(...)       LOG_CFORMAT(LOG_LOGGER, LOG_LEVEL_TRACE, __VA_ARGS__)
#define LOG_DEBUG_CFORMAT(...)       LOG_CFORMAT(LOG_LOGGER, LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LOG_INFO_CFORMAT(...)        LOG_CFORMAT(LOG_LOGGER, LOG_LEVEL_INFO,  __VA_ARGS__)
#define LOG_WARN_CFORMAT(...)        LOG_CFORMAT(LOG_LOGGER, LOG_LEVEL_WARNING,  __VA_ARGS__)
#define LOG_ERROR_CFORMAT(...)       LOG_CFORMAT(LOG_LOGGER, LOG_LEVEL_ERROR, __VA_ARGS__)
#define LOG_FATAL_CFORMAT(...)       LOG_CFORMAT(LOG_LOGGER, LOG_LEVEL_FATAL_ERROR, __VA_ARGS__)

#define LOG_INFO_CFORMAT_W(...)        LOG_CFORMAT_W(LOG_LOGGER, LOG_LEVEL_INFO,  __VA_ARGS__)

#define LOG_CALL(_func)  LOG_LOGGER->_func

#define LOGGER_TYPE                  LOG_LOGGER->get_logger_type()

namespace rs
{
    namespace utils
    {
        /**
        * @brief Class for scoped log objects. The object will log at the creation and destruction moments only.
        */
        class scope_log
        {
        public:
            /**
            * @brief Constructs a scope log object. The object will log at the creation and destruction moments only.
            * @param[in] msg Message to be logged when the object is created and destructed
            */
            scope_log(const char* msg): _msg(msg)
            {
                LOG_TRACE_CFORMAT("%s - begin", _msg);
            }

            ~scope_log()
            {
                LOG_TRACE_CFORMAT("%s - end", _msg);
            }
        private:
            const char* _msg;
        };
    }
}

#define LOG_FUNC_SCOPE()      rs::utils::scope_log log(__FUNCTION__)
