// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <stddef.h>
#include "disk_write.h"
#include "file/file.h"
#include "sdk/sdk_utilities.h"
#include "rs/utils/log_utils.h"
#include <stddef.h>

#define FILE_VERSION 1

using namespace rs::core;

namespace rs
{
    namespace record
    {
        static const int32_t MAX_CACHED_SAMPLES = 120;

        disk_write::disk_write(void):
            m_paused(false), m_stop_writing(true), m_device_info(), m_stream_profiles(), m_coordinate_system(file_types::coordinate_system::rear_default)
        {

        }

        disk_write::~disk_write(void)
        {
            stop();
        }

        void disk_write::record_sample(std::vector<std::shared_ptr<file_types::sample> > &samples)
        {
            LOG_FUNC_SCOPE();
            if (m_paused)
            {
                return;//device is still streaming but samples are not recorded
            }
            bool insert_samples = false;
            {
                std::lock_guard<std::mutex> guard(m_main_mutex);
                LOG_VERBOSE("add " << samples.size() << " samples to the write queue");
                insert_samples = m_samples_queue.size() < MAX_CACHED_SAMPLES;
                if (insert_samples)//it is ok that sample queue size may exceed MAX_CACHED_SAMPLES by few samples
                {
                    m_samples_queue.insert(m_samples_queue.end(), std::make_move_iterator(samples.begin()), std::make_move_iterator(samples.end()));
                }
            }
            if(insert_samples)
            {
                std::lock_guard<std::mutex> guard(m_notify_write_thread_mutex);
                m_notify_write_thread_cv.notify_one();
            }
        }

        status disk_write::start()
        {
            LOG_FUNC_SCOPE();
            m_file = std::unique_ptr<rs::core::file>(new rs::core::file());
            status sts = m_file->open(m_file_path, (open_file_option)(open_file_option::write));

            if (sts != status::status_no_error)
                return sts;

            write_header();
            write_device_info();
            write_sw_info();
            write_capabilities();
            write_stream_info();
            write_properties();
            write_first_frame_offset();
            m_stop_writing = false;//protection is not required before the thread is started
            assert(!m_thread.joinable());//we don't expect the thread to be active on start
            m_thread = std::thread(&disk_write::write_thread, this);
            return sts;
        }

        void disk_write::stop()
        {
            LOG_FUNC_SCOPE();

            {
                std::lock_guard<std::mutex> guard(m_main_mutex);
                m_stop_writing = true;
                m_samples_queue.clear();

                if(m_file)
                    m_file->close();
            }

            {
                std::lock_guard<std::mutex> guard(m_notify_write_thread_mutex);
                m_notify_write_thread_cv.notify_one();
            }

            if (m_thread.joinable())
            {
                m_thread.join();
            }
        }

        void disk_write::set_pause(bool pause)
        {
            std::lock_guard<std::mutex> guard(m_main_mutex);
            m_samples_queue.clear();
            m_paused = pause;
        }

        void disk_write::write_thread(void)
        {
            LOG_FUNC_SCOPE();
            while (!m_stop_writing)
            {
                std::unique_lock<std::mutex> guard(m_notify_write_thread_mutex);
                m_notify_write_thread_cv.wait(guard);
                guard.unlock();

                std::vector<std::shared_ptr<file_types::sample>> queue;
                {
                    std::lock_guard<std::mutex> guard(m_main_mutex);
                    if (m_stop_writing)
                        break;
                    queue = m_samples_queue;
                    m_samples_queue.clear();
                }
                LOG_VERBOSE("queue contains " << queue.size() << " samples")

                for (auto iter = queue.begin(); iter != queue.end(); ++iter)
                {
                    auto& sample = (*iter);
                    assert(sample);
                    write_sample_type(sample);
                    write_sample(sample);
                }
            }
            /* Write the stream numbers */
            write_stream_num_of_frames();
        }

        void disk_write::write_header()
        {
            file_types::disk_format::file_header header = {};
            header.data.id = UID('R', 'S', 'L', 'X');
            header.data.version = FILE_VERSION;
            header.data.coordinate_system = m_coordinate_system;

            /* calculate the number of streams */
            header.data.nstreams = m_stream_profiles.size();

            uint32_t bytes_written;
            m_file->set_position(0, move_method::begin);
            m_file->write_bytes(&header, sizeof(header), bytes_written);
            LOG_INFO("write header chunk, chunk size - " << sizeof(header))
        }

        void disk_write::write_device_info()
        {
            file_types::chunk_info chunk = {};
            chunk.id = file_types::chunk_id::chunk_device_info;
            file_types::disk_format::device_info device_info;
            chunk.size = sizeof(device_info);
            device_info.data = m_device_info;

            uint32_t bytes_written;
            m_file->write_bytes(&chunk, sizeof(chunk), bytes_written);
            m_file->write_bytes(&device_info, sizeof(device_info), bytes_written);
            LOG_INFO("write device info chunk, chunk size - " << chunk.size)
        }

        void disk_write::write_sw_info()
        {
            file_types::chunk_info chunk = {};
            chunk.id = file_types::chunk_id::chunk_sw_info;
            chunk.size = sizeof(file_types::disk_format::sw_info);

            file_types::disk_format::sw_info sw_info;
            memset(&sw_info, 0, sizeof(file_types::disk_format::sw_info));
            sw_info.data.sdk = {SDK_VER_MAJOR, SDK_VER_MINOR, SDK_VER_COMMIT_NUMBER, SDK_VER_COMMIT_ID};
            sw_info.data.librealsense = {RS_API_VERSION};

            uint32_t bytes_written;
            m_file->write_bytes(&chunk, sizeof(chunk), bytes_written);
            m_file->write_bytes(&sw_info, sizeof(sw_info), bytes_written);
            LOG_INFO("write sw info chunk, chunk size - " << chunk.size)
            LOG_INFO("sdk version - " << sdk_utils::get_sdk_version().c_str())
        }

        void disk_write::write_capabilities()
        {
            file_types::chunk_info chunk = {};
            chunk.id = file_types::chunk_id::chunk_capabilities;
            chunk.size = m_capabilities.size() * sizeof(rs_capabilities);

            uint32_t bytes_written;
            m_file->write_bytes(&chunk, sizeof(chunk), bytes_written);
            m_file->write_bytes(m_capabilities.data(), chunk.size, bytes_written);
            LOG_INFO("write capabilities chunk, chunk size - " << chunk.size)
        }

        void disk_write::write_stream_info()
        {
            file_types::chunk_info chunk = {};
            chunk.id = file_types::chunk_id::chunk_stream_info;
            chunk.size = m_stream_profiles.size() * sizeof(file_types::disk_format::stream_info);

            uint32_t bytes_written;
            m_file->write_bytes(&chunk, sizeof(chunk), bytes_written);

            /* Write each stream info */

            for(auto iter = m_stream_profiles.begin(); iter != m_stream_profiles.end(); ++iter)
            {
                file_types::stream_info sinfo = {};
                auto stream = iter->first;
                sinfo.ctype = m_compression.compression_policy(stream);
                sinfo.profile = iter->second;
                /* Save the stream nframes offset for later update */
                int64_t pos = 0;
                m_file->set_position(pos, move_method::current, &pos);
                m_offsets[stream] = pos + offsetof(file_types::stream_info, nframes);
                sinfo.stream = stream;
                file_types::disk_format::stream_info stream_info = {};
                stream_info.data = sinfo;
                m_file->write_bytes(&stream_info, sizeof(stream_info), bytes_written);
                LOG_INFO("write stream info chunk, chunk size - " << chunk.size)
            }
        }

        void disk_write::write_properties()
        {
            file_types::chunk_info chunk = {};
            chunk.id = file_types::chunk_id::chunk_properties;
            chunk.size = m_options.size()*sizeof(file_types::device_cap);

            uint32_t bytes_written;
            m_file->write_bytes(&chunk, sizeof(chunk), bytes_written);
            m_file->write_bytes(m_options.data(), chunk.size, bytes_written);
            LOG_INFO("write properties chunk, chunk size - " << chunk.size)
        }

        void disk_write::write_first_frame_offset()
        {
            int64_t pos = 0;
            m_file->set_position(pos, move_method::current, &pos);
            m_file->set_position((int64_t)offsetof(file_types::file_header, first_frame_offset), move_method::begin);

            uint32_t bytes_written;
            int32_t firstFramePosition = (int32_t)pos;
            m_file->write_bytes(&firstFramePosition, sizeof(firstFramePosition), bytes_written);
            m_file->set_position(pos, move_method::begin, &pos);
            LOG_INFO("first frame offset - " << pos)

        }

        void disk_write::write_stream_num_of_frames()
        {
            for (auto s = m_offsets.begin(); s != m_offsets.end(); s++)
            {
                auto itr = m_number_of_frames.find(s->first);
                if (itr == m_number_of_frames.end()) continue;

                uint32_t bytes_written;
                m_file->set_position(s->second, move_method::begin);
                m_file->write_bytes(&itr->second, sizeof(int32_t), bytes_written);
                LOG_INFO("stream - " << s->first << " ,number of frames - " << itr->second)
            }
        }

        void disk_write::write_sample_type(std::shared_ptr<file_types::sample> &sample)
        {
            file_types::chunk_info chunk = {};
            chunk.id = file_types::chunk_id::chunk_sample_type;
            chunk.size = sizeof(file_types::sample_type);

            uint32_t bytes_written;
            m_file->write_bytes(&chunk, sizeof(chunk), bytes_written);
            m_file->write_bytes(&sample->type, chunk.size, bytes_written);
        }

        void disk_write::write_sample(std::shared_ptr<file_types::sample> &sample)
        {
            file_types::chunk_info chunk = {};
            chunk.id = file_types::chunk_id::chunk_sample_info;
            uint64_t pos = 0;
            m_file->get_position(&pos);
            switch(sample->type)
            {
                case file_types::sample_type::st_image:
                {
                    file_types::disk_format::frame_info frame_info = {};
                    chunk.size = sizeof(frame_info);
                    auto frame = std::dynamic_pointer_cast<file_types::frame>(sample);
                    m_number_of_frames[frame->info.stream]++;
                    frame->info.offset = pos;
                    frame_info.data = frame->info;
                    uint32_t bytes_written;
                    m_file->write_bytes(&chunk, sizeof(chunk), bytes_written);
                    m_file->write_bytes(&frame_info, chunk.size, bytes_written);
                    write_image_data(sample);
                    LOG_VERBOSE("write frame, relative time - " << frame->info.sync)
                    break;
                }
                case file_types::sample_type::st_motion:
                {
                    file_types::disk_format::motion_info motion_info = {};
                    chunk.size = sizeof(motion_info);
                    motion_info.data.offset = pos;
                    auto motion = static_cast<file_types::motion*>(sample.get());
                    motion_info.data = motion->info;
                    uint32_t bytes_written;
                    m_file->write_bytes(&chunk, sizeof(chunk), bytes_written);
                    m_file->write_bytes(&motion_info, chunk.size, bytes_written);
                    LOG_VERBOSE("write motion, relative time - " << motion->info.sync)
                    break;
                }
                case file_types::sample_type::st_time:
                {
                    file_types::disk_format::time_stamp_info time_stamp_info = {};
                    chunk.size = sizeof(time_stamp_info);
                    time_stamp_info.data.offset = pos;
                    auto time = static_cast<file_types::time_stamp*>(sample.get());
                    time_stamp_info.data = time->info;
                    uint32_t bytes_written;
                    m_file->write_bytes(&chunk, sizeof(chunk), bytes_written);
                    m_file->write_bytes(&time_stamp_info, chunk.size, bytes_written);
                    LOG_VERBOSE("write time stamp, relative time - " << time->info.sync)
                    break;
                }
            }
        }

        void disk_write::write_image_data(std::shared_ptr<file_types::sample> &sample)
        {
            auto frame = std::dynamic_pointer_cast<file_types::frame>(sample);

            /* Get raw stream size */
            int32_t nbytes = (frame->info.stride * frame->info.bpp * frame->info.height);

            std::vector<uint8_t> buffer;
            file_types::compression_type ctype = m_compression.compression_policy(frame->info.stream);
            if (ctype == file_types::compression_type::none)
            {
                auto data = frame->data;
                buffer = std::vector<uint8_t>(data, data + nbytes);
            }
            else
            {
                m_compression.encode_image(ctype, frame, buffer);
            }

            file_types::chunk_info chunk = {};
            chunk.id = file_types::chunk_id::chunk_sample_data;
            chunk.size = buffer.size();

            u_int32_t bytes_written;
            m_file->write_bytes(&chunk, sizeof(chunk), bytes_written);
            m_file->write_bytes(buffer.data(), chunk.size, bytes_written);
        }
    }
}
