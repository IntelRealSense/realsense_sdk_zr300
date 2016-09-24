// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "samples_time_sync_zr300.h"
#include <algorithm>


using namespace std;
using namespace rs::core;
using namespace rs::utils;

rs::utils::samples_time_sync_zr300::samples_time_sync_zr300(int streams_fps[],
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

bool rs::utils::samples_time_sync_zr300::sync_all( rs::core::correlated_sample_set& sample_set )
{
    bool set_found = false;

    if (empty_list_exists())
        return false;

    double largest_timestamp;

    while (!set_found)
    {
        set_found = true;

        largest_timestamp = -1;

        // first pass - find largest (latest) timestamp;
        for (auto& stream_list : m_stream_lists)
        {
            // skip fish_eye stream since its timestamp is not the same as all other streams
            if (stream_list.first == stream_type::fisheye)
                continue;

            if (largest_timestamp < stream_list.second.front()->query_time_stamp() )
                largest_timestamp = stream_list.second.front()->query_time_stamp();
        }

        //second pass - eliminate the frames with the timstamps earlier than largest timistamp
        for (auto& stream_list : m_stream_lists)
        {
            // skip fish_eye stream since its timestamp is not the same as all other streams
            if (stream_list.first == stream_type::fisheye)
                continue;

            while ( stream_list.second.size() > 0 && stream_list.second.front()->query_time_stamp() < largest_timestamp)
                pop_or_save_to_not_matched(stream_list.first);

            // now the list may become empty - in this case - correleated sample can not be found - just return false
            if (stream_list.second.size() == 0)
                return false;

            // if timestamp of the head of the list is NOT equal to the found maximum = go for the next iteration
            set_found = set_found && (largest_timestamp == stream_list.second.front()->query_time_stamp());
        }


        if (is_stream_registered(stream_type::fisheye) && set_found)
        {
            while (largest_timestamp - m_stream_lists[stream_type::fisheye].front()->query_time_stamp() > m_max_diff )
            {
                pop_or_save_to_not_matched(stream_type::fisheye);

                if (m_stream_lists[stream_type::fisheye].size() == 0)
                    return false;
            }

            if (largest_timestamp - m_stream_lists[stream_type::fisheye].front()->query_time_stamp() < (-1 * m_max_diff) )
            {
                //remove heads of all streams, except fish_eye - these will not be matched to any fisheye frame
                for (auto& stream_l : m_stream_lists)
                {
                    // skip fish_eye stream since its timestamp is not the same as all other streams
                    if (stream_l.first == stream_type::fisheye)
                        continue;

                    pop_or_save_to_not_matched(stream_l.first);

                    // return false if any stream turns to be empty
                    if (stream_l.second.size()==0)
                        return false;
                }

                //force next iteration of while(!set_found)
                set_found = false;
            }
        } // end of fish_eye

    } //end of while (!set_found)

    // at this point, head of all lists have frames with the same/closest timestamp
    for (auto& stream_list : m_stream_lists)
    {
        //setting the image in the output sample set, adding ref count because on pop_front the shared ptr will call release
        stream_list.second.front()->add_ref();
        sample_set[stream_list.first] = stream_list.second.front().get();
        stream_list.second.pop_front();
    }

    //pick up corresponding motions
    for (auto& motion_list : m_motion_lists)
    {
        sample_set[motion_list.first] = motion_list.second.front();
        motion_list.second.pop_front();

        while(motion_list.second.size() > 0)
        {
            auto a1 = abs(largest_timestamp-sample_set[motion_list.first].timestamp);
            auto a2 = abs(largest_timestamp-motion_list.second.front().timestamp);

            // pick  up the closest to the selected timestamp motion sample
            if ( a2 >= a1 )
                break;

            sample_set[motion_list.first] = motion_list.second.front();
            motion_list.second.pop_front();
        } //end of while
    }

    return true;
}

bool rs::utils::samples_time_sync_zr300::empty_list_exists()
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

void rs::utils::samples_time_sync_zr300::pop_or_save_to_not_matched(stream_type st_type)
{
    if (m_not_matched_frames_buffer_size!=0)
            m_stream_lists_dropped_frames[st_type].push_back( m_stream_lists[st_type].front());

    m_stream_lists[st_type].pop_front();
}

bool rs::utils::samples_time_sync_zr300::insert(image_interface * new_image,
                                     rs::core::correlated_sample_set& correlated_sample)
{
    if (!new_image)
        throw std::invalid_argument("Null pointer received!");

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


bool rs::utils::samples_time_sync_zr300::insert(rs::core::motion_sample& new_motion, rs::core::correlated_sample_set& correlated_sample)
{
    if (!is_motion_registered(new_motion.type))
        throw std::invalid_argument("Stream was not registered to this sync utility instance!");

    std::lock_guard<std::mutex> lock_guard(m_image_mutex);

    m_motion_lists[new_motion.type].push_back(new_motion);

    return sync_all(correlated_sample);
}

bool rs::utils::samples_time_sync_zr300::get_not_matched_frame(rs::core::stream_type stream_type, image_interface **not_matched_frame)
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

void rs::utils::samples_time_sync_zr300::flush()
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



