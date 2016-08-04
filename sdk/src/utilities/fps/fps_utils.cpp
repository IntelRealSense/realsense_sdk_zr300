// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <iostream>

#include "fps_utils.h"

namespace rs
{
    namespace utils
    {
        fps_util::fps_util()
        {}

        fps_util::fps_util(int number_of_frames) :
            m_frames(number_of_frames)
        {
            m_is_frames_number_const = true;
        }

        void fps_util::add_time()
        {
            timespec time_value;
            clock_gettime(CLOCK_BOOTTIME, &time_value);
            m_time_values.push_back(time_value);
        }

        void fps_util::tick()
        {
            if (!m_is_frames_number_const) m_frames++;
        }

        void fps_util::calculate_fps(double& process_fps) const
        {
            int time_vector_size = m_time_values.size();
            if (time_vector_size <= 1)
            {
                std::cerr << "\nNot enough time values added to calculate fps" << std::endl;
                process_fps = -1;
                return;
            }
            const long billion = 1000000000;
            uint64_t process_time_sec = billion * (m_time_values[time_vector_size - 1].tv_sec - m_time_values[0].tv_sec)
                                        + m_time_values[time_vector_size - 1].tv_nsec - m_time_values[0].tv_nsec;
            process_time_sec /= billion; // convert to seconds
            process_fps = m_frames/process_time_sec;
        }
    }
}
