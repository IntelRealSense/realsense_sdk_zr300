// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.
/**
 * @file exception_utils.h
 *
 * Defines sdk exception that can be thrown by each sdk module and the mechanism to transform any exception into
 * sdk exception as well as log exceptions via logging utility.
 */
#pragma once

namespace rs
{
    namespace core
    {
        /**
         * @class exception
         * @brief The exception class is the base class for all sdk exceptions.
         *
         * The class provides a common way to work with sdk exceptions.
         * The derived class instances are thrown by sdk modules.
         * Standard or unknown exceptions are translated into sdk exceptions inside the sdk modules.
         * Catch this exception to look for sdk errors when working with sdk.
         */
        class exception
        {
        public:
            /**
             * @brief Return error description.
             *
             * Error description may specify std exception error when
             * std exception is translated into an instance of sdk exception.
             * Unknown exception may be returned when
             * neither std exception is translated nor sdk exception is re-thrown.
             * @return: const char*     Error description.
             */
            virtual const char* what() const = 0;

            /**
             * @brief Return function name from which the throw was called.
             *
             * Function should always specify the name of the original function
             * from which the throw was called either implicitly through a defined macro
             * or explicitly.
             * @return: const char*     Calling function name.
             */
            virtual const char* function() const = 0;
        };
    }
}
