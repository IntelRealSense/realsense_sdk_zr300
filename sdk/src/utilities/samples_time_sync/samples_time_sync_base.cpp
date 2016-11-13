// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "samples_time_sync_base.h"
#include <algorithm>


using namespace std;
using namespace rs::core;
using namespace rs::utils;

rs::utils::samples_time_sync_base::samples_time_sync_base(int streams_fps[],
                                                            int motions_fps[],
                                                            unsigned int max_input_latency,
                                                            unsigned int not_matched_frames_buffer_size) :
    m_highest_fps(0), m_not_matched_frames_buffer_size(not_matched_frames_buffer_size)
{
    LOG_FUNC_SCOPE();

    int registered_streams=0;

    if (max_input_latency == 0)
    {
        LOG_ERROR("Zero latency is not acceptable");
        throw std::invalid_argument("Zero latency is not acceptable");
    }


    // Save fps's for each stream and motion requested
    for (int i = 0; i<static_cast<int>(stream_type::max); i++)
    {
        m_streams_fps[i] = streams_fps[i];

        if (streams_fps[i] == 0 )
            continue;

        m_highest_fps = std::max(m_highest_fps, streams_fps[i]);

        registered_streams++;

        int buffer_length = streams_fps[i] * max_input_latency / 1000;

        if (buffer_length == 0)
            buffer_length = 1;

        m_stream_lists.insert(std::make_pair(static_cast<stream_type>(i), cyclic_array<rs::utils::unique_ptr<image_interface>>(buffer_length)));

        if (m_not_matched_frames_buffer_size != 0)
            m_stream_lists_dropped_frames.insert(std::make_pair(static_cast<stream_type>(i), cyclic_array<rs::utils::unique_ptr<image_interface>>(m_not_matched_frames_buffer_size)));

        LOG_DEBUG("For stream " << i << " with fps " << streams_fps[i] << " using buffer length " << buffer_length);

    }

    m_max_diff = (double)1000 / m_highest_fps / 2;

    for (int i=0; i<static_cast<int>(motion_type::max); i++)
    {
        m_motions_fps[i] = motions_fps[i];

        if (motions_fps[i] == 0 )
            continue;

        registered_streams++;

        int buffer_length = motions_fps[i] * max_input_latency / 1000;

        if (buffer_length == 0)
            buffer_length = 1;

        LOG_DEBUG("For stream " << i << " with fps " << motions_fps[i] << " using buffer length " << buffer_length);

        m_motion_lists.insert(std::make_pair(static_cast<motion_type>(i), cyclic_array<motion_sample>(buffer_length)));
    }

    if (registered_streams < 2)
        throw std::invalid_argument("Less than two streams were registered to sync utility!");
}

bool rs::utils::samples_time_sync_base::empty_list_exists()
{
    for (auto& item : m_stream_lists)
    {
        if (item.second.size() == 0)
            return true;
    }

    for (auto& item : m_motion_lists)
    {
        if (item.second.size() == 0)
            return true;
    }

    return false;

}

void rs::utils::samples_time_sync_base::pop_or_save_to_not_matched(stream_type st_type)
{
    if (m_not_matched_frames_buffer_size!=0)
            m_stream_lists_dropped_frames[st_type].push_back( m_stream_lists[st_type].front());

    m_stream_lists[st_type].pop_front();
}

bool rs::utils::samples_time_sync_base::insert(image_interface * new_image,
                                     rs::core::correlated_sample_set& correlated_sample)
{
    if (!new_image)
        throw std::invalid_argument("Null pointer received!");
    new_image->add_ref();
    auto new_unique_image = get_unique_ptr_with_releaser(new_image);

    auto stream_type = new_unique_image->query_stream_type();
    // this stream type was not registered with this instance of the sync_utility
    if (!is_stream_registered(stream_type))
        throw std::invalid_argument("Stream was not registered to this sync utility instance!");

    std::lock_guard<std::mutex> lock_guard(m_image_mutex);

    m_stream_lists[stream_type].push_back(new_unique_image);

    // return synced color and depth
    return sync_all(correlated_sample);
}


bool rs::utils::samples_time_sync_base::insert(rs::core::motion_sample& new_motion, rs::core::correlated_sample_set& correlated_sample)
{
    if (!is_motion_registered(new_motion.type))
        throw std::invalid_argument("Stream was not registered to this sync utility instance!");

    std::lock_guard<std::mutex> lock_guard(m_image_mutex);

    m_motion_lists[new_motion.type].push_back(new_motion);

    return sync_all(correlated_sample);
}

bool rs::utils::samples_time_sync_base::get_not_matched_frame(rs::core::stream_type stream_type, image_interface **not_matched_frame)
{
    if (!not_matched_frame)
        throw std::invalid_argument("Null pointer received");

    *not_matched_frame = nullptr;

    if (m_not_matched_frames_buffer_size == 0 || !is_stream_registered(stream_type))
        return false;

    std::lock_guard<std::mutex> lock(m_dropped_images_mutex);

    if (m_stream_lists_dropped_frames[stream_type].size() == 0 )
        return false;

    auto raw_image = m_stream_lists_dropped_frames[stream_type].front().get();

    raw_image->add_ref();
    *not_matched_frame = raw_image;

    m_stream_lists_dropped_frames[stream_type].pop_front();

    if (m_stream_lists_dropped_frames[stream_type].size() == 0 )
        return false;

    return true;

}

void rs::utils::samples_time_sync_base::flush()
{
    std::lock_guard<std::mutex> lock_guard(m_image_mutex);
    //remove all frames from all lists
    for (auto& stream_list : m_stream_lists)
    {
        while (stream_list.second.size())
            stream_list.second.pop_front();
    }


    std::lock_guard<std::mutex> lock(m_dropped_images_mutex);
    //remove all frames from all lists
    for (auto& stream_list : m_stream_lists_dropped_frames)
    {
        while (stream_list.second.size())
            stream_list.second.pop_front();
    }

}



