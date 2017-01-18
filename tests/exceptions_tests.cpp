// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <memory>
#include <exception>

#include <gtest/gtest.h>

#include "rs_core.h"
#include "src/utilities/exception_utils/exceptions_translation.h"
#include "utilities/utilities.h"

using namespace rs::core;
using namespace rs::utils;

namespace
{
    static const char EXCEPTION_MESSAGE[] = "test";
}


// throw sdk exception
TEST(exceptions_tests, throw_sdk_exception_test) try
{
    // scenario:
    // throw exception inside - catch on the test(app) level.
    // without logging.

    try
    {
        THROW_EXCEPTION(EXCEPTION_MESSAGE);
        GTEST_FAIL() << "throw_exception didn't throw";
    }
    catch (const rs::core::exception& e)
    {
        ASSERT_EQ(std::string(e.what()), EXCEPTION_MESSAGE);
        ASSERT_TRUE(!std::string(e.function()).empty());
    }
}CATCH_SDK_EXCEPTION()
TEST(exceptions_tests, throw_and_log_sdk_exception_test) try
{
    // scenario:
    // throw exception inside - catch on the test(app) level.
    // with logging.

    try
    {
        THROW_EXCEPTION_AND_LOG(EXCEPTION_MESSAGE);
        GTEST_FAIL() << "throw_exception didn't throw";
    }
    catch (const rs::core::exception& e)
    {
        ASSERT_EQ(std::string(e.what()), EXCEPTION_MESSAGE);
        ASSERT_TRUE(!std::string(e.function()).empty());
    }
}CATCH_SDK_EXCEPTION()


// exception translation mechanism
TEST(exceptions_tests, translate_sdk_exception_test) try
{
    // scenario:
    // throw sdk exception, use exception translation mechanism - catch on the test(app) level.

    try
    {
        INVOKE_FUNCTION_AND_TRANSLATE_EXCEPTION(void, [] { THROW_EXCEPTION_AND_LOG(EXCEPTION_MESSAGE); });
        GTEST_FAIL() << "exception translation function didn't throw";
    }
    catch (const rs::core::exception& e)
    {
        ASSERT_EQ(std::string(e.what()), EXCEPTION_MESSAGE);
        ASSERT_TRUE(!std::string(e.function()).empty());
    }
}CATCH_SDK_EXCEPTION()
TEST(exceptions_tests, translate_std_exception_test) try
{
    // scenario:
    // throw std::exception, use exception translation mechanism - catch on the test(app) level.

    try
    {
        INVOKE_FUNCTION_AND_TRANSLATE_EXCEPTION(void, [] { throw std::runtime_error(EXCEPTION_MESSAGE); });
        ADD_FAILURE() << "exception translation function didn't throw: on throw std::runtime_error";
    }
    catch (const rs::core::exception& e)
    {
        ASSERT_EQ(std::string(e.what()), EXCEPTION_MESSAGE);
        ASSERT_TRUE(!std::string(e.function()).empty());
    }

    try
    {
        const int value_on_success = INVOKE_FUNCTION_AND_TRANSLATE_EXCEPTION(int, [] { std::vector<int> a; return a.at(1); });
        ADD_FAILURE() << "exception translation function didn't throw: on internal throw in std::vector::at()";
    }
    catch (const rs::core::exception& e)
    {
        ASSERT_TRUE(!std::string(e.what()).empty());
        ASSERT_TRUE(!std::string(e.function()).empty());
    }
}CATCH_SDK_EXCEPTION()
TEST(exceptions_tests, translate_unknown_exception_test) try
{
    // scenario:
    // throw unknown exception, use exceptions translation mechanism - catch on the test(app) level.

    try
    {
        INVOKE_FUNCTION_AND_TRANSLATE_EXCEPTION(void, [] { throw EXCEPTION_MESSAGE; });
        GTEST_FAIL() << "exception translation function didn't throw";
    }
    catch (const rs::core::exception& e)
    {
        ASSERT_TRUE(!std::string(e.what()).empty());
        ASSERT_TRUE(!std::string(e.function()).empty());
    }
}CATCH_SDK_EXCEPTION()


// safe invoke mechanism
TEST(exceptions_tests, safe_invoke_sdk_exception_test) try
{
    // scenario:
    // throw sdk exception, use safe invoke to wrap exception - return invalid status on the test(app) level.

    try
    {
        rs::core::status sts = SAFE_INVOKE_FUNCTION(rs::core::status, rs::core::status::status_exec_aborted,
                                                    [] { THROW_EXCEPTION_AND_LOG(EXCEPTION_MESSAGE); return rs::core::status::status_no_error; });
        ASSERT_EQ(rs::core::status::status_exec_aborted, sts);
    }
    catch(...)
    {
        GTEST_FAIL() << "safe invoke threw an exception";
    }
}CATCH_SDK_EXCEPTION()
TEST(exceptions_tests, safe_invoke_std_exception_test) try
{
    // scenario:
    // throw std::exception, use safe invoke to wrap exception - return invalid status on the test(app) level.

    try
    {
        rs::core::status sts = SAFE_INVOKE_FUNCTION(rs::core::status, rs::core::status::status_exec_aborted,
                                                    [] { throw std::runtime_error(EXCEPTION_MESSAGE); return rs::core::status::status_no_error; });
        ASSERT_EQ(rs::core::status::status_exec_aborted, sts);
    }
    catch(...)
    {
        GTEST_FAIL() << "safe invoke threw an exception";
    }
}CATCH_SDK_EXCEPTION()
TEST(exceptions_tests, safe_invoke_unknown_exception_test) try
{
    // scenario:
    // throw unknown exception, use safe invoke to wrap exception - return invalid status on the test(app) level.

    try
    {
        rs::core::status sts = SAFE_INVOKE_FUNCTION(rs::core::status, rs::core::status::status_exec_aborted,
                                                    [] { throw EXCEPTION_MESSAGE; return rs::core::status::status_no_error; });
        ASSERT_EQ(rs::core::status::status_exec_aborted, sts);
    }
    catch (...)
    {
        GTEST_FAIL() << "safe invoke threw an exception";
    }
}CATCH_SDK_EXCEPTION()
