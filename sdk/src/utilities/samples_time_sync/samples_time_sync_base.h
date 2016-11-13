// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <memory>
#include <iostream>
#include <librealsense/rs.hpp>
#include <list>
#include <mutex>
#include <map>

#include "rs_sdk.h"
#include "rs/utils/cyclic_array.h"

#pragma once

namespace rs
{
    namespace utils
    {
        class samples_time_sync_base : public release_self_base<samples_time_sync_interface>
        {
        public:
            samples_time_sync_base(int streams_fps[static_cast<int>(rs::core::stream_type::max)],
                                    int motions_fps[static_cast<int>(rs::core::motion_type::max)],
                                    unsigned int max_input_latency,
                                    unsigned int not_matched_frames_buffer_size);

            virtual bool insert(rs::core::image_interface * new_image, rs::core::correlated_sample_set& sample_set) override;

            virtual bool insert(rs::core::motion_sample& new_motion, rs::core::correlated_sample_set& sample_set) override;

            virtual bool get_not_matched_frame(rs::core::stream_type stream_type, rs::core::image_interface ** not_matched_frame) override;

            virtual void flush() override;

            virtual ~samples_time_sync_base() {}

        protected:

            samples_time_sync_base& operator=(const samples_time_sync_base&) = delete;
            samples_time_sync_base(const samples_time_sync_base&) = delete;

            std::map<rs::core::stream_type, rs::utils::cyclic_array<rs::utils::unique_ptr<rs::core::image_interface>>>    m_stream_lists;
            std::map<rs::core::motion_type, rs::utils::cyclic_array<rs::core::motion_sample>>                       m_motion_lists;

            std::map<rs::core::stream_type, rs::utils::cyclic_array<rs::utils::unique_ptr<rs::core::image_interface>>>    m_stream_lists_dropped_frames;

            bool empty_list_exists();

            virtual bool sync_all(rs::core::correlated_sample_set &sample_set) = 0;

            void pop_or_save_to_not_matched(rs::core::stream_type st_type);

            inline bool is_stream_registered(rs::core::stream_type stream) { return m_streams_fps[static_cast<int>(stream)] != 0; }
            inline bool is_motion_registered(rs::core::motion_type motion) { return m_motions_fps[static_cast<int>(motion)] != 0; }

            std::mutex m_image_mutex;
            std::mutex m_dropped_images_mutex;

            unsigned int m_max_input_latency;
            unsigned int m_not_matched_frames_buffer_size;

            int m_streams_fps[static_cast<int>(rs::core::stream_type::max)];
            int m_motions_fps[static_cast<int>(rs::core::motion_type::max)];

            int m_highest_fps;   // store the highest fps of all streams (not motions)
            double m_max_diff;   // while searching for the closest match between two different frames
                                 // the difference in timestamp should not be karger than max_diff
                                 // which is half of the period of stream with highest fps (in ms)
        };
    }
}

