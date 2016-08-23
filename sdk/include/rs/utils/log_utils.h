// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs/utils/logging_service.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <sstream>

namespace rs
{
    namespace utils
    {
        class log_util
        {
        public:
            /**
            @brief Create a log_util object with a *name* name
            @param[in]    name       The name of the logger to be created. If NULL, then the logger with the application name will be created
            */
            log_util(wchar_t* name = NULL);
            virtual ~log_util();

            logging_service* m_logger;
            empty_logger m_empty_logger;
        };
    }
}

extern rs::utils::log_util logger;

#define LOG_LOGGER  logger.m_logger // default logger

#define LOG_LEVEL_FATAL_ERROR	logging_service::LEVEL_FATAL
#define LOG_LEVEL_ERROR			logging_service::LEVEL_ERROR
#define LOG_LEVEL_WARNING		logging_service::LEVEL_WARN
#define LOG_LEVEL_INFO			logging_service::LEVEL_INFO
#define LOG_LEVEL_DEBUG			logging_service::LEVEL_DEBUG
#define LOG_LEVEL_TRACE			logging_service::LEVEL_TRACE
#define LOG_LEVEL_VERBOSE		logging_service::LEVEL_VERBOSE

#ifndef __FUNCSIG__
#define __FUNCSIG__   __FUNCTION__
#endif

/* _TRUNCATE */
#if !defined(_TRUNCATE)
#define _TRUNCATE ((size_t)-1)
#endif


/**
@brief Log a message with specified log level. Log as chars
@param[in]    _level        Logging level.
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
@brief Log a message with specified log level. Log as chars
@param[in]    _logger       The logger to use.
@param[in]    _level        Logging level.
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
@brief Log a message with specified log level. Log as wide chars
@param[in]    _logger       The logger to use.
@param[in]    _level        Logging level.
@param[in]   _message      Message to be logged.
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

#define LOG_VERBOSE(_message)        LOG_STREAM(LOG_LOGGER, logging_service::LEVEL_VERBOSE, _message)
#define LOG_TRACE(_message)          LOG_STREAM(LOG_LOGGER, logging_service::LEVEL_VERBOSE, _message)
#define LOG_DEBUG(_message)          LOG_STREAM(LOG_LOGGER, logging_service::LEVEL_DEBUG, _message)
#define LOG_INFO(_message)           LOG_STREAM(LOG_LOGGER, logging_service::LEVEL_INFO,  _message)
#define LOG_WARN(_message)           LOG_STREAM(LOG_LOGGER, logging_service::LEVEL_WARN,  _message)
#define LOG_ERROR(_message)          LOG_STREAM(LOG_LOGGER, logging_service::LEVEL_ERROR, _message)
#define LOG_FATAL(_message)          LOG_STREAM(LOG_LOGGER, logging_service::LEVEL_FATAL, _message)

#define LOG_VERBOSE_VAR(_var)        LOG_STREAM(LOG_LOGGER, logging_service::LEVEL_VERBOSE, #_var " = " << _var)
#define LOG_TRACE_VAR(_var)          LOG_STREAM(LOG_LOGGER, logging_service::LEVEL_TRACE, #_var " = " << _var)
#define LOG_DEBUG_VAR(_var)          LOG_STREAM(LOG_LOGGER, logging_service::LEVEL_DEBUG, #_var " = " << _var)
#define LOG_INFO_VAR(_var)           LOG_STREAM(LOG_LOGGER, logging_service::LEVEL_INFO,  #_var " = " << _var)
#define LOG_WARN_VAR(_var)           LOG_STREAM(LOG_LOGGER, logging_service::LEVEL_WARN,  #_var " = " << _var)
#define LOG_ERROR_VAR(_var)          LOG_STREAM(LOG_LOGGER, logging_service::LEVEL_ERROR, #_var " = " << _var)
#define LOG_FATAL_VAR(_var)          LOG_STREAM(LOG_LOGGER, logging_service::LEVEL_FATAL, #_var " = " << _var)
#define LOG_LEVEL_VAR(_level, _var)  LOG_STREAM(LOG_LOGGER, _level, #_var " = " << _var)

#define LOG_VERBOSE_CFORMAT(...)     LOG_CFORMAT(LOG_LOGGER, logging_service::LEVEL_VERBOSE, __VA_ARGS__)
#define LOG_TRACE_CFORMAT(...)       LOG_CFORMAT(LOG_LOGGER, logging_service::LEVEL_TRACE, __VA_ARGS__)
#define LOG_DEBUG_CFORMAT(...)       LOG_CFORMAT(LOG_LOGGER, logging_service::LEVEL_DEBUG, __VA_ARGS__)
#define LOG_INFO_CFORMAT(...)        LOG_CFORMAT(LOG_LOGGER, logging_service::LEVEL_INFO,  __VA_ARGS__)
#define LOG_WARN_CFORMAT(...)        LOG_CFORMAT(LOG_LOGGER, logging_service::LEVEL_WARN,  __VA_ARGS__)
#define LOG_ERROR_CFORMAT(...)       LOG_CFORMAT(LOG_LOGGER, logging_service::LEVEL_ERROR, __VA_ARGS__)
#define LOG_FATAL_CFORMAT(...)       LOG_CFORMAT(LOG_LOGGER, logging_service::LEVEL_FATAL, __VA_ARGS__)

#define LOG_INFO_CFORMAT_W(...)        LOG_CFORMAT_W(LOG_LOGGER, logging_service::LEVEL_INFO,  __VA_ARGS__)

#define LOG_TASK(_task_name)         _LOG_TASK(LOG_LOGGER, logging_service::LEVEL_TRACE, _task_name)
#define LOG_TASK_VERBOSE(_task_name) _LOG_TASK(LOG_LOGGER, logging_service::LEVEL_VERBOSE, _task_name)

#define _LOG_TASK_FRAMEID(_task_name, _level, _frame_id)                                     \
{                                                                                               \
    char _szBuffer[1024];                                                                       \
    _szBuffer[sizeof(_szBuffer) - 1] = 0;                                                       \
    _snprintf_s(_szBuffer, sizeof(_szBuffer) - 1, "%s Frame_id %lld", _task_name, _frame_id);   \
    _LOG_TASK(LOG_LOGGER, _level, _szBuffer);                                             \
}

#define LOG_TASK_FRAMEID(_task_name, _frame_id) _LOG_TASK_FRAMEID(_task_name, logging_service::LEVEL_TRACE, _frame_id)
#define LOG_TASK_VERBOSE_FRAMEID(_task_name, _frame_id) _LOG_TASK_FRAMEID(_task_name, logging_service::LEVEL_VERBOSE, _frame_id)

#define LOG_CALL(_func)  LOG_LOGGER->_func


namespace rs
{
    namespace utils
    {
        /**
        @brief Construct scope log object. The object will log at the creation and destruction moments only
        */
        class scope_log
        {
        public:
            /**
            @brief Construct scope log object. The object will log at the creation and destruction moments only
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
#define LOG_SCOPE(_TASK)      rs::utils::scope_log log(_TASK)
