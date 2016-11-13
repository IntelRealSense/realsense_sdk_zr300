// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "samples_time_sync_zr300.h"

using namespace std;
using namespace rs::core;
using namespace rs::utils;

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
