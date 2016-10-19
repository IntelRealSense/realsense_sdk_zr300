// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <stddef.h>
#include <assert.h>
#include "disk_write.h"
#include "include/file.h"
#include "rs_sdk_version.h"
#include "rs/utils/log_utils.h"

using namespace rs::core;

namespace rs
{
    namespace record
    {
        static const uint32_t MAX_CACHED_SAMPLES = 5;

        disk_write::disk_write(void):
            m_is_configured(false),
            m_paused(false),
            m_stop_writing(true),
            m_min_fps(0)
        {

        }

        disk_write::~disk_write(void)
        {
            stop();
        }

        uint32_t disk_write::get_min_fps(const std::map<rs_stream, core::file_types::stream_profile>& stream_profiles)
        {
            uint32_t rv = 0xffffffff;
            for(auto it = stream_profiles.begin(); it != stream_profiles.end(); ++it)
            {
                if(static_cast<uint32_t>(it->second.info.framerate) < rv)
                    rv = it->second.info.framerate;
            }
            if(rv == 0)
                throw std::runtime_error("illegal frame rate value");
            if(rv == 0xffffffff)
                throw std::runtime_error("no streams were enabled before start streaming");
            return rv;
        }

        bool disk_write::allow_sample(std::shared_ptr<rs::core::file_types::sample> &sample)
        {
            if(sample->info.type != file_types::sample_type::st_image) return true;
            auto frame = std::dynamic_pointer_cast<file_types::frame_sample>(sample);
            if (frame)
            {
                auto max_samples = MAX_CACHED_SAMPLES * frame->finfo.framerate / m_min_fps;
                if(m_samples_count[frame->finfo.stream] > max_samples) return false;
                m_samples_count[frame->finfo.stream]++;
                return true;
            }
            return false;
        }

        void disk_write::record_sample(std::shared_ptr<file_types::sample> &sample)
        {
            LOG_FUNC_SCOPE();
            if (m_paused)
            {
                return;//device is still streaming but samples are not recorded
            }
            bool insert_samples = false;
            {
                std::lock_guard<std::mutex> guard(m_main_mutex);
                insert_samples = allow_sample(sample);
                if (insert_samples)//it is ok that sample queue size may exceed MAX_CACHED_SAMPLES by few samples
                {
                    m_samples_queue.push(sample);
                }
                else
                {
                    LOG_WARN("sample drop, sample type - " << sample->info.type << " ,capture time - " << sample->info.capture_time);
                }
            }
            if(insert_samples)
            {
                std::lock_guard<std::mutex> guard(m_notify_write_thread_mutex);
                m_notify_write_thread_cv.notify_one();
            }
        }

        bool disk_write::start()
        {
            LOG_FUNC_SCOPE();
            if(!m_is_configured) return false;
            m_stop_writing = false;//protection is not required before the thread is started
            assert(!m_thread.joinable());//we don't expect the thread to be active on start
            m_thread = std::thread(&disk_write::write_thread, this);
            return true;
        }

        void disk_write::stop()
        {
            LOG_FUNC_SCOPE();

            std::unique_lock<std::mutex> guard(m_main_mutex);
            m_stop_writing = true;
            guard.unlock();

            m_notify_write_thread_cv.notify_one();

            if (m_thread.joinable())
            {
                m_thread.join();
            }

            guard.lock();
            if(m_file)
                m_file->close();
            guard.unlock();
        }

        void disk_write::set_pause(bool pause)
        {
            std::lock_guard<std::mutex> guard(m_main_mutex);
            m_paused = pause;
        }

        status disk_write::configure(const configuration& config)
        {
            std::lock_guard<std::mutex> guard(m_main_mutex);
            if(m_is_configured) return status::status_exec_aborted;
            m_file = std::unique_ptr<rs::core::file>(new rs::core::file());
            status sts = m_file->open(config.m_file_path, (open_file_option)(open_file_option::write));

            if (sts != status::status_no_error)
                throw std::runtime_error("failed to open file for recording, file path - " + config.m_file_path);

            m_min_fps = get_min_fps(config.m_stream_profiles);
            write_header(static_cast<uint8_t>(config.m_stream_profiles.size()), config.m_coordinate_system, config.m_capture_mode);
            write_device_info(config.m_device_info);
            write_sw_info();
            write_capabilities(config.m_capabilities);
            write_motion_intrinsics(config.m_motion_intrinsics);
            write_stream_info(config.m_stream_profiles);
            write_properties(config.m_options);
            write_first_frame_offset();
            m_is_configured = true;
            return sts;
        }

        void disk_write::write_to_file(const void* data, unsigned int numberOfBytesToWrite, unsigned int& numberOfBytesWritten)
        {
            auto sts = m_file->write_bytes(data, numberOfBytesToWrite, numberOfBytesWritten);
            if(sts != status::status_no_error)
            {
                m_file->close();
                LOG_ERROR("failed writing to file");
                throw std::runtime_error("failed writing to file");
            }
        }

        void disk_write::write_thread(void)
        {
            LOG_FUNC_SCOPE();
            while (!m_stop_writing)
            {
                std::unique_lock<std::mutex> guard(m_notify_write_thread_mutex);
                m_notify_write_thread_cv.wait(guard);
                guard.unlock();

                LOG_VERBOSE("queue contains " << m_samples_queue.size() << " samples")

                std::shared_ptr<core::file_types::sample> sample = nullptr;
                while(!m_samples_queue.empty())
                {
                    {
                        std::lock_guard<std::mutex> guard(m_main_mutex);
                        if(m_samples_queue.empty()) break;
                        sample = m_samples_queue.front();
                        m_samples_queue.pop();
                        if(!sample) continue;
                    }
                    write_sample_info(sample);
                    write_sample(sample);
                }
            }
        }

        void disk_write::write_header(uint8_t stream_count, file_types::coordinate_system cs, playback::capture_mode capture_mode)
        {
            file_types::disk_format::file_header header = {};
            header.data.version = 2;
            header.data.id = UID('R', 'S', 'L', '0' + header.data.version);
            header.data.coordinate_system = cs;
            header.data.capture_mode = capture_mode;

            /* calculate the number of streams */
            header.data.nstreams = stream_count;

            uint32_t bytes_written = 0;
            m_file->set_position(0, move_method::begin);
            write_to_file(&header, sizeof(header), bytes_written);
            LOG_INFO("write header chunk, chunk size - " << sizeof(header))
        }

        void disk_write::write_device_info(file_types::device_info info)
        {
            file_types::chunk_info chunk = {};
            chunk.id = file_types::chunk_id::chunk_device_info;
            file_types::disk_format::device_info device_info;
            chunk.size = sizeof(device_info);
            device_info.data = info;

            uint32_t bytes_written = 0;
            write_to_file(&chunk, sizeof(chunk), bytes_written);
            write_to_file(&device_info, sizeof(device_info), bytes_written);
            LOG_INFO("write device info chunk, chunk size - " << chunk.size)
        }

        void disk_write::write_sw_info()
        {
            file_types::chunk_info chunk = {};
            chunk.id = file_types::chunk_id::chunk_sw_info;
            chunk.size = sizeof(file_types::disk_format::sw_info);

            file_types::disk_format::sw_info sw_info;
            memset(&sw_info, 0, sizeof(file_types::disk_format::sw_info));
            sw_info.data.sdk = {SDK_VER_MAJOR, SDK_VER_MINOR, SDK_VER_PATCH, 0};
            sw_info.data.librealsense = {RS_API_MAJOR_VERSION, RS_API_MINOR_VERSION, RS_API_PATCH_VERSION, 0};

            uint32_t bytes_written = 0;
            write_to_file(&chunk, sizeof(chunk), bytes_written);
            write_to_file(&sw_info, sizeof(sw_info), bytes_written);
            LOG_INFO("write sw info chunk, chunk size - " << chunk.size)
        }

        void disk_write::write_capabilities(std::vector<rs_capabilities> capabilities)
        {
            file_types::chunk_info chunk = {};
            chunk.id = file_types::chunk_id::chunk_capabilities;
            chunk.size = static_cast<int32_t>(capabilities.size() * sizeof(rs_capabilities));

            uint32_t bytes_written = 0;
            write_to_file(&chunk, sizeof(chunk), bytes_written);
            write_to_file(capabilities.data(), chunk.size, bytes_written);
            LOG_INFO("write capabilities chunk, chunk size - " << chunk.size)
        }

        void disk_write::write_stream_info(std::map<rs_stream, file_types::stream_profile> profiles)
        {
            file_types::chunk_info chunk = {};
            chunk.id = file_types::chunk_id::chunk_stream_info;
            chunk.size = static_cast<int32_t>(profiles.size() * sizeof(file_types::disk_format::stream_info));

            uint32_t bytes_written = 0;
            write_to_file(&chunk, sizeof(chunk), bytes_written);

            /* Write each stream info */

            for(auto iter = profiles.begin(); iter != profiles.end(); ++iter)
            {
                file_types::stream_info sinfo = {};
                auto stream = iter->first;
                sinfo.ctype = m_compression.compression_policy(stream);
                sinfo.profile = iter->second;
                /* Save the stream nframes offset for later update */
                uint64_t pos = 0;
                m_file->set_position(pos, move_method::current, &pos);
                m_offsets[stream] = pos + offsetof(file_types::stream_info, nframes);
                sinfo.stream = stream;
                file_types::disk_format::stream_info stream_info = {};
                stream_info.data = sinfo;
                write_to_file(&stream_info, sizeof(stream_info), bytes_written);
                LOG_INFO("write stream info chunk, chunk size - " << chunk.size)
            }
        }

        void disk_write::write_motion_intrinsics(const rs_motion_intrinsics &motion_intrinsics)
        {
            file_types::chunk_info chunk = {};
            chunk.id = file_types::chunk_id::chunk_motion_intrinsics;
            file_types::disk_format::motion_intrinsics mi;
            chunk.size = sizeof(mi);
            mi.data = motion_intrinsics;

            uint32_t bytes_written = 0;
            write_to_file(&chunk, sizeof(chunk), bytes_written);
            write_to_file(&mi, chunk.size, bytes_written);
            LOG_INFO("write motion intrinsics chunk, chunk size - " << chunk.size)
        }

        void disk_write::write_properties(const std::vector<file_types::device_cap>& properties)
        {
            file_types::chunk_info chunk = {};
            chunk.id = file_types::chunk_id::chunk_properties;
            chunk.size = static_cast<int32_t>(properties.size() * sizeof(file_types::device_cap));

            uint32_t bytes_written = 0;
            write_to_file(&chunk, sizeof(chunk), bytes_written);
            write_to_file(properties.data(), chunk.size, bytes_written);
            LOG_INFO("write properties chunk, chunk size - " << chunk.size)
        }

        void disk_write::write_first_frame_offset()
        {
            uint64_t pos = 0;
            m_file->set_position(pos, move_method::current, &pos);
            m_file->set_position((int64_t)offsetof(file_types::file_header, first_frame_offset), move_method::begin);

            uint32_t bytes_written = 0;
            int32_t firstFramePosition = (int32_t)pos;
            write_to_file(&firstFramePosition, sizeof(firstFramePosition), bytes_written);
            m_file->set_position(pos, move_method::begin, &pos);
            LOG_INFO("first frame offset - " << pos)

        }

        void disk_write::write_stream_num_of_frames(rs_stream stream, int32_t frame_count)
        {
            auto it = m_offsets.find(stream);
            if(it == m_offsets.end()) return;
            uint64_t pos;
            m_file->get_position(&pos);
            uint32_t bytes_written = 0;
            m_file->set_position(it->second, move_method::begin);
            write_to_file(&frame_count, sizeof(int32_t), bytes_written);
            m_file->set_position(pos, move_method::begin);
            LOG_VERBOSE("stream - " << stream << " ,number of frames - " << frame_count)
        }

        void disk_write::write_sample_info(std::shared_ptr<file_types::sample> &sample)
        {
            file_types::chunk_info chunk = {};
            chunk.id = file_types::chunk_id::chunk_sample_info;
            chunk.size = sizeof(file_types::disk_format::sample_info);
            file_types::disk_format::sample_info sample_info;

            uint64_t pos = 0;
            m_file->get_position(&pos);
            sample->info.offset = pos;

            sample_info.data = sample->info;
            uint32_t bytes_written = 0;
            write_to_file(&chunk, sizeof(chunk), bytes_written);
            write_to_file(&sample_info, chunk.size, bytes_written);
        }

        void disk_write::write_sample(std::shared_ptr<file_types::sample> &sample)
        {
            switch(sample->info.type)
            {
                case file_types::sample_type::st_image:
                {
                    file_types::chunk_info chunk = {};
                    chunk.id = file_types::chunk_id::chunk_frame_info;
                    file_types::disk_format::frame_info frame_info = {};
                    chunk.size = sizeof(frame_info);
                    auto frame = std::dynamic_pointer_cast<file_types::frame_sample>(sample);
                    if (frame)
                    {
                        frame_info.data = frame->finfo;

                        uint32_t bytes_written = 0;
                        write_to_file(&chunk, sizeof(chunk), bytes_written);
                        write_to_file(&frame_info, chunk.size, bytes_written);
                        write_frame_metadata_chunk(frame->metadata);
                        write_image_data(sample);
                        LOG_VERBOSE("write frame, stream type - " << frame->finfo.stream << " capture time - " << frame->info.capture_time);
                        LOG_VERBOSE("write frame, stream type - " << frame->finfo.stream << " system time - " << frame->finfo.system_time);
                        LOG_VERBOSE("write frame, stream type - " << frame->finfo.stream << " time stamp - " << frame->finfo.time_stamp);
                        LOG_VERBOSE("write frame, stream type - " << frame->finfo.stream << " frame number - " << frame->finfo.number);
                    }
                    break;
                }
                case file_types::sample_type::st_motion:
                {
                    file_types::chunk_info chunk = {};
                    chunk.id = file_types::chunk_id::chunk_sample_data;
                    file_types::disk_format::motion_data motion_data = {};
                    chunk.size = sizeof(motion_data);
                    auto motion = std::dynamic_pointer_cast<file_types::motion_sample>(sample);
                    if (motion)
                    {
                        motion_data.data = motion->data;
                        uint32_t bytes_written = 0;
                        write_to_file(&chunk, sizeof(chunk), bytes_written);
                        write_to_file(&motion_data, chunk.size, bytes_written);
                        LOG_VERBOSE("write motion, relative time - " << motion->info.capture_time)
                    }
                    break;
                }
                case file_types::sample_type::st_time:
                {
                    file_types::chunk_info chunk = {};
                    chunk.id = file_types::chunk_id::chunk_sample_data;
                    file_types::disk_format::time_stamp_data time_stamp_data = {};
                    chunk.size = sizeof(time_stamp_data);
                    auto time = std::dynamic_pointer_cast<file_types::time_stamp_sample>(sample);
                    if (time)
                    {
                        time_stamp_data.data = time->data;
                        uint32_t bytes_written = 0;
                        write_to_file(&chunk, sizeof(chunk), bytes_written);
                        write_to_file(&time_stamp_data, chunk.size, bytes_written);
                        LOG_VERBOSE("write time stamp, relative time - " << time->info.capture_time)
                    }
                    break;
                }
            }
        }

        void disk_write::write_frame_metadata_chunk(const std::map<rs_frame_metadata, double>& metadata)
        {
            using metadata_pair_type = typename std::remove_reference<decltype(metadata)>::type::value_type; //get the undrlying pair of the map same as in playback
            if(metadata.size() == 0)
            {
                LOG_ERROR("No metadata to write for current frame");
            }
            std::vector<metadata_pair_type> metadata_pairs;
            for(auto key_value : metadata)
            {
                metadata_pairs.emplace_back(key_value.first, key_value.second);
            }
            assert(metadata_pairs.size() <= RS_FRAME_METADATA_COUNT);
            uint32_t num_bytes_to_write = static_cast<uint32_t>(sizeof(metadata_pair_type) * metadata_pairs.size());
            uint32_t bytes_written = 0;

            file_types::chunk_info chunk = {};
            chunk.id = file_types::chunk_id::chunk_image_metadata;
            chunk.size = num_bytes_to_write;

            write_to_file(&chunk, sizeof(chunk), bytes_written);
            write_to_file(metadata_pairs.data(), num_bytes_to_write, bytes_written);
        }

        void disk_write::write_image_data(std::shared_ptr<file_types::sample> &sample)
        {
            auto frame = std::dynamic_pointer_cast<file_types::frame_sample>(sample);

            if (frame)
            {
                /* Get raw stream size */
                int32_t nbytes = (frame->finfo.stride * frame->finfo.height);

                std::vector<uint8_t> buffer;
                file_types::compression_type ctype = m_compression.compression_policy(frame->finfo.stream);
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
                chunk.size = static_cast<int32_t>(buffer.size());

                uint32_t bytes_written = 0;
                write_to_file(&chunk, sizeof(chunk), bytes_written);
                write_to_file(buffer.data(), chunk.size, bytes_written);
                m_number_of_frames[frame->finfo.stream]++;
                write_stream_num_of_frames(frame->finfo.stream, m_number_of_frames[frame->finfo.stream]);
                std::lock_guard<std::mutex> guard(m_main_mutex);
                m_samples_count[frame->finfo.stream]--;
            }
        }
    }
}
