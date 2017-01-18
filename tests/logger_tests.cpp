// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "gtest/gtest.h"
#include "rs_sdk.h"
#include "utilities/utilities.h"

GTEST_TEST(LoggerTests, logger_configured_test) try
{
    ASSERT_NE(LOGGER_TYPE,rs::utils::logging_service::logger_type::empty_logger) << "Logger .so file is not loaded, or logger configuration failure.";
}CATCH_SDK_EXCEPTION()
