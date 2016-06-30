// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include <sstream>
#include "sdk_version.h"

namespace rs
{
    namespace core
    {
        namespace sdk_utils
        {
            static std::string get_sdk_version()
            {
                std::stringstream ss;
                ss << SDK_VER_MAJOR << "." <<
                   SDK_VER_MINOR << "." <<
                   SDK_VER_COMMIT_NUMBER << "." << std::hex << "0x" <<
                   SDK_VER_COMMIT_ID;
                return ss.str();
            }
        }
    }
}
