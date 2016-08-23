// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <mutex>
#include <deque>
#include <chrono>

namespace rs
{
    namespace utils
    {
        class fps_counter
        {
        public:
            /**
             * @brief fps_counter
             * Fps measurement class constructor.
             * @param[in] frame_rate        Frame rate value requested for stream(e.g. color)
             */
            fps_counter(unsigned int frame_rate) :
                m_time_buffer_max_size(1.3 * frame_rate) // multiplying initial frame_rate value to lessen the impact of small delays
                // Coefficient is a magic number to balance between better measurement and smaller time interval of getting proper results
                // as adequate value of fps will be counted approx. after [1 sec * coefficient] seconds
            {}

            /**
             * @brief tick
             * Store current time value in buffer.
             * Call the function each time in a loop to populate buffer and count fps.
             */
            void tick()
            {
                if (m_skip_first_frames != 0) m_skip_first_frames--; // skip first frames as they may be incorrectly processed(e.g. assume some buffer allocations)
                else
                {
                    std::lock_guard<std::mutex> lock(m_time_values_mutex);
                    time_point time_value = std::chrono::steady_clock::now();
                    if (m_time_buffer.size() == (m_time_buffer_max_size))
                        m_time_buffer.pop_front();
                    m_time_buffer.push_back(time_value);
                    m_frames++;

                    if (!m_first_time_value_set)
                    {
                        m_first_time_value = time_value;
                        m_first_time_value_set = true;
                    }
                }
            }

            /**
             * @brief total_average_fps
             * Calculate fps based on frames number and elapsed time between first and last tick() calls.
             * @return                      Total average fps
             */
            const double total_average_fps()
            {
                std::lock_guard<std::mutex> lock(m_time_values_mutex);
                if (!m_first_time_value_set)
                {
                    throw std::out_of_range("No time values were stored with tick()");
                }
                const long billion = 1000000000;
                const double time_delta =
                        (long double)(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                          m_time_buffer.back() - m_first_time_value).count()) / billion;
                return (double(m_frames) / time_delta);
            }

            /**
             * @brief current_fps
             * Calculate current(last second) average fps.
             * @return                      Current average fps
             */
            const double current_fps()
            {
                std::lock_guard<std::mutex> lock(m_time_values_mutex);
                const int time_buffer_size = m_time_buffer.size();
                if (!time_buffer_size) return 0;
                const long billion = 1000000000;
                const double time_delta =
                        (long double)(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                          m_time_buffer.back() - m_time_buffer.front()).count()) / billion;
                if (time_delta == 0) return 0;
                return (double(time_buffer_size) / time_delta);
            }


        private:
            fps_counter() = delete;


            int                    m_frames = 0; /**< number of frames */

            typedef std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> time_point; //typedef for short
            std::deque<time_point> m_time_buffer; /**< time values buffer */
            time_point             m_first_time_value; /**< first time value to calculate total average fps */

            const size_t           m_time_buffer_max_size; /**< max supposed size of time values buffer */
            bool                   m_first_time_value_set = false; /**< flag for the first time value */
            int                    m_skip_first_frames = 5; /**< number of possibly invalid frames at stream start */
            std::mutex             m_time_values_mutex; /**< mutex for time values */
        };
    }
}
