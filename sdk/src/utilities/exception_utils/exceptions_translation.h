// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs/utils/log_utils.h"
#include "exception_impl.h"

namespace rs
{
    namespace utils
    {
        /**
         * @brief Invoke user-specified function and return error value if exception is thrown.
         *
         * This function safely invokes user-specified function and returns a value of the user-defined type,
         * catching any thrown exception during execution and returning user-specified error value.
         * This function is guaranteed to no throw.
         * If the invoked user-specified function throws, a user-defined error value is returned, otherwise,
         * the return value is the value returned by invoked user-specified function.
         * In case the invoked function returns void, a wrapper-function with a non-void return value is expected to be provided by the user
         * (for example, [] { my_void_func(); return status_no_error; }).
         * The function logs any exception that is thrown by the invoked user-specified function.
         * Use a macro instead that calls this function providing appropriate function_name, line, file parameters. Otherwise,
         * use this function if there is a need to define function name, line number and file name manually.
         * @param[in] function_to_invoke        User-specified function to invoke.
         *                                      The invoked function must be a callable object with defined operator() that has no parameters.
         * @param[in] function_name             Function name of the calling function  (e.g. "get_device").
         * @param[in] line                      Line number in the calling function    (e.g. 177 - in "get_device").
         * @param[in] file                      File that defines the calling function (e.g. "device.cpp").
         * @return: T(user-defined type)        Return value of the invoked function.
         * @return: error_value                 The returned value if invoked function throws exception.
         */
        template<typename T, T error_value, typename callable>
        static T safe_invoke_function(callable&& function_to_invoke, const char* function_name, const int& line, const char* file) noexcept(true)
        {
            try
            {
                return function_to_invoke();
            }
            catch (const rs::core::exception& e)
            {
                try
                {
                    if (logger.m_logger->is_level_enabled(logging_service::level_warn))
                    {
                        char buffer[2048] = "";
                        buffer[sizeof(buffer) - 1] = 0;
                        snprintf(buffer,2048,"%s",e.what());
                        logger.m_logger->log(logging_service::level_warn, buffer, file, line, e.function());
                    }
                } catch (...) {}
                return error_value;
            }
            catch (const std::exception& e)
            {
                try
                {
                    if (logger.m_logger->is_level_enabled(logging_service::level_warn))
                    {
                        char buffer[2048] = "";
                        buffer[sizeof(buffer) - 1] = 0;
                        snprintf(buffer,2048,"%s",e.what());
                        logger.m_logger->log(logging_service::level_warn, buffer, file, line, function_name);
                    }
                } catch (...) {}
                return error_value;
            }
            catch (...)
            {
                try
                {
                    if (logger.m_logger->is_level_enabled(logging_service::level_warn))
                    {
                        char buffer[2048] = "";
                        buffer[sizeof(buffer) - 1] = 0;
                        snprintf(buffer,2048,"%s","unknown exception");
                        logger.m_logger->log(logging_service::level_warn, buffer, file, line, function_name);
                    }
                } catch (...) {}
                return error_value;
            }
        }


        /**
         * @brief Translate any thrown exception into sdk exception.
         *
         * This function translates std::exception or unknown exception into sdk exception or re-throws sdk exception (if it was already a current one),
         * that are originally thrown by invoked user-specified function, which is passed as parameter.
         * If the function throws, an application is to be terminated unless the user-defined exception handling is provided (e.g. try-catch).
         * The function does not throw on sucessful execution of the invoked user-specified function.
         * The function logs std::exception and unknown exceptions thrown by the invoked user-specified function, the sdk exception is supposed
         * to be logged by the sdk module if it is intended.
         * Use a macro instead that calls this function providing appropriate function_name, line, file parameters. Otherwise,
         * use this function if there is a need to define function name, line number and file name manually.
         * @param[in] function_to_invoke        User-specified function to invoke.
         *                                      The invoked function must be a callable object with defined operator() that has no parameters.
         * @param[in] function_name             Function name of the calling function  (e.g. "get_device").
         * @param[in] line                      Line number in the calling function    (e.g. 177 - in "get_device").
         * @param[in] file                      File that defines the calling function (e.g. "device.cpp").
         * @return: T(user-defined type)        Return value of the invoked function.
         */
        template<typename T, typename callable>
        static T invoke_function_and_translate_exception(callable&& function_to_invoke, const char* function_name, const int& line, const char* file) noexcept(false)
        {
            try
            {
                return function_to_invoke();
            }
            catch (const rs::core::exception& e) // expect sdk exception to be logged before
            {
                try
                {
                    if (logger.m_logger->is_level_enabled(logging_service::level_fatal))
                    {
                        char buffer[2048] = "";
                        buffer[sizeof(buffer) - 1] = 0;
                        snprintf(buffer,2048,"%s",e.what());
                        logger.m_logger->log(logging_service::level_fatal, buffer, file, line, e.function());
                    }
                } catch (...) {}
                throw; // re-throw sdk exception
            }
            catch (const std::exception& e) // log and translate std::exception into sdk exception
            {
                try
                {
                    if (logger.m_logger->is_level_enabled(logging_service::level_fatal))
                    {
                        char buffer[2048] = "";
                        buffer[sizeof(buffer) - 1] = 0;
                        snprintf(buffer,2048,"%s",e.what());
                        logger.m_logger->log(logging_service::level_fatal, buffer, file, line, function_name);
                    }
                } catch (...) {}
                throw_exception(e.what(), function_name);
            }
            catch (...) // log and translate unknown exception into sdk exception
            {
                try
                {
                    if (logger.m_logger->is_level_enabled(logging_service::level_fatal))
                    {
                        char buffer[2048] = "";
                        buffer[sizeof(buffer) - 1] = 0;
                        snprintf(buffer,2048,"%s","unknown exception");
                        logger.m_logger->log(logging_service::level_fatal, buffer, file, line, function_name);
                    }
                } catch (...) {}
                throw_exception("unknown exception", function_name);
            }
        }
    }
}

/**
 * @brief Invoke function and translate any thrown exception into sdk exception.
 *
 * Translate std::exception or unknown exception into sdk exception
 * or re-throw sdk exception if it is a current one.
 * Std::exception and unknown exception are logged, sdk exception is supposed to be already logged by the sdk module, which function is invoked.
 * The macro is used to capture logging related data: the caller (calling function), the line number and the implementation file
 * and to pass this data to the translation function.
 * @param[in] _return_type(user-defined type)   Return type of the invoked function.
 * @param[in] _function                         User-specified function to invoke.
 *                                              The invoked function must be a callable object with defined operator() that has no parameters.
 */
#define INVOKE_FUNCTION_AND_TRANSLATE_EXCEPTION(_return_type, _function) rs::utils::invoke_function_and_translate_exception<_return_type>(_function, __FUNC_WITH_EXCEPTION__, __LINE__, __FILE__);

/**
 * @brief Invoke function and return error value if the function throws an exception.
 *
 * Invoke user-specified function and return error value if the exception is thrown during the function execution.
 * Any thrown exception is logged.
 * The macro is used to capture logging related data: the caller (calling function), the line number and the implementation file
 * and to pass this data to the safe invoke function.
 * @param[in] _return_type(user-defined type)   Return type of the invoked function.
 * @param[in] _returned_error_value             User-specified error value that is returned if exception is thrown.
 * @param[in] _function                         User-specified function to invoke.
 *                                              The invoked function must be a callable object with defined operator() that has no parameters.
 * @return: T(user-defined type)                Return value of the invoked function.
 * @return: _returned_error_value               The returned value if invoked function throws exception.
 */
#define SAFE_INVOKE_FUNCTION(_return_type, _returned_error_value, _function) \
    rs::utils::safe_invoke_function<_return_type, _returned_error_value>(_function, __FUNC_WITH_EXCEPTION__, __LINE__, __FILE__ );

