// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <memory>
#include <iostream>
#include <librealsense/rs.hpp>
#include <list>
#include <mutex>

#include "rs_sdk.h"
#include "rs/utils/cyclic_array.h"

using namespace rs::core;
using namespace std;

namespace rs
{
    namespace utils
    {
        class sync_utility
        {
        public:
            /**
            @brief Create and initialize the sync utility - register streams and motions that are required to be synced.
            @param[in]  streams                   Vector of pairs <stream type, frame per second> for every stream needed to be registered.
            @param[in]  motions                   Vector of pairs <motion type, frame per second> for every motion needed to be registered.
            @param[in]  max_input_latency         The maximum latency in millisecinds that is allowed to be when receiving two frames from
                                                  different streams with same timestamp. Defines the number of frames to be stored in sync
                                                  utility. Increasing this value will cause a larger number of buffered images.
            */
            sync_utility(vector<pair<stream_type, unsigned int>>& streams, vector<pair<motion_type, unsigned int>>& motions, unsigned int max_input_latency = 100 );

            /**
            @brief inserts the new image to the sync utility. Returns true if the correlated sample was found.
            @param[in]  new_image                 New image.
            @param[out] sample_set                Correlated sample containing correlated images and/or motions. May be empty.
            @return                               true if the match was found
            */
            bool insert(rs::utils::smart_ptr<image_interface> new_image, rs::core::correlated_sample_set& sample_set);

            /**
            @brief inserts the new motion to the sync utility. Returns true if the correlated sample was found.
            @param[in]  new_motion                New motion.
            @param[out] sample_set                Correlated sample containing correlated images and/or motions. May be empty.
            @return                               true if the match was found            */
            bool insert(rs::core::motion_sample new_motion, rs::core::correlated_sample_set& sample_set);


            ~sync_utility();
        private:

            map<stream_type, unsigned int> m_stream_fps;
            map<motion_type, unsigned int> m_motion_fps;

            map<stream_type, rs::utils::cyclic_array<smart_ptr<image_interface>>>    m_stream_lists;
            map<motion_type, rs::utils::cyclic_array<motion_sample>>                 m_motion_lists;

            bool sync_color_and_depth( rs::core::correlated_sample_set& sample_set );

            bool is_stream_registered(stream_type);
            bool is_motion_registered(motion_type);

            std::mutex m_image_mutex;

            unsigned int m_max_input_latency;
        };
    }
}

