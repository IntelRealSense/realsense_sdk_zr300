#include <rs_sdk.h>
#include "rs/utils/log_utils.h"


rs::utils::sync_utility::sync_utility(vector<pair<stream_type, unsigned int>>& streams, vector<pair<motion_type, unsigned int>>& motions, unsigned int max_input_latency) :
    m_max_input_latency(max_input_latency)
{

    LOG_FUNC_SCOPE();

    if (max_input_latency == 0)
    {
        LOG_ERROR("Zero latency is not acceptable");
        throw "Zero latency is not acceptable";
    }

    if (streams.size() + motions.size() < 2 )
    {
        LOG_ERROR("Less than two streams were registered to sync utility instance!");
        throw "Less than two streams were registered to sync utility instance!";
    }

    // Save fps's for each stream and motion requested
    for (auto item : streams)
    {
        if (item.first != stream_type::color && item.first != stream_type::depth)
            throw "Not implemented.";

        int buffer_length = item.second * max_input_latency / 1000;

        auto ret = m_stream_lists.insert(std::make_pair(item.first, cyclic_array<smart_ptr<image_interface>>(buffer_length)));

        LOG_DEBUG("For stream " << static_cast<uint8_t>(item.first) << " with fps " << item.second << " using buffer length " << buffer_length);

        if (ret.second == false)
        {
            LOG_ERROR("Same stream type specified twice !");
            throw "Same stream type specified twice !";
        }

        m_stream_fps[item.first] = item.second;
    }

    for (auto item : motions)
    {
        // We do not support motions meanwhile
        throw "Not implemented.";

        m_motion_fps[item.first] = item.second;

        int buffer_length = item.second * max_input_latency / 1000;

        auto ret = m_motion_lists.insert(std::make_pair(item.first, cyclic_array<motion_sample>(buffer_length)));

        LOG_DEBUG("For stream " << static_cast<uint8_t>(item.first) << " with fps " << item.second << " using buffer length " << buffer_length);

        if (ret.second == false)
        {
            LOG_ERROR("Same stream type specified twice !");
            throw "Same motion type specified twice !";
        }

    }


}

bool rs::utils::sync_utility::sync_color_and_depth( rs::core::correlated_sample_set& sample_set )
{
    while (true)
    {
        // if one of the lists is empty, then match can not be found
        if (m_stream_lists[stream_type::color].size() == 0 ||
                m_stream_lists[stream_type::depth].size() == 0 )
        {
            LOG_TRACE(" Depth and/or color stream list(s) is (are) empty.")
            return false;
        }

        if ( m_stream_lists[stream_type::color].front()->query_time_stamp() > m_stream_lists[stream_type::depth].front()->query_time_stamp() )
        {
            // timestamp of color at position 0 is larger than timestamp of depth at position 0  ->
            // this depth frame will never be matched, we can  remove it from the list
            m_stream_lists[stream_type::depth].pop_front();

            LOG_TRACE(" Removing not matching frame from depth list.")
            continue;
        }
        else if ( m_stream_lists[stream_type::color].front()->query_time_stamp() < m_stream_lists[stream_type::depth].front()->query_time_stamp() )
        {
            // timestamp of color at position 0 is smaller than timestamp of depth at position 0  ->
            // this color frame will never be matched, we can  remove it from the list
            m_stream_lists[stream_type::color].pop_front();

            LOG_TRACE(" Removing not matching frame from color list.")
            continue;
        }
        else
        {
            // timestamps are equal -> match found, remove it from lists and put to correlated set
            sample_set.images[static_cast<uint8_t>(stream_type::color)] = m_stream_lists[stream_type::color].front();
            sample_set.images[static_cast<uint8_t>(stream_type::depth)] = m_stream_lists[stream_type::depth].front();

            m_stream_lists[stream_type::color].pop_front();
            m_stream_lists[stream_type::depth].pop_front();

            LOG_TRACE(" Match found.")

            return true;
        }
    } // end of while(true)
}

bool rs::utils::sync_utility::is_stream_registered(stream_type stream)
{
    auto it = m_stream_fps.find(stream);

    if (it == m_stream_fps.end())
        return false;

    return true;
}

bool rs::utils::sync_utility::is_motion_registered(motion_type motion)
{
    auto it = m_motion_fps.find(motion);

    if (it == m_motion_fps.end())
        return false;

    return true;

}


bool rs::utils::sync_utility::insert(rs::utils::smart_ptr<image_interface> new_image,
                                     rs::core::correlated_sample_set& correlated_sample)
{
    if (!new_image)
        throw "Null pointer received!";

    // this stream type was not registered with this instance of the sync_utility
    if (!is_stream_registered( new_image->query_stream_type() ))
        throw "Stream was not registered to this sync utility instance!";

    std::lock_guard<std::mutex> lock_guard(m_image_mutex);

    m_stream_lists[new_image->query_stream_type()].push_back( new_image );

    // return synced color and depth
    return sync_color_and_depth(correlated_sample);
}


bool rs::utils::sync_utility::insert(rs::core::motion_sample new_motion, rs::core::correlated_sample_set& correlated_sample)
{
    throw "Not implemented.";

    return false;
}

