// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.


/** 
* \file fps_counter.h
* @brief Describes the \c rs::utils::fps_counter class.
*/

#pragma once
#include <mutex>
#include <deque>
#include <chrono>

namespace rs
{
    namespace utils
    {
        /**
         * @brief Provides a common way to measure FPS, regardless of the context it is used in.
         *
         * The \c fps_counter uses a fixed size buffer to store time values so there should not be any impactful memory allocations (for example, buffer resize)
         * after approximately 2 seconds of streaming.
		 *
         * Try to refrain from using time consuming operations (for example, using output stream or allocating large memory chunks) unless it is required,
         * as they can reduce the general performance of the code block which can impact the FPS counting.
		 *
         * The \c fps_counter uses a monotonic clock (with its features) to store time values,
         * however, this clock may have different precision that depends on system, STL implementation.
		 * 
         * Time values stored have nanoseconds precision by default.
         */
        class fps_counter
        {
        public:
            /**
             * @brief Creates an instance of \c fps_counter.
             *
             * Creates an instance with a device stream frame rate specified (that can be acquired by using \c get_framerate() or similar).
             * The frame rate value is used to define the internal buffer size. The frame rate is multiplied by some magic number which helps to
             * lessen the impact of small delays (for example, system-specific delays, intermittent rendering overheads).
			 
             * To count FPS for each stream separately, create one instance per stream and stream from device independently.
			 
             * To count FPS for the whole streaming, regardless of stream amount, create only one instance and specify the stream highest frame rate
             * to have proper FPS counting.
             * @param[in] frame_rate Frame rate value requested for stream (for example, color stream frame rate)
             */
            fps_counter(unsigned int frame_rate) :
                m_time_buffer_max_size(static_cast<size_t>(1.3 * frame_rate))
                // Coefficient is a magic number to balance between better measurement and smaller time interval of getting proper results
                // as valid value of fps will be counted approx. after [1 sec * coefficient] seconds
            {}

            /**
             * @brief Captures an event of frame arrival.
             *
             * Call \c tick() on frame arrival during processing (for example, rendering).
			 *
             * The calculated FPS reflects the ticks per second which were indicated by the user through this method.
             * The buffer is used later to provide time values to count current or total FPS.
             * The method is the main method for the whole FPS counting and it is mandatory to call it for proper calculations.
             * The first few frames are skipped to avoid jitters of the streams at the beginning of the streaming.
             * The method processing is guaranteed to be short to prevent FPS changes due to measurement.
             */
            void tick()
            {
                if (m_skip_first_frames != 0) m_skip_first_frames--; // skip first frames as they may be incorrectly processed(for example, assume some buffer allocations)
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
             * @brief Calculates average FPS throughout the entire streaming session between first and last ticks.
             *
             * The returned value is the total average FPS for some process (for example, streaming, rendering) based on 
			 * frames count and elapsed time between first and last \c tick() calls.
             * The method is the main method to get total FPS.
			 *
             * A valid FPS is expected to be available after <tt>[1 sec * buffer_size / stream frame rate]</tt> seconds. 
			 * Before that period elapses, the average FPS is unpredictable.
             * The method processing is guaranteed to be short to prevent FPS changes due to measurement.
             * @return const double Total average FPS
             */
            const double total_average_fps()
            {
                std::lock_guard<std::mutex> lock(m_time_values_mutex);
                if (!m_first_time_value_set)
                {
                    throw std::out_of_range("No time values were stored with tick()");
                }
                const long billion = 1000000000;
                const double time_delta = static_cast<double>(
                        (long double)(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                          m_time_buffer.back() - m_first_time_value).count()) / billion);
                return (static_cast<double>(m_frames - 1) / time_delta);
            }

            /**
             * @brief Returns the last second average FPS.
             *
             * The returned value is the last second average FPS for the utility user based on the ticks data that was provided.
             * The method is the main method to get last second FPS.
             * A valid FPS is expected to be available after <tt>[1 sec * buffer_size / stream frame rate]</tt> seconds. 
			 * Before that period elapses, the average FPS is unpredictable.
             * The method processing is guaranteed to be short to prevent FPS changes due to measurement.
             * @return const double Current average FPS
             */
            const double current_fps()
            {
                std::lock_guard<std::mutex> lock(m_time_values_mutex);
                const size_t time_buffer_size = m_time_buffer.size();
                if (!time_buffer_size) return 0;
                const long billion = 1000000000;
                const double time_delta = static_cast<double>(
                        (long double)(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                          m_time_buffer.back() - m_time_buffer.front()).count()) / billion);
                if (time_delta == 0) return 0;
                return (static_cast<double>(time_buffer_size - 1) / time_delta);
            }


        private:
            fps_counter() = delete;
            fps_counter(const fps_counter&) = delete;
            const fps_counter& operator=(const fps_counter&) = delete;


            int                    m_frames = 0; /**< number of frames */

            //typedef for shorter usage
            typedef std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> time_point;
            std::deque<time_point> m_time_buffer; /**< time values buffer with nanosecond precision */
            time_point             m_first_time_value; /**< first time value to calculate total average FPS */

            const size_t           m_time_buffer_max_size; /**< max supposed size of time values buffer */
            bool                   m_first_time_value_set = false; /**< flag for the first time value */
            int                    m_skip_first_frames = 5; /**< number of possibly invalid frames at stream start */
            std::mutex             m_time_values_mutex; /**< mutex for time values */
        };
    }
}
