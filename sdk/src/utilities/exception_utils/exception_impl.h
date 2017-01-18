// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs/core/exception.h"
#include "cstring"

namespace rs
{
    namespace utils
    {
        class exception_impl : public rs::core::exception
        {
            exception_impl() = delete;
            exception_impl& operator=(const exception_impl&) = delete;

        public:
            /**
             * @brief Create an exception with error message and function name from which the throw was called.
             *
             * Exception instance is constructed on throw call.
             * Constructor takes character strings for error description and function name.
             * Catch the instance to query error message and function from which the throw was called.
             * There is no need to catch any additional exceptions from the sdk modules
             * as std::exception and unknown exception should be transformed into the instance of this class.
             * @param[in] message       Message that provides error description.
             * @param[in] function      Calling function name from which the exception was thrown.
             */
            exception_impl(const char* message, const char* function) noexcept(true)
            {
                memset(m_message, 0, m_exception_message_length);
                const size_t message_length = strlen(message) > m_exception_message_length ? m_exception_message_length : strlen(message);
                memcpy(m_message, message, message_length);

                memset(m_function, 0, m_exception_message_length);
                const size_t function_length = strlen(function) > m_exception_message_length ? m_exception_message_length : strlen(function);
                memcpy(m_function, function, function_length);
            }

            /**
             * @brief Return error description.
             *
             * Error description may specify std::exception error when instance of sdk exception
             * was constructed from std::exception.
             * Unknown exception may be returned when instance of sdk exception neither was constructed
             * from std::exception nor was rethrown in sdk module.
             * @return: const char*     Error description.
             */
            virtual const char* what() const noexcept(true) override
            {
                return m_message;
            }

            /**
             * @brief Return function name from which the throw was called.
             *
             * Function should always specify the name of the original function
             * from which the throw was called either implicitly through a defined macro
             * or explicitly.
             * @return: const char*     Calling function name.
             */
            virtual const char* function() const noexcept(true) override
            {
                return m_function;
            }

        private:
            static const int m_exception_message_length = 256; /**< max length of the message returned by what() and function() */
            char m_message[m_exception_message_length]; /**< message that provides error description */
            char m_function[m_exception_message_length]; /**< name of the function from which the exception was thrown */
        };


#ifndef __NO_RETURN__
# if defined(__GNUC__)
#  define __NO_RETURN__ __attribute__((__noreturn__))
# elif defined(_MSC_VER)
#  define __NO_RETURN__ __declspec(noreturn)
# else
#  define __NO_RETURN__
# endif
#endif
        /**
         * @brief Throw sdk exception with specified error description and function name.
         *
         * Use the macro instead to throw sdk exception.
         * The method is a wrapper to call throw with sdk exception.
         * Only use this method if there is a need to define function name manually
         * and no need to log the error.
         * @param[in] message           Error description.
         * @param[in] function          Calling function name.
         */
        __NO_RETURN__ static void throw_exception(const char* message, const char* function)
        {
            throw rs::utils::exception_impl(message, function);
        }
    }
}


#ifndef __FUNC_WITH_EXCEPTION__
# if defined(__GNUC__)
#  define __FUNC_WITH_EXCEPTION__ __PRETTY_FUNCTION__
# elif defined(__FUNCSIG__)
#  define __FUNC_WITH_EXCEPTION__ __FUNCSIG__
# else
#  define __FUNC_WITH_EXCEPTION__ __FUNCTION__
# endif
#endif

/** @brief Throw sdk exception.
 *
 * Use this macro to throw sdk exception. The error is not logged.
 * The macro is used to capture logging data: calling function.
 * @param[in] _message              Error description.
 */
#define THROW_EXCEPTION(_message) rs::utils::throw_exception(_message, __FUNC_WITH_EXCEPTION__);

/**
 * @brief Log and throw sdk exception.
 *
 * Use this macro throw sdk exception.
 * The error should be logged if there are no errors on the side of the logger.
 * The macro is used to capture logging data: calling function and to correctly log the error.
 * @param[in] _message              Error description.
 */
#define THROW_EXCEPTION_AND_LOG(_message) { try { LOG_FATAL_CFORMAT("%s", _message); } catch (...) {}; \
    rs::utils::throw_exception(_message, __FUNC_WITH_EXCEPTION__); }


