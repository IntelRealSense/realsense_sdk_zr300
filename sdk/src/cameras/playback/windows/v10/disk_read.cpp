// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "windows/v10/disk_read.h"
#include "windows/v10/file_types.h"
#include "windows/v10/conversions.h"
#include "include/file.h"
#include "rs/utils/log_utils.h"

namespace
{
    static rs_capabilities get_capability(rs_stream stream)
    {
        switch(stream)
        {
            case rs_stream::RS_STREAM_COLOR: return rs_capabilities::RS_CAPABILITIES_COLOR;
            case rs_stream::RS_STREAM_DEPTH: return rs_capabilities::RS_CAPABILITIES_DEPTH;
            case rs_stream::RS_STREAM_INFRARED: return rs_capabilities::RS_CAPABILITIES_INFRARED;
            case rs_stream::RS_STREAM_INFRARED2: return rs_capabilities::RS_CAPABILITIES_INFRARED2;
            case rs_stream::RS_STREAM_FISHEYE: return rs_capabilities::RS_CAPABILITIES_FISH_EYE;
            default: return rs_capabilities::RS_CAPABILITIES_COUNT;
        }
    }
}

using namespace rs::playback::windows::v10::file_types;

namespace rs
{
    namespace playback
    {
        namespace windows
        {
            namespace v10
            {
                struct FrameInfo
                {
                    int64_t offset;					// file offset in bytes
                    int64_t timeStamp;				// the time stamp in 100ns.
                    int32_t syncId;				    // syncId in the file
                };

                disk_read::~disk_read(void)
                {
                    LOG_FUNC_SCOPE();
                    pause();
                }

                void disk_read::handle_ds_projection(std::vector<uint8_t> &projection_data)
                {
                    LOG_INFO("handle ds projection")
                    auto data = &(projection_data.data()[640]); // 640 is the size of PXCSerializableService::ProfileInfo
                    ds_projection::ProjectionDataVersions projectionDataVersion = (ds_projection::ProjectionDataVersions)((unsigned int)(*data));
                    ds_projection::ProjectionData* projection = NULL;
                    ds_projection::ProjectionData v0TmpProjection;
                    switch (projectionDataVersion)
                    {
                        case ds_projection::ProjectionDataVersions::VERSION_0:
                        {
                            ds_projection::Version0_ProjectionData* version0ProjectionData = reinterpret_cast<ds_projection::Version0_ProjectionData*>(data);
                            v0TmpProjection = *version0ProjectionData;
                            projection = &v0TmpProjection;
                        }
                        break;
                        case ds_projection::ProjectionDataVersions::VERSION_1:
                            projection = reinterpret_cast<ds_projection::ProjectionData*>(data);
                            break;
                    }
                    m_streams_infos[rs_stream::RS_STREAM_COLOR].profile.intrinsics = conversions::get_intrinsics(file_types::stream_type::stream_type_color, projection);
                    m_streams_infos[rs_stream::RS_STREAM_DEPTH].profile.intrinsics = conversions::get_intrinsics(file_types::stream_type::stream_type_depth, projection);
                    m_streams_infos[rs_stream::RS_STREAM_COLOR].profile.extrinsics = conversions::get_extrinsics(file_types::stream_type::stream_type_color, projection);
                    m_streams_infos[rs_stream::RS_STREAM_DEPTH].profile.extrinsics = { 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 };
                }

                core::status disk_read::read_headers()
                {
                    /* Get the file header */
                    m_file_data_read->set_position(0, core::move_method::begin);
                    disk_format::header header = {};
                    auto data_read_status = m_file_data_read->read_to_object(header);
                    if(data_read_status == core::status_no_error)
                        data_read_status = conversions::convert(header, m_file_header);
                    if(data_read_status != core::status_no_error)
                        return core::status_item_unavailable;
                    if (header.version >= 8)
                        data_read_status = conversions::convert(header.coordinate_system, m_file_header.coordinate_system);
                    /* Get all chunks */
                    while (data_read_status == core::status_no_error)
                    {
                        disk_format::chunk chunk = {};
                        data_read_status = m_file_data_read->read_to_object(chunk);
                        if (data_read_status != core::status_no_error || chunk.chunk_id == file_types::disk_format::chunk_frame_meta_data)
                            return data_read_status;

                        switch (chunk.chunk_id)
                        {
                            case file_types::disk_format::chunk_deviceinfo:
                            {
                                file_types::disk_format::device_info_disk did = {};
                                data_read_status = m_file_data_read->read_to_object(did, chunk.chunk_size);
                                if(data_read_status == core::status_no_error)
                                    data_read_status = conversions::convert(did, m_camera_info);
                                LOG_INFO("read device info chunk " << (data_read_status == core::status_no_error ? "succeeded" : "failed"));
                            }
                            break;
                            case file_types::disk_format::chunk_profiles:
                            {
                                file_types::disk_format::stream_profile_set_disk spsd = {};
                                data_read_status = m_file_data_read->read_to_object(spsd, chunk.chunk_size);
                                if(data_read_status == core::status_no_error)
                                    data_read_status = conversions::convert(spsd, m_streams_infos);
                                LOG_INFO("read profiles chunk " << (data_read_status == core::status_no_error ? "succeeded" : "failed"));
                            }
                            break;
                            case file_types::disk_format::chunk_serializeable:
                            {
                                file_types::property label = static_cast<file_types::property>(0);
                                data_read_status = m_file_data_read->read_to_object(label);
                                if(data_read_status == core::status_no_error)
                                {
                                    if (label == file_types::property::property_projection_serializable)
                                    {
                                        auto str = m_camera_info.at(rs_camera_info::RS_CAMERA_INFO_DEVICE_NAME);
                                        std::size_t found = str.find("R200");
                                        if (found != std::string::npos)
                                        {
                                            auto data_size = static_cast<uint32_t>(chunk.chunk_size - sizeof(label));
                                            std::vector<uint8_t> data(data_size);
                                            data_read_status = m_file_data_read->read_to_object_array(data);

                                            if(data_read_status == core::status_no_error)
                                                handle_ds_projection(data);
                                        }
                                    }
                                }
                                LOG_INFO("read serializeable chunk " << (data_read_status == core::status_no_error ? "succeeded" : "failed"));
                            }
                            break;
                            case file_types::disk_format::chunk_streaminfo:
                            {
                                uint32_t stream_count = static_cast<uint32_t>(chunk.chunk_size / sizeof(disk_format::stream_info));

                                std::vector<disk_format::stream_info> stream_infos(stream_count);
                                data_read_status = m_file_data_read->read_to_object_array(stream_infos);
                                if(data_read_status == core::status_no_error)
                                {
                                    for (auto &stream_info : stream_infos)
                                    {
                                        core::file_types::stream_info si = {};
                                        auto sts = conversions::convert(stream_info, si);
                                        if(sts == core::status_feature_unsupported)
                                            continue; //ignore unsupported streams
                                        data_read_status = sts;
                                        if(data_read_status != core::status_no_error)
                                            break;
                                        m_streams_infos[si.stream] = si;
                                        auto cap = get_capability(si.stream);
                                        if(cap != rs_capabilities::RS_CAPABILITIES_COUNT)
                                            m_capabilities.push_back(cap);
                                    }
                                }
                                LOG_INFO("read stream info chunk " << (data_read_status == core::status_no_error ? "succeeded" : "failed"));
                            }
                            break;
                            default:
                            {
                                m_file_data_read->set_position(chunk.chunk_size, core::move_method::current);
                                LOG_INFO("ignore chunk, "<< "chunk id - " << chunk.chunk_id);
                            }
                            break;
                        }
                        if (data_read_status != core::status_no_error)
                            return core::status_item_unavailable;
                    }
                    return data_read_status;
                }

                int32_t disk_read::size_of_pitches(void)
                {
                    return sizeof(int32_t) * NUM_OF_PLANES;
                }

                void disk_read::index_next_samples(uint32_t number_of_samples)
                {
                    if (m_is_index_complete) return;

                    std::lock_guard<std::mutex> guard(m_mutex);

                    for (uint32_t index = 0; index < number_of_samples;)
                    {
                        disk_format::chunk chunk = {};
                        core::status data_read_status = m_file_indexing->read_to_object(chunk);
                        if (data_read_status != core::status_no_error || chunk.chunk_size <= 0 || chunk.chunk_size > 100000000 /*invalid chunk*/)
                        {
                            m_is_index_complete = true;
                            LOG_INFO("samples indexing is done")
                            break;
                        }
                        switch(chunk.chunk_id)
                        {
                            case file_types::disk_format::chunk_frame_meta_data:
                            {
                                file_types::disk_format::frame_metadata mdata = {};
                                uint32_t so = static_cast<uint32_t>(m_file_header.version < 10 ? 24 : sizeof(mdata));
                                data_read_status = m_file_indexing->read_to_object(mdata, so);
                                if(data_read_status != core::status_no_error)
                                    break;
                                core::file_types::sample_info sample_info = {};
                                core::file_types::frame_info frame_info = {};
                                data_read_status = conversions::convert(mdata, frame_info);
                                if(data_read_status != core::status_no_error)
                                    break;
                                auto ts = frame_info.time_stamp;
                                auto st = frame_info.stream;
                                auto fn = frame_info.number;
                                frame_info = m_streams_infos[frame_info.stream].profile.info;
                                frame_info.time_stamp = ts;
                                frame_info.stream = st;
                                frame_info.number = fn;
                                if(m_time_stamp_base == 0) m_time_stamp_base = static_cast<uint64_t>(frame_info.time_stamp);
                                frame_info.time_stamp -= static_cast<double>(m_time_stamp_base);
                                sample_info.type = core::file_types::sample_type::st_image;
                                sample_info.capture_time = static_cast<uint64_t>(frame_info.time_stamp);
                                m_file_indexing->get_position(&sample_info.offset);
                                frame_info.index_in_stream = static_cast<uint32_t>(m_image_indices[frame_info.stream].size());
                                m_image_indices[frame_info.stream].push_back(static_cast<uint32_t>(m_samples_desc.size()));
                                m_samples_desc.push_back(std::make_shared<core::file_types::frame_sample>(frame_info, sample_info));
                                ++index;
                                LOG_VERBOSE("frame sample indexed, sample time - " << sample_info.capture_time)
                            }
                            break;
                            default:
                            {
                                // skip any other sections
                                m_file_indexing->set_position(chunk.chunk_size, core::move_method::current);
                            }
                        }
                        if (data_read_status != core::status_no_error)
                            break;
                    }
                }

                uint32_t disk_read::read_frame_metadata(const std::shared_ptr<core::file_types::frame_sample> & frame, unsigned long num_bytes_to_read)
                {
                    //Does not do anything at the moment
                    m_file_data_read->set_position(num_bytes_to_read, core::move_method::current);
                    return 0;
                }
            }
        }
    }
}

