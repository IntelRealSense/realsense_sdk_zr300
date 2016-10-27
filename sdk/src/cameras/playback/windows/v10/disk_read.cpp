// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "windows/v10/disk_read.h"
#include "windows/v10/file_types.h"
#include "windows/v10/conversions.h"
#include "include/file.h"
#include "rs/utils/log_utils.h"

//using namespace rs::core;
//using namespace rs::windows;
//using namespace rs::playback;

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
                }

                core::status disk_read::read_headers()
                {
                    /* Get the file header */
                    m_file_data_read->set_position(0, core::move_method::begin);
                    uint32_t nbytesRead = 0;
                    unsigned long nbytesToRead = 0;
                    file_types::disk_format::header header;
                    m_file_data_read->read_bytes(&header, sizeof(header), nbytesRead);
                    if(conversions::convert(header, m_file_header) != core::status_no_error) return core::status_item_unavailable;
                    if (nbytesRead < sizeof(m_file_header)) return core::status_item_unavailable;
                    if (m_file_header.id != UID('R', 'S', 'C', 'F')) return core::status_param_unsupported;
                    if (header.version >= 8)
                        conversions::convert(header.coordinate_system, m_file_header.coordinate_system);
                    /* Get all chunks */
                    for (;;)
                    {
                        file_types::disk_format::chunk chunk = {};
                        m_file_data_read->read_bytes(&chunk, sizeof(chunk), nbytesRead);
                        if (nbytesRead < sizeof(chunk)) break;
                        if (chunk.chunk_id == file_types::disk_format::chunk_frame_meta_data) break;
                        nbytesToRead = chunk.chunk_size;
                        switch (chunk.chunk_id)
                        {
                            case file_types::disk_format::chunk_deviceinfo:
                            {
                                file_types::disk_format::device_info_disk did;
                                m_file_data_read->read_bytes(&did, static_cast<uint32_t>(std::min(nbytesToRead, (unsigned long)sizeof(did))), nbytesRead);
                                if(conversions::convert(did, m_camera_info) != core::status_no_error) return core::status_item_unavailable;
                                nbytesToRead -= nbytesRead;
                                LOG_INFO("read device info chunk " << (nbytesToRead == 0 ? "succeeded" : "failed"))
                            }
                            break;
                            case file_types::disk_format::chunk_profiles:
                                file_types::disk_format::stream_profile_set_disk spsd;
                                m_file_data_read->read_bytes(&spsd, static_cast<uint32_t>(std::min(nbytesToRead, (unsigned long)sizeof(spsd))), nbytesRead);
                                if(conversions::convert(spsd, m_streams_infos) != core::status_no_error) return core::status_item_unavailable;
                                nbytesToRead -= nbytesRead;
                                LOG_INFO("read profiles chunk " << (nbytesToRead == 0 ? "succeeded" : "failed"))
                                break;
                            case file_types::disk_format::chunk_properties://TODO - create conversion table
                                do
                                {
                                    file_types::device_cap devcap = {};
                                    m_file_data_read->read_bytes(&devcap, static_cast<uint32_t>(std::min(nbytesToRead, (unsigned long)sizeof(devcap))), nbytesRead);
                                    //if(Conversions::convert(devcap, option) !=core::status_no_error) returncore::status_item_unavailable;
                                    nbytesToRead -= nbytesRead;
                                }
                                while (nbytesToRead > 0 && nbytesRead > 0);
                                LOG_INFO("read properties chunk " << (nbytesToRead == 0 ? "succeeded" : "failed"))
                                break;
                            case file_types::disk_format::chunk_serializeable:
                            {
                                file_types::property label = (file_types::property)0;
                                m_file_data_read->read_bytes(&label, static_cast<uint32_t>(std::min(nbytesToRead, (unsigned long)sizeof(label))), nbytesRead);
                                nbytesToRead -= nbytesRead;
                                std::vector<uint8_t> data(nbytesToRead);
                                m_file_data_read->read_bytes(data.data(), static_cast<uint32_t>(nbytesToRead), nbytesRead);
                                nbytesToRead -= nbytesRead;
                                LOG_INFO("read serializeable chunk " << (nbytesToRead == 0 ? "succeeded" : "failed"))

                                if (label == file_types::property::property_projection_serializable)
                                {
                                    auto str = m_camera_info.at(rs_camera_info::RS_CAMERA_INFO_DEVICE_NAME);
                                    std::size_t found = str.find("R200");
                                    if (found!=std::string::npos)
                                    {
                                        handle_ds_projection(data);
                                    }
                                }
                            }
                            break;
                            case file_types::disk_format::chunk_streaminfo:
                                for (int i = 0; i < m_file_header.nstreams; i++)
                                {
                                    file_types::disk_format::stream_info  stream_info1 = {};
                                    m_file_data_read->read_bytes(&stream_info1, static_cast<uint32_t>(std::min(nbytesToRead, (unsigned long)sizeof(stream_info1))), nbytesRead);
                                    nbytesToRead -= nbytesRead;
                                    core::file_types::stream_info si;
                                    auto sts = conversions::convert(stream_info1, si);
                                    if(sts == core::status_feature_unsupported) continue; //ignore unsupported streams
                                    if(sts != core::status_no_error) return core::status_item_unavailable;
                                    m_streams_infos[si.stream] = si;
                                    auto cap = get_capability(si.stream);
                                    if(cap != rs_capabilities::RS_CAPABILITIES_COUNT)
                                        m_capabilities.push_back(cap);
                                }
                                LOG_INFO("read stream info chunk " << (nbytesToRead == 0 ? "succeeded" : "failed"))
                                break;
                            default:
                                std::vector<uint8_t> data(nbytesToRead);
                                m_file_data_read->read_bytes(&data[0], static_cast<uint32_t>(nbytesToRead), nbytesRead);
                                m_unknowns[(core::file_types::chunk_id)chunk.chunk_id] = data;
                                nbytesToRead -= nbytesRead;
                                LOG_INFO("read unknown chunk " << (nbytesToRead == 0 ? "succeeded" : "failed") << "chunk id - " << chunk.chunk_id)
                        }
                        if (nbytesToRead > 0) return core::status_item_unavailable;
                    }
                    return core::status_no_error;
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
                        file_types::disk_format::chunk chunk = {};
                        uint32_t nbytesRead = 0;
                        m_file_indexing->read_bytes(&chunk, sizeof(chunk), nbytesRead);
                        if (nbytesRead < sizeof(chunk) || chunk.chunk_size <= 0 || chunk.chunk_size > 100000000 /*invalid chunk*/)
                        {
                            m_is_index_complete = true;
                            LOG_INFO("samples indexing is done")
                            break;
                        }
                        unsigned long nbytesToRead = chunk.chunk_size;
                        if (chunk.chunk_id == file_types::disk_format::chunk_frame_meta_data)
                        {
                            file_types::disk_format::frame_metadata mdata = {};
                            unsigned long so = m_file_header.version < 10 ? 24 : (unsigned long)sizeof(mdata);
                            m_file_indexing->read_bytes(&mdata, static_cast<uint32_t>(so), nbytesRead);
                            core::file_types::sample_info sample_info;
                            core::file_types::frame_info frame_info;
                            if(conversions::convert(mdata, frame_info) != core::status_no_error) continue;
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
                        else
                        {
                            // skip any other sections
                            m_file_indexing->set_position(chunk.chunk_size, core::move_method::current);
                        }
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

