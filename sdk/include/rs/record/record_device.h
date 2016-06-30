// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense/rs.hpp>

namespace rs
{
    namespace record
    {
        /**
        This class provides rs::device capablities with extentions of record capablities.
        */
        class device : public rs::device
        {
        public:
            /**
            @brief Pause recording.
            */
            void pause_record();
            /**
            @brief Resume recording.
            */
            void resume_record();
        };
    }
}
