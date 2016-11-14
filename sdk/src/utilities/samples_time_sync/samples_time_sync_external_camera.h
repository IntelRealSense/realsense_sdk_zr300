// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.
#pragma once

#include <memory>
#include <iostream>
#include <librealsense/rs.hpp>
#include <list>
#include <mutex>
#include <map>
#include "rs_sdk.h"

#include "samples_time_sync_base.h"

namespace rs
{
    namespace utils
    {
        class samples_time_sync_external_camera : public samples_time_sync_base
        {
        public:
            samples_time_sync_external_camera(int streams_fps[static_cast<int>(rs::core::stream_type::max)],
                                  int motions_fps[static_cast<int>(rs::core::motion_type::max)],
                                  unsigned int max_input_latency,
                                  unsigned int not_matched_frames_buffer_size) :
                samples_time_sync_base(streams_fps, motions_fps, max_input_latency, not_matched_frames_buffer_size) {}

            virtual ~samples_time_sync_external_camera() {}

        protected:
            virtual bool sync_all(streams_map& streams, motions_map& motions, rs::core::correlated_sample_set &sample_set) override;

        };
    }
}

