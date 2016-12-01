// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "disk_read.h"
#include "include/file.h"
#include "rs/utils/log_utils.h"

namespace rs
{
    namespace playback
    {
        disk_read::~disk_read(void)
        {
            LOG_FUNC_SCOPE();
            pause();
        }

       core::status disk_read::read_headers()
        {
            /* Get the file header */
            m_file_data_read->set_position(0, core::move_method::begin);
            uint32_t num_bytes_read = 0;
            unsigned long num_bytes_to_read = 0;
            core::file_types::disk_format::file_header fh;
            m_file_data_read->read_bytes(&fh, sizeof(fh), num_bytes_read);
            m_file_header = fh.data;
            if (num_bytes_read < sizeof(m_file_header)) return core::status_item_unavailable;
            if (m_file_header.id != UID('R', 'S', 'L', '2')) return core::status_param_unsupported;

            /* Get all chunks */
            for (;;)
            {
                core::file_types::chunk_info chunk = {};
                m_file_data_read->read_bytes(&chunk, sizeof(chunk), num_bytes_read);
                if (num_bytes_read < sizeof(chunk)) break;
                if (chunk.id == core::file_types::chunk_id::chunk_sample_info) break;
                num_bytes_to_read = chunk.size;

                switch (chunk.id)
                {
                    case core::file_types::chunk_id::chunk_properties:
                        do
                        {
                            core::file_types::device_cap devcap = {};
                            m_file_data_read->read_bytes(&devcap, static_cast<uint32_t>(std::min(num_bytes_to_read, (unsigned long)sizeof(devcap))), num_bytes_read);
                            m_properties[devcap.label] = devcap.value;
                            num_bytes_to_read -= num_bytes_read;
                        }
                        while (num_bytes_to_read > 0 && num_bytes_read > 0);
                        LOG_INFO("read properties chunk " << (num_bytes_to_read == 0 ? "succeeded" : "failed"))
                        break;
                    case core::file_types::chunk_id::chunk_serializeable:
                    {
                        rs_option label = (rs_option)0;
                        m_file_data_read->read_bytes(&label, static_cast<uint32_t>(std::min(num_bytes_to_read, (unsigned long)sizeof(label))), num_bytes_read);
                        num_bytes_to_read -= num_bytes_read;
                        std::vector<uint8_t> data(num_bytes_to_read);
                        m_file_data_read->read_bytes(data.data(), static_cast<uint32_t>(num_bytes_to_read), num_bytes_read);
                        num_bytes_to_read -= num_bytes_read;
                        LOG_INFO("read serializeable chunk " << (num_bytes_to_read == 0 ? "succeeded" : "failed"))
                    }
                    break;
                    case core::file_types::chunk_id::chunk_stream_info:
                        for (int i = 0; i < m_file_header.nstreams; i++)
                        {
                            core::file_types::disk_format::stream_info stream_info1 = {};
                            m_file_data_read->read_bytes(&stream_info1, static_cast<uint32_t>(std::min(num_bytes_to_read, (unsigned long)sizeof(stream_info1))), num_bytes_read);
                            m_streams_infos[stream_info1.data.stream] = stream_info1.data;
                            num_bytes_to_read -= num_bytes_read;
                        }
                        LOG_INFO("read stream info chunk " << (num_bytes_to_read == 0 ? "succeeded" : "failed"))
                        break;
                    case core::file_types::chunk_id::chunk_motion_intrinsics:
                    {
                        core::file_types::disk_format::motion_intrinsics mi;
                        m_file_data_read->read_bytes(&mi, static_cast<uint32_t>(std::min(num_bytes_to_read, (unsigned long)sizeof(mi))), num_bytes_read);
                        m_motion_intrinsics = mi.data;
                        num_bytes_to_read -= num_bytes_read;
                        LOG_INFO("read motion intrinsics chunk " << (num_bytes_to_read == 0 ? "succeeded" : "failed"))
                    }
                    break;
                    case core::file_types::chunk_id::chunk_sw_info:
                    {
                        core::file_types::disk_format::sw_info swinfo;
                        m_file_data_read->read_bytes(&swinfo, static_cast<uint32_t>(std::min(num_bytes_to_read, (unsigned long)sizeof(swinfo))), num_bytes_read);
                        m_sw_info = swinfo.data;
                        num_bytes_to_read -= num_bytes_read;
                        LOG_INFO("read sw info chunk " << (num_bytes_to_read == 0 ? "succeeded" : "failed"))
                    }
                    break;
                    case core::file_types::chunk_id::chunk_capabilities:
                    {
                        std::vector<rs_capabilities> caps(chunk.size / sizeof(rs_capabilities));
                        m_file_data_read->read_bytes(caps.data(), chunk.size, num_bytes_read);
                        m_capabilities = caps;
                        num_bytes_to_read -= num_bytes_read;
                        LOG_INFO("read capabilities chunk " << (num_bytes_to_read == 0 ? "succeeded" : "failed"))
                    }
                    break;
                    case core::file_types::chunk_id::chunk_camera_info:
                    {
                        std::vector<uint8_t> info(num_bytes_to_read);
                        m_file_data_read->read_bytes(info.data(), static_cast<uint32_t>(num_bytes_to_read), num_bytes_read);

                        for(uint8_t* it = info.data(); it < info.data() + num_bytes_to_read; )
                        {
                            rs_camera_info id = *(reinterpret_cast<rs_camera_info*>(it));
                            it += sizeof(id);

                            uint32_t cam_info_size = *(reinterpret_cast<uint32_t*>(it));;
                            it += sizeof(cam_info_size);

                            char* cam_info = reinterpret_cast<char*>(it);
                            it += cam_info_size;

                            m_camera_info.emplace(id, std::string(cam_info));
                        }
                        num_bytes_to_read -= num_bytes_read;
                        LOG_INFO("read device info chunk " << (num_bytes_to_read == 0 ? "succeeded" : "failed"))
                    }
                    break;
                    default:
                        std::vector<uint8_t> data(num_bytes_to_read);
                        m_file_data_read->read_bytes(&data[0], static_cast<uint32_t>(num_bytes_to_read), num_bytes_read);
                        m_unknowns[chunk.id] = data;
                        num_bytes_to_read -= num_bytes_read;
                        LOG_INFO("read unknown chunk " << (num_bytes_to_read == 0 ? "succeeded" : "failed") << "chunk id - " << chunk.id)
                }
                if (num_bytes_to_read > 0) return core::status_item_unavailable;
            }
            return core::status_no_error;
        }

        void disk_read::index_next_samples(uint32_t number_of_samples)
        {
            if (m_is_index_complete) return;

            std::lock_guard<std::mutex> guard(m_mutex);

            for (uint32_t index = 0; index < number_of_samples;)
            {
                core::file_types::chunk_info chunk = {};
                uint32_t nbytesRead = 0;
                auto sts = m_file_indexing->read_bytes(&chunk, sizeof(chunk), nbytesRead);
                if (sts != core::status::status_no_error)
                {
                    m_is_index_complete = true;
                    LOG_INFO("samples indexing is done");
                    break;
                }
                if(chunk.id == core::file_types::chunk_id::chunk_sample_info)
                {
                    core::file_types::disk_format::sample_info si;
                    m_file_indexing->read_bytes(&si, static_cast<uint32_t>(std::min((long unsigned)chunk.size, (unsigned long)sizeof(si))), nbytesRead);
                    auto sample_info = si.data;
                    //old files of version 2 were recorded with milliseconds capture time unit
                    if(sample_info.capture_time_unit == core::file_types::time_unit::milliseconds)
                        sample_info.capture_time *= 1000;
                    core::file_types::chunk_info chunk2 = {};
                    m_file_indexing->read_bytes(&chunk2, sizeof(chunk2), nbytesRead);
                    switch(sample_info.type)
                    {
                        case core::file_types::sample_type::st_image:
                        {
                            core::file_types::disk_format::frame_info fi = {};
                            m_file_indexing->read_bytes(&fi, static_cast<uint32_t>(std::min((long unsigned)chunk2.size, (unsigned long)sizeof(fi))), nbytesRead);
                            core::file_types::frame_info frame_info = fi.data;
                            frame_info.index_in_stream = static_cast<uint32_t>(m_image_indices[frame_info.stream].size());
                            m_image_indices[frame_info.stream].push_back(static_cast<uint32_t>(m_samples_desc.size()));
                            m_samples_desc.push_back(std::make_shared<core::file_types::frame_sample>(frame_info, sample_info));
                            ++index;
                            LOG_VERBOSE("frame sample indexed, sample time - " << sample_info.capture_time)
                            break;
                        }
                        case core::file_types::sample_type::st_motion:
                        {
                            core::file_types::disk_format::motion_data md = {};
                            m_file_indexing->read_bytes(&md, static_cast<uint32_t>(std::min((long unsigned)chunk2.size, (unsigned long)sizeof(md))), nbytesRead);
                            rs_motion_data motion_data = md.data;
                            m_samples_desc.push_back(std::make_shared<core::file_types::motion_sample>(motion_data, sample_info));
                            ++index;
                            LOG_VERBOSE("motion sample indexed, sample time - " << sample_info.capture_time)
                            break;
                        }
                        case core::file_types::sample_type::st_time:
                        {
                            core::file_types::disk_format::time_stamp_data tsd = {};
                            m_file_indexing->read_bytes(&tsd, static_cast<uint32_t>(std::min((long unsigned)chunk2.size, (unsigned long)sizeof(tsd))), nbytesRead);
                            rs_timestamp_data time_stamp_data = tsd.data;
                            m_samples_desc.push_back(std::make_shared<core::file_types::time_stamp_sample>(time_stamp_data, sample_info));
                            ++index;
                            LOG_VERBOSE("time stamp sample indexed, sample time - " << sample_info.capture_time)
                            break;
                        }
                    }
                }
                else
                {
                    m_file_indexing->set_position(chunk.size, core::move_method::current);
                    continue;
                }
            }
        }

        int32_t disk_read::size_of_pitches(void)
        {
            return 0;
        }

        uint32_t disk_read::read_frame_metadata(const std::shared_ptr<core::file_types::frame_sample>& frame, unsigned long num_bytes_to_read)
        {
            using metadata_pair_type = decltype(frame->metadata)::value_type; //gets the pair<K,V> of the map
            assert(num_bytes_to_read != 0); //if the chunk size is 0 there shouldn't be a chunk
            assert(num_bytes_to_read % sizeof(metadata_pair_type) == 0); //nbytesToRead must be a multiplication of sizeof(metadataPairType)
            auto num_pairs = num_bytes_to_read / sizeof(metadata_pair_type);
            std::vector<metadata_pair_type> metadata_pairs(num_pairs);
            uint32_t num_bytes_read = 0;
            m_file_data_read->read_bytes(metadata_pairs.data(), static_cast<unsigned int>(num_bytes_to_read), num_bytes_read);
            for(uint32_t i = 0; i < num_pairs; i++)
            {
                frame->metadata.emplace(metadata_pairs[i].first, metadata_pairs[i].second);
            }
            return num_bytes_read;
        }
    }
}
