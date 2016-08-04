// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <ctime>
#include <vector>

using namespace std;

namespace rs
{
    namespace utils
    {
        class fps_util
        {
        public:
            /**
             * @brief fps_util
             * Default constructor.
             */
            fps_util();

            /**
             * @brief fps_util
             * Constructor specific for constant number of frames in the application.
             * Used to omit the usage of unnecessary methods in the source code.
             * @param[in] number_of_frames  Number of frames that is used in fps measurement
             */
            fps_util(int number_of_frames);

            /**
             * @brief add_time
             * Store time value to further calculate fps.
             */
            void add_time();

            /**
             * @brief tick
             * Increment amount of frames in a loop.
             */
            void tick();

            /**
             * @brief calculate_fps
             * Calculate fps based on frames number and time values stored.
             * @param[out] process_fps      Fps value. -1 if calculation is not possible
             */
            void calculate_fps(double& process_fps) const;

        private:
            int                   m_frames = 0; /**< number of frames */
            bool                  m_is_frames_number_const = false;
            std::vector<timespec> m_time_values; /**< time values vector */
        };
    }
}
