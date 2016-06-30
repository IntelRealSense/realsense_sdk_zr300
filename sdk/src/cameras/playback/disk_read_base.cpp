// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "disk_read_base.h"
#include <limits>
#include <vector>
#include <algorithm>
#include "rs/core/metadata_interface.h"
#include "file/file.h"
#include "rs/utils/log_utils.h"

using namespace rs::core;
using namespace rs::playback;

namespace
{
    uint64_t get_sample_time_stamp(std::shared_ptr<file_types::sample> &sample)
    {
        switch(sample->type)
        {
            case file_types::sample_type::st_image:
            {
                file_types::frame *frame = static_cast<file_types::frame*>(sample.get());
                return frame->info.sync;
            }
            case file_types::sample_type::st_motion:
            {
                file_types::motion *motion = static_cast<file_types::motion*>(sample.get());
                return motion->info.sync;
            }
            case file_types::sample_type::st_time:
            {
                file_types::time_stamp *time = static_cast<file_types::time_stamp*>(sample.get());
                return time->info.sync;
            }
        }
        return 0;
    }
}

disk_read_base::disk_read_base(const char *filename) : m_file_name(filename), m_file_header(), m_pause(false),
    m_realtime(true), m_streams_infos(), m_projection_changed(false), m_base_ts(0), m_curr_ts(0), m_is_indexed(false),
    m_coordinate_system(file_types::coordinate_system::rear_default), m_main_index(0)
{

}

status disk_read_base::init()
{
    if (m_file_name.empty()) return status_file_open_failed;
    m_file_info = std::unique_ptr<file>(new file());

    auto status = m_file_info->open(m_file_name.c_str(), (open_file_option)(open_file_option::read));
    if (status != status_no_error) return status;

    m_file_index = std::unique_ptr<file>(new file());
    status = m_file_index->open(m_file_name.c_str(), (open_file_option)(open_file_option::read));
    if (status != status_no_error) return status;

    m_file_data = std::unique_ptr<file>(new file());
    status = m_file_data->open(m_file_name.c_str(), (open_file_option)(open_file_option::read));
    if (status != status_no_error) return status;

    status = read_headers();

    if (status < status_no_error) return status;

    /* Be prepared to index the frames */
    m_file_index->set_position(m_file_header.first_frame_offset, move_method::begin);
    LOG_INFO("init " << (status == status_no_error ? "succeeded" : "failed") << "(status - " << status << ")")

    return status;
}

void disk_read_base::start()
{
    LOG_INFO("start")
    if (m_thread.joinable())
        m_thread.join();
    set_pause(false);
    m_thread = std::thread(&disk_read_base::read_thread, this);
}

void disk_read_base::enable_stream(rs_stream stream, bool state)
{
    if(state)
    {
        if(std::find(m_active_streams.begin(), m_active_streams.end(), stream) == m_active_streams.end())
            m_active_streams.push_back(stream);
    }
    else
    {
        m_active_streams.erase(std::remove(m_active_streams.begin(), m_active_streams.end(), stream), m_active_streams.end());
    }
}

disk_read_base::~disk_read_base(void)
{
    LOG_FUNC_SCOPE();
    set_pause(true);
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

void disk_read_base::read_thread()
{
    m_clock_sys = std::chrono::high_resolution_clock::now();
    while (!m_pause)
    {
        read_next_sample();
    }
}

std::shared_ptr<file_types::frame> disk_read_base::get_image_sample(rs_stream stream)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    std::shared_ptr<file_types::frame> rv = m_current_image_sample[stream];
    m_current_image_sample[stream].reset();
    LOG_VERBOSE("get image sample, stream - " << stream << " sample time - " << rv->info.sync)
    return rv;
}

std::map<rs_stream, std::shared_ptr<file_types::frame>> disk_read_base::get_image_samples()
{
    std::lock_guard<std::mutex> guard(m_mutex);

    std::map<rs_stream, std::shared_ptr<file_types::frame>> rv;
    for(auto it = m_current_image_sample.begin(); it != m_current_image_sample.end(); ++it)
    {
        auto stream = it->first;
        rv[stream] = it->second;
        it->second.reset();
    }
    LOG_VERBOSE("get " << rv.size() << " image samples")
    return rv;
}

void disk_read_base::read_next_sample()
{
    while(m_main_index >= m_samples.size() && !m_is_indexed)index_next_samples(NUMBER_OF_SAMPLES_TO_INDEX);
    if(m_main_index >= m_samples.size())
    {
        set_pause(true);
        return;
    }
    auto sample = m_samples[m_main_index];
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_main_index++;
    }
    switch(sample->type)
    {
        case file_types::sample_type::st_image:
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            auto frame = std::dynamic_pointer_cast<file_types::frame>(sample);
            //don't prefatch frame if stream is disabled.
            if(std::find(m_active_streams.begin(), m_active_streams.end(), frame->info.stream) == m_active_streams.end())
                return;
            auto curr = std::shared_ptr<file_types::frame>(
            new file_types::frame(frame->info), [](file_types::frame* f) { delete[] f->data; });
            read_image_buffer(curr);
            m_buffered_samples[frame->info.stream]++;
            m_prefetched_samples.push(curr);
            m_curr_ts = curr->info.sync;
            LOG_VERBOSE("frame sample prefetched, frame time - " << curr->info.sync)
            break;
        }
        case file_types::sample_type::st_motion:
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            auto motion = std::dynamic_pointer_cast<file_types::motion>(sample);
            m_prefetched_samples.push(sample);
            m_curr_ts = motion->info.sync;
            LOG_VERBOSE("motion sample prefetched, motion time - " << motion->info.sync)
            break;
        }
        case file_types::sample_type::st_time:
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            auto time = std::dynamic_pointer_cast<file_types::time_stamp>(sample);
            m_prefetched_samples.push(sample);
            m_curr_ts = time->info.sync;
            LOG_VERBOSE("time sample prefetched, time - " << time->info.sync)
            break;
        }
    }
    if(m_realtime)
    {
        auto wait_for = calc_sleep_time(m_prefetched_samples.front());
        //goto sleep in case we have at least on frame ready for each stream
        if(all_samples_bufferd())
            std::this_thread::sleep_for (std::chrono::milliseconds(wait_for));
        //handle next sample if it's time as come
        while(wait_for <= 0)
        {
            if(m_prefetched_samples.front()->type == file_types::sample_type::st_image)
            {
                auto frame = std::dynamic_pointer_cast<file_types::frame>(m_prefetched_samples.front());
                m_buffered_samples[frame->info.stream]--;
                LOG_VERBOSE("frame callback, frame time - " << frame->info.sync)
            }
            m_sample_callback(m_prefetched_samples.front());
            m_prefetched_samples.pop();
            wait_for = m_prefetched_samples.size() > 0 ? calc_sleep_time(m_prefetched_samples.front()) : 1;
        }
    }
    else
    {
        m_sample_callback(m_prefetched_samples.front());
        m_prefetched_samples.pop();
    }
}

bool disk_read_base::is_stream_available(rs_stream stream, int width, int height, rs_format format, int framerate)
{
    for(auto it = m_streams_infos.begin(); it != m_streams_infos.end(); ++it)
    {
        file_types::stream_info si = it->second;
        if(si.stream != stream) continue;
        if(si.profile.info.width != width) continue;
        if(si.profile.info.height != height) continue;
        if(si.profile.info.format != format) continue;
        if(si.profile.frame_rate != framerate) continue;
        return true;
    }
    return false;
}

bool disk_read_base::set_frame_by_index(uint32_t index, rs_stream stream_type)
{
    set_pause(true);

    if (index < 0) return false;

    uint32_t sample_index = 0;

    do
    {
        if(sample_index >= m_samples.size())
        {
            if(m_is_indexed)return false;
            index_next_samples(NUMBER_OF_SAMPLES_TO_INDEX);
        }
        if(sample_index >= m_samples.size())continue;
        std::lock_guard<std::mutex> guard(m_mutex);
        if(m_samples[sample_index]->type != file_types::sample_type::st_image)continue;
        auto frame = std::dynamic_pointer_cast<file_types::frame>(m_samples[sample_index]);
        if(frame->info.stream != stream_type) continue;
        if(frame->info.index_in_stream >= index)
        {
            break;
        }
    }
    while(++sample_index);

    //update current frames for all streams.
    find_nearest_frames(sample_index, stream_type);
    LOG_VERBOSE("set index to - " << index << " ,stream - " << stream_type);
    return true;
}

bool disk_read_base::set_frame_by_time_stamp(uint64_t ts)
{
    set_pause(true);
    rs_stream stream = (rs_stream)-1;
    uint32_t index = 0;
    // Index the streams until we have at least a stream whose time stamp is bigger than ts.
    do
    {
        if(index >= m_samples.size())
        {
            if(m_is_indexed)return false;
            index_next_samples(NUMBER_OF_SAMPLES_TO_INDEX);
        }
        {
            if(index >= m_samples.size())continue;
            std::lock_guard<std::mutex> guard(m_mutex);
            if(m_samples[index]->type != file_types::sample_type::st_image)continue;
            auto frame = std::dynamic_pointer_cast<file_types::frame>(m_samples[index]);
            if(frame->info.time_stamp >= ts)
            {
                stream = frame->info.stream;
                break;
            }
        }
    }
    while(++index);

    if(stream == (rs_stream)-1) return false;

    find_nearest_frames(index, stream);
    LOG_VERBOSE("requested time stamp - " << ts << " ,set index to - " << index);
    return true;
}

void disk_read_base::find_nearest_frames(uint32_t sample_index, rs_stream stream)
{
    std::map<rs_stream, uint64_t> prev_sync;
    std::map<rs_stream, uint64_t> prev_index;
    std::map<rs_stream, uint64_t> next_sync;
    std::map<rs_stream, uint64_t> next_index;
    auto index = sample_index;
    while(index > 0 && prev_sync.size() < m_active_streams.size())
    {
        index--;
        if(m_samples[index]->type != file_types::sample_type::st_image)continue;
        auto frame = std::dynamic_pointer_cast<file_types::frame>(m_samples[index]);
        if(next_sync.find(frame->info.stream) != next_sync.end())continue;
        prev_sync[frame->info.stream] = frame->info.sync;
        prev_index[frame->info.stream] = index;
    }
    index = sample_index;
    while(next_sync.size() < m_active_streams.size())
    {
        if(index + 1 >= m_samples.size())
        {
            if(m_is_indexed)break;
            index_next_samples(NUMBER_OF_SAMPLES_TO_INDEX);
        }
        if(index + 1 >= m_samples.size())continue;
        index++;
        if(m_samples[index]->type != file_types::sample_type::st_image)continue;
        auto frame = std::dynamic_pointer_cast<file_types::frame>(m_samples[index]);
        if(next_sync.find(frame->info.stream) != next_sync.end())continue;
        next_sync[frame->info.stream] = frame->info.sync;
        next_index[frame->info.stream] = index;
    }
    auto f = std::dynamic_pointer_cast<file_types::frame>(m_samples[sample_index]);
    auto ref_sync = f->info.sync;
    m_curr_ts = ref_sync;
    for(auto it = m_active_streams.begin(); it != m_active_streams.end(); ++it)
    {
        std::lock_guard<std::mutex> guard(m_mutex);

        std::shared_ptr<file_types::sample> sample;
        if(*it == stream)
            sample = m_samples[sample_index];
        else
            sample = abs(ref_sync - prev_sync[*it]) > abs(ref_sync - next_sync[*it]) ?
                     m_samples[next_index[*it]] : m_samples[prev_index[*it]];
        auto frame = std::dynamic_pointer_cast<file_types::frame>(sample);
        auto curr = std::shared_ptr<file_types::frame>(
        new file_types::frame(frame->info), [](file_types::frame* f) { delete[] f->data; });
        read_image_buffer(curr);
        m_current_image_sample[frame->info.stream] = curr;
        m_curr_ts = curr->info.sync > m_curr_ts ? curr->info.sync : m_curr_ts;
    }
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_main_index = sample_index;
    }
    LOG_VERBOSE("update " << m_current_image_sample.size() << " frames")
}

void disk_read_base::set_realtime(bool realtime)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    this->m_realtime = realtime;

    //set time base to currnt sample time
    update_time_base(m_curr_ts);
    LOG_INFO((realtime ? "enable" : "disable") << " realtime")
}

void disk_read_base::set_pause(bool pause)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    this->m_pause = pause;
    if(!pause)
    {
        //reset time base on resume
        update_time_base(m_curr_ts);
    }
    if(m_stop_callback)
        m_stop_callback(m_pause);
    LOG_VERBOSE((pause ? "pause" : "resume") << " streaming")
}

uint32_t disk_read_base::query_number_of_frames(rs_stream stream_type)
{
    uint32_t nframes = m_streams_infos[stream_type].nframes;

    if (nframes > 0) return nframes;

    /* If not able to get from the header, let's count */
    while (!m_is_indexed) index_next_samples(std::numeric_limits<uint32_t>::max());

    return (int32_t)m_image_indexes[stream_type].size();
}

bool disk_read_base::all_samples_bufferd()
{
    for(auto it = m_buffered_samples.begin(); it != m_buffered_samples.end(); ++it)
    {
        if(!m_active_streams[it->first]) continue;//don't check if stream is disabled
        if(it->second > 0) continue;//continue if at least one frame is ready
        return false;
    }
    return true;
}

int64_t disk_read_base::calc_sleep_time(std::shared_ptr<file_types::sample> sample)
{
    auto now = std::chrono::high_resolution_clock::now();
    auto time_span = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_clock_sys).count();
    auto time_stamp = get_sample_time_stamp(sample);
    //number of miliseconds to wait - the diff in milisecond between the last call for streaming resume
    //and the recorded time.
    int wait_for = time_stamp - m_base_ts - time_span;
    LOG_VERBOSE("sleep length " << wait_for << " miliseconds")
    return wait_for;
}

void disk_read_base::update_time_base(uint64_t time)
{
    m_clock_sys = std::chrono::high_resolution_clock::now();
    m_base_ts = time;
    LOG_VERBOSE("new time base - " << time)
}

file_types::version disk_read_base::query_sdk_version()
{
    return m_sw_info.sdk;
}

file_types::version disk_read_base::query_librealsense_version()
{
    return m_sw_info.librealsense;
}

status disk_read_base::read_image_buffer(std::shared_ptr<file_types::frame> &frame)
{
    status sts = status_item_unavailable;

    m_file_data->set_position(frame->info.offset, move_method::begin);

    uint32_t nbytesRead;
    unsigned long nbytesToRead = 0;
    file_types::chunk_info chunk = {};
    for (;;)
    {
        m_file_data->read_bytes(&chunk, sizeof(chunk), nbytesRead);
        nbytesToRead = chunk.size;
        switch (chunk.id)
        {
            case file_types::chunk_id::chunk_sample_data:
            {
                m_file_data->set_position(size_of_pitches(),move_method::current);
                nbytesToRead -= size_of_pitches();
                auto ctype = m_streams_infos[frame->info.stream].ctype;
                switch (ctype)
                {
                    case file_types::compression_type::none:
                    {
                        auto data = new uint8_t[nbytesToRead];
                        m_file_data->read_bytes(data, nbytesToRead, nbytesRead);
                        frame->data = data;
                        nbytesToRead -= nbytesRead;
                        if(nbytesToRead == 0 && frame.get() != nullptr) sts = status_no_error;
                        break;
                    }
                    case file_types::compression_type::lzo:
                    case file_types::compression_type::h264:
                    {
                        std::vector<uint8_t> buffer(nbytesToRead);
                        m_file_data->read_bytes(buffer.data(), nbytesToRead, nbytesRead);
                        nbytesToRead -= nbytesRead;
                        sts = m_compression.decode_image(ctype, frame, buffer);
                    }
                    break;
                }
                if (nbytesToRead > 0)
                {
                    LOG_ERROR("image size failed to match the data size")
                    return status_item_unavailable;
                }
                return sts;
            }
            default:
            {
                m_file_data->set_position(nbytesToRead, move_method::current);
            }
            nbytesToRead = 0;
        }
    }
}
