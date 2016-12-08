// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "disk_read.h"
#include "linux/v1/disk_read.h"
#include "linux/v1/conversions.h"
#include "include/file.h"
#include "rs/utils/log_utils.h"

using namespace rs::playback::linux::v1::file_types;

namespace rs
{
    namespace playback
    {
        namespace linux
        {
            namespace v1
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

                    disk_format::file_header fh;
                    auto data_read_status = m_file_data_read->read_to_object(fh);
                    if (data_read_status != core::status_no_error)
                        return core::status_item_unavailable;
                    if(conversions::convert(fh.data, m_file_header) != core::status::status_no_error)
                        return core::status::status_item_unavailable;
                    if (m_file_header.id != UID('R', 'S', 'L', '1')) return core::status_param_unsupported;

                    /* Get all chunks */
                    while (data_read_status == core::status_no_error)
                    {
                        chunk_info chunk = {};
                        data_read_status = m_file_data_read->read_to_object(chunk);
                        if (data_read_status != core::status_no_error || chunk.id == core::file_types::chunk_id::chunk_sample_info)
                            break;
                        switch (chunk.id)
                        {
                            case core::file_types::chunk_id::chunk_device_info:
                            {
                                file_types::disk_format::device_info dinfo;
                                data_read_status = m_file_data_read->read_to_object(dinfo, chunk.size);
                                if(conversions::convert(dinfo.data, m_camera_info) != core::status::status_no_error)
                                    return core::status::status_item_unavailable;
                                LOG_INFO("read device info chunk " << (data_read_status == core::status_no_error ? "succeeded" : "failed"));
                            }
                            break;
                            case core::file_types::chunk_id::chunk_properties:
                            {
                                uint32_t properties_count = static_cast<uint32_t>(chunk.size / sizeof(core::file_types::device_cap));
                                std::vector<core::file_types::device_cap> devcaps(properties_count);
                                data_read_status = m_file_data_read->read_to_object_array(devcaps);
                                for(auto & caps : devcaps)
                                {
                                    m_properties[caps.label] = caps.value;
                                }
                                LOG_INFO("read properties chunk " << (data_read_status == core::status_no_error ? "succeeded" : "failed"));
                            }
                            break;
                            case core::file_types::chunk_id::chunk_stream_info:
                            {
                                uint32_t stream_count = static_cast<uint32_t>(chunk.size / sizeof(disk_format::stream_info));

                                std::vector<disk_format::stream_info> stream_infos(stream_count);
                                data_read_status = m_file_data_read->read_to_object_array(stream_infos);

                                for (auto &stream_info : stream_infos)
                                {
                                    if(conversions::convert(stream_info.data, m_streams_infos[stream_info.data.stream]) != core::status::status_no_error)
                                        return core::status::status_item_unavailable;
                                }
                                LOG_INFO("read stream info chunk " << (data_read_status == core::status_no_error ? "succeeded" : "failed"));
                            }
                            break;
                            case core::file_types::chunk_id::chunk_sw_info:
                            {
                                disk_format::sw_info swinfo;
                                data_read_status = m_file_data_read->read_to_object(swinfo, chunk.size);
                                if(conversions::convert(swinfo.data, m_sw_info) != core::status::status_no_error)
                                    return core::status::status_item_unavailable;
                                LOG_INFO("read sw info chunk " << (data_read_status == core::status_no_error ? "succeeded" : "failed"));
                            }
                            break;
                            case core::file_types::chunk_id::chunk_capabilities:
                            {
                                uint32_t caps_count = static_cast<uint32_t>(chunk.size / sizeof(rs_capabilities));
                                m_capabilities.resize(caps_count);
                                data_read_status = m_file_data_read->read_to_object_array(m_capabilities);
                                LOG_INFO("read capabilities chunk " << (data_read_status == core::status_no_error ? "succeeded" : "failed"));
                            }
                            break;
                            default:
                            {
                                m_unknowns[chunk.id].resize(chunk.size);
                                data_read_status = m_file_data_read->read_to_object_array(m_unknowns[chunk.id]);

                                LOG_INFO("read unknown chunk " << (data_read_status == core::status_no_error ? "succeeded" : "failed") << "chunk id - " << chunk.id);
                            }
                            break;
                        }
                        if (data_read_status != core::status_no_error)
                            return core::status_item_unavailable;
                    }
                    return core::status::status_no_error;
                }

                void disk_read::index_next_samples(uint32_t number_of_samples)
                {
                    if (m_is_index_complete) return;

                    std::lock_guard<std::mutex> guard(m_mutex);

                    for (uint32_t index = 0; index < number_of_samples;)
                    {
                        core::file_types::chunk_info chunk = {};
                        bool data_read_status = m_file_indexing->read_to_object(chunk);
                        if (data_read_status != core::status_no_error)
                        {
                            m_is_index_complete = true;
                            LOG_INFO("samples indexing is done");
                            break;
                        }
                        if(chunk.id == core::file_types::chunk_id::chunk_sample_info)
                        {
                            file_types::disk_format::sample_info si;
                            data_read_status = m_file_indexing->read_to_object(si, chunk.size);
                            core::file_types::sample_info sample_info;
                            if(conversions::convert(si.data, sample_info) != core::status::status_no_error)
                                continue;
                            if(sample_info.capture_time_unit == core::file_types::time_unit::milliseconds)
                                sample_info.capture_time *= 1000;
                            core::file_types::chunk_info chunk2 = {};
                            data_read_status = m_file_indexing->read_to_object(chunk2);
                            switch(sample_info.type)
                            {
                                case core::file_types::sample_type::st_image:
                                {
                                    file_types::disk_format::frame_info fi = {};
                                    data_read_status = m_file_indexing->read_to_object(fi, chunk2.size);
                                    core::file_types::frame_info frame_info;
                                    if(conversions::convert(fi.data, frame_info) != core::status::status_no_error)
                                        throw std::runtime_error("failed to convert frame info");
                                    frame_info.index_in_stream = static_cast<uint32_t>(m_image_indices[frame_info.stream].size());
                                    m_image_indices[frame_info.stream].push_back(static_cast<uint32_t>(m_samples_desc.size()));
                                    m_samples_desc.push_back(std::make_shared<core::file_types::frame_sample>(frame_info, sample_info));
                                    ++index;
                                    LOG_VERBOSE("frame sample indexed, sample time - " << sample_info.capture_time)
                                    break;
                                }
                                case core::file_types::sample_type::st_motion:
                                {
                                    file_types::disk_format::motion_data md = {};
                                    data_read_status = m_file_indexing->read_to_object(md, chunk2.size);
                                    rs_motion_data motion_data = md.data;
                                    m_samples_desc.push_back(std::make_shared<core::file_types::motion_sample>(motion_data, sample_info));
                                    ++index;
                                    LOG_VERBOSE("motion sample indexed, sample time - " << sample_info.capture_time)
                                    break;
                                }
                                case core::file_types::sample_type::st_time:
                                {
                                    file_types::disk_format::time_stamp_data tsd = {};
                                    data_read_status = m_file_indexing->read_to_object(tsd, chunk2.size);
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
                    throw std::runtime_error("unsupported");
                }
            }
        }
    }
}
