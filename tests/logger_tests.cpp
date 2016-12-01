// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "gtest/gtest.h"
#include "rs_sdk.h"

GTEST_TEST(LoggerTests, logger_configured_test)
{
    ASSERT_NE(LOGGER_TYPE,rs::utils::logging_service::logger_type::EMPTY_LOGGER) << "Logger .so file is not loaded, or logger configuration failure.";
}
