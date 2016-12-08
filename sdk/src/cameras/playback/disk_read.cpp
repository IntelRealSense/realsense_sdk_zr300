// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "disk_read.h"
#include "include/file.h"
#include "rs/utils/log_utils.h"

using namespace rs::core;
using namespace rs::core::file_types;

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
            disk_format::file_header file_header;
            auto data_read_status = m_file_data_read->read_to_object(file_header);
            if (data_read_status != status::status_no_error)
                return core::status_item_unavailable;
            m_file_header = file_header.data;
            if (m_file_header.id != UID('R', 'S', 'L', '2'))
                return core::status_param_unsupported;

            /* Get all chunks */
            while (data_read_status == status::status_no_error)
            {
                chunk_info chunk = {};
                data_read_status = m_file_data_read->read_to_object(chunk);
                if (data_read_status != status::status_no_error || chunk.id == chunk_id::chunk_sample_info)
                    break;

                switch (chunk.id)
                {
                    case chunk_id::chunk_device_info:
                    {
                        disk_format::device_info dinfo = {};
                        data_read_status = m_file_data_read->read_to_object(dinfo, chunk.size);
                        if(data_read_status == status::status_no_error)
                        {
                            m_camera_info[rs_camera_info::RS_CAMERA_INFO_DEVICE_NAME] = dinfo.data.name;
                            m_camera_info[rs_camera_info::RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER] = dinfo.data.serial;
                            m_camera_info[rs_camera_info::RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION] = dinfo.data.camera_firmware;
                            m_camera_info[rs_camera_info::RS_CAMERA_INFO_ADAPTER_BOARD_FIRMWARE_VERSION] = dinfo.data.adapter_board_firmware;
                            m_camera_info[rs_camera_info::RS_CAMERA_INFO_MOTION_MODULE_FIRMWARE_VERSION] = dinfo.data.motion_module_firmware;
                        }
                        LOG_INFO("read device info chunk " << (data_read_status == status::status_no_error ? "succeeded" : "failed"));
                    }
                    break;
                    case chunk_id::chunk_properties:
                    {
                        uint32_t properties_count = static_cast<uint32_t>(chunk.size / sizeof(device_cap));
                        std::vector<device_cap> devcaps(properties_count);
                        data_read_status = m_file_data_read->read_to_object_array(devcaps);
                        for(auto & caps : devcaps)
                        {
                            m_properties[caps.label] = caps.value;
                        }
                        LOG_INFO("read properties chunk " << (data_read_status == status::status_no_error ? "succeeded" : "failed"));
                    }
                    break;
                    case chunk_id::chunk_stream_info:
                    {
                        uint32_t stream_count = static_cast<uint32_t>(chunk.size / sizeof(disk_format::stream_info));
                        std::vector<disk_format::stream_info> stream_infos(stream_count);
                        data_read_status = m_file_data_read->read_to_object_array(stream_infos);

                        for (auto &stream_info : stream_infos)
                        {
                            m_streams_infos[stream_info.data.stream] = stream_info.data;
                        }
                        LOG_INFO("read stream info chunk " << (data_read_status == status::status_no_error ? "succeeded" : "failed"));
                    }
                    break;
                    case chunk_id::chunk_motion_intrinsics:
                    {
                        disk_format::motion_intrinsics mi;
                        data_read_status = m_file_data_read->read_to_object(mi, chunk.size);

                        m_motion_intrinsics = mi.data;
                        LOG_INFO("read motion intrinsics chunk " << (data_read_status == status::status_no_error ? "succeeded" : "failed"));
                    }
                    break;
                    case chunk_id::chunk_sw_info:
                    {
                        disk_format::sw_info swinfo;
                        data_read_status = m_file_data_read->read_to_object(swinfo, chunk.size);

                        m_sw_info = swinfo.data;
                        LOG_INFO("read sw info chunk " << (data_read_status == status::status_no_error ? "succeeded" : "failed"));
                    }
                    break;
                    case chunk_id::chunk_capabilities:
                    {
                        uint32_t caps_count = static_cast<uint32_t>(chunk.size / sizeof(rs_capabilities));
                        m_capabilities.resize(caps_count);
                        data_read_status = m_file_data_read->read_to_object_array(m_capabilities);

                        LOG_INFO("read capabilities chunk " << (data_read_status == status::status_no_error ? "succeeded" : "failed"));
                    }
                    break;
                    case chunk_id::chunk_camera_info:
                    {
                        uint32_t num_bytes_to_read = chunk.size;
                        std::vector<uint8_t> info(num_bytes_to_read);
                        data_read_status = m_file_data_read->read_to_object_array(info);

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
                        LOG_INFO("read device info chunk " << (data_read_status == status::status_no_error ? "succeeded" : "failed"));
                    }
                    break;
                    default:
                    {
                        m_unknowns[chunk.id].resize(chunk.size);
                        data_read_status = m_file_data_read->read_to_object_array(m_unknowns[chunk.id]);

                        LOG_INFO("read unknown chunk " << (data_read_status == status::status_no_error ? "succeeded" : "failed") << "chunk id - " << chunk.id);
                    }
                    break;
                }
                if (data_read_status != status::status_no_error)
                    return core::status_item_unavailable;
            }
            return core::status_no_error;
        }

        void disk_read::index_next_samples(uint32_t number_of_samples)
        {
            if (m_is_index_complete) return;

            std::lock_guard<std::mutex> guard(m_mutex);

            for (uint32_t index = 0; index < number_of_samples;)
            {
                chunk_info chunk = {};
                bool data_read_status = m_file_indexing->read_to_object(chunk);
                if (data_read_status != status::status_no_error)
                {
                    m_is_index_complete = true;
                    LOG_INFO("samples indexing is done");
                    break;
                }
                switch(chunk.id)
                {
                    case chunk_id::chunk_sample_info:
                    {
                        disk_format::sample_info si;
                        data_read_status = m_file_indexing->read_to_object(si, chunk.size);

                        auto sample_info = si.data;
                        //old files of version 2 were recorded with milliseconds capture time unit
                        if(sample_info.capture_time_unit == time_unit::milliseconds)
                            sample_info.capture_time *= 1000;
                        chunk_info chunk2 = {};
                        data_read_status = m_file_indexing->read_to_object(chunk2);
                        switch(sample_info.type)
                        {
                            case sample_type::st_image:
                            {
                                disk_format::frame_info fi = {};
                                data_read_status = m_file_indexing->read_to_object(fi, chunk2.size);

                                frame_info frame_info = fi.data;
                                frame_info.index_in_stream = static_cast<uint32_t>(m_image_indices[frame_info.stream].size());
                                m_image_indices[frame_info.stream].push_back(static_cast<uint32_t>(m_samples_desc.size()));
                                m_samples_desc.push_back(std::make_shared<frame_sample>(frame_info, sample_info));
                                ++index;
                                LOG_VERBOSE("frame sample indexed, sample time - " << sample_info.capture_time)
                                break;
                            }
                            case sample_type::st_motion:
                            {
                                disk_format::motion_data md = {};
                                data_read_status = m_file_indexing->read_to_object(md, chunk2.size);

                                rs_motion_data motion_data = md.data;
                                m_samples_desc.push_back(std::make_shared<motion_sample>(motion_data, sample_info));
                                ++index;
                                LOG_VERBOSE("motion sample indexed, sample time - " << sample_info.capture_time)
                                break;
                            }
                            case sample_type::st_time:
                            {
                                disk_format::time_stamp_data tsd = {};
                                data_read_status = m_file_indexing->read_to_object(tsd, chunk2.size);

                                rs_timestamp_data time_stamp_data = tsd.data;
                                m_samples_desc.push_back(std::make_shared<time_stamp_sample>(time_stamp_data, sample_info));
                                ++index;
                                LOG_VERBOSE("time stamp sample indexed, sample time - " << sample_info.capture_time)
                                break;
                            }
                        }
                    }
                    break;
                    default:
                    {
                        m_file_indexing->set_position(chunk.size, core::move_method::current);
                        break;
                    }
                }
            }
        }

        int32_t disk_read::size_of_pitches(void)
        {
            return 0;
        }

        uint32_t disk_read::read_frame_metadata(const std::shared_ptr<frame_sample>& frame, unsigned long num_bytes_to_read)
        {
            using metadata_pair_type = decltype(frame->metadata)::value_type; //gets the pair<K,V> of the map
            assert(num_bytes_to_read != 0); //if the chunk size is 0 there shouldn't be a chunk
            if(num_bytes_to_read % sizeof(metadata_pair_type) != 0) //num_bytes_to_read must be a multiplication of sizeof(metadata_pair_type)
            {
                //in case data size is not valid move file pointer to the next chunk
                LOG_ERROR("failed to read frame metadata, metadata size is not valid");
                m_file_data_read->set_position(num_bytes_to_read, rs::core::move_method::current);
                return static_cast<uint32_t>(num_bytes_to_read);
            }
            auto num_pairs = num_bytes_to_read / sizeof(metadata_pair_type);
            std::vector<metadata_pair_type> metadata_pairs(num_pairs);
            uint32_t num_bytes_read = 0;
            m_file_data_read->read_to_object_array(metadata_pairs);
            for(uint32_t i = 0; i < num_pairs; i++)
            {
                frame->metadata.emplace(metadata_pairs[i].first, metadata_pairs[i].second);
            }
            return num_bytes_read;
        }
    }
}
