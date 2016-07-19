// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved

#include "windows/disk_read_windows.h"
#include "windows/file_types_windows.h"
#include "windows/conversions.h"
#include "file/file.h"
#include "rs/utils/log_utils.h"

using namespace rs::PXC;
using namespace rs::playback;

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
            default: return rs_capabilities::RS_CAPABILITIES_MAX_ENUM;
        }
    }
}

struct FrameInfo
{
    int64_t offset;					// file offset in bytes
    int64_t timeStamp;				// the time stamp in 100ns.
    int32_t syncId;				    // syncId in the file
};

disk_read_windows::~disk_read_windows(void)
{
    LOG_FUNC_SCOPE();
    pause();
}

void disk_read_windows::handle_ds_projection(std::vector<uint8_t> &projection_data)
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
    m_streams_infos[rs_stream::RS_STREAM_COLOR].profile.intrinsics = PXC::Conversions::get_intrinsics(StreamType::STREAM_TYPE_COLOR, projection);
    m_streams_infos[rs_stream::RS_STREAM_DEPTH].profile.intrinsics = PXC::Conversions::get_intrinsics(StreamType::STREAM_TYPE_DEPTH, projection);
    m_streams_infos[rs_stream::RS_STREAM_COLOR].profile.extrinsics = PXC::Conversions::get_extrinsics(StreamType::STREAM_TYPE_COLOR, projection);
}

status disk_read_windows::read_headers()
{
    /* Get the file header */
    m_file_data_read->set_position(0, move_method::begin);
    uint32_t nbytesRead = 0;
    unsigned long nbytesToRead = 0;
    CMDiskFormat::Header header;
    m_file_data_read->read_bytes(&header, sizeof(header), nbytesRead);
    if(Conversions::convert(header, m_file_header) != status_no_error) return status_item_unavailable;
    if (nbytesRead < sizeof(m_file_header)) return status_item_unavailable;
    if (m_file_header.id != UID('R', 'S', 'C', 'F')) return status_param_unsupported;
    if (header.version >= 8)
        Conversions::convert(header.coordinateSystem, m_file_header.coordinate_system);
    /* Get all chunks */
    for (;;)
    {
        CMDiskFormat::Chunk chunk = {};
        m_file_data_read->read_bytes(&chunk, sizeof(chunk), nbytesRead);
        if (nbytesRead < sizeof(chunk)) break;
        if (chunk.chunkId == CMDiskFormat::CHUNK_FRAME_META_DATA) break;
        nbytesToRead = chunk.chunkSize;
        switch (chunk.chunkId)
        {
            case CMDiskFormat::CHUNK_DEVICEINFO:
            {
                CMDiskFormat::DeviceInfoDisk did;
                m_file_data_read->read_bytes(&did, std::min(nbytesToRead, (unsigned long)sizeof(did)), nbytesRead);
                if(Conversions::convert(did, m_device_info) != status_no_error) return status_item_unavailable;
                nbytesToRead -= nbytesRead;
                LOG_INFO("read device info chunk " << (nbytesToRead == 0 ? "succeeded" : "failed"))
            }
            break;
            case CMDiskFormat::CHUNK_PROFILES:
                CMDiskFormat::StreamProfileSetDisk spsd;
                m_file_data_read->read_bytes(&spsd, std::min(nbytesToRead, (unsigned long)sizeof(spsd)), nbytesRead);
                if(Conversions::convert(spsd, m_streams_infos) != status_no_error) return status_item_unavailable;
                nbytesToRead -= nbytesRead;
                LOG_INFO("read profiles chunk " << (nbytesToRead == 0 ? "succeeded" : "failed"))
                break;
            case CMDiskFormat::CHUNK_PROPERTIES://TODO - create conversion table
                do
                {
                    DeviceCap devcap = {};
                    m_file_data_read->read_bytes(&devcap, std::min(nbytesToRead, (unsigned long)sizeof(devcap)), nbytesRead);
                    //if(Conversions::convert(devcap, option) != status_no_error) return status_item_unavailable;
                    nbytesToRead -= nbytesRead;
                }
                while (nbytesToRead > 0 && nbytesRead > 0);
                LOG_INFO("read properties chunk " << (nbytesToRead == 0 ? "succeeded" : "failed"))
                break;
            case CMDiskFormat::CHUNK_SERIALIZEABLE:
            {
                Property label = (Property)0;
                m_file_data_read->read_bytes(&label, std::min(nbytesToRead, (unsigned long)sizeof(label)), nbytesRead);
                nbytesToRead -= nbytesRead;
                std::vector<uint8_t> data(nbytesToRead);
                m_file_data_read->read_bytes(data.data(), nbytesToRead, nbytesRead);
                nbytesToRead -= nbytesRead;
                LOG_INFO("read serializeable chunk " << (nbytesToRead == 0 ? "succeeded" : "failed"))

                if (label == Property::PROPERTY_PROJECTION_SERIALIZABLE)
                {
                    auto str = std::string(m_device_info.name);
                    std::size_t found = str.find("R200");
                    if (found!=std::string::npos)
                    {
                        handle_ds_projection(data);
                    }
                }
            }
            break;
            case CMDiskFormat::CHUNK_STREAMINFO:
                for (int i = 0; i < m_file_header.nstreams; i++)
                {
                    CMDiskFormat::StreamInfo  stream_info1 = {};
                    m_file_data_read->read_bytes(&stream_info1, std::min(nbytesToRead, (unsigned long)sizeof(stream_info1)), nbytesRead);
                    file_types::stream_info si;
                    if(Conversions::convert(stream_info1, si) != status_no_error) return status_item_unavailable;
                    m_streams_infos[si.stream] = si;
                    auto cap = get_capability(si.stream);
                    if(cap != rs_capabilities::RS_CAPABILITIES_MAX_ENUM)
                        m_capabilities.push_back(cap);
                    nbytesToRead -= nbytesRead;
                }
                LOG_INFO("read stream info chunk " << (nbytesToRead == 0 ? "succeeded" : "failed"))
                break;
            default:
                std::vector<uint8_t> data(nbytesToRead);
                m_file_data_read->read_bytes(&data[0], nbytesToRead, nbytesRead);
                m_unknowns[(file_types::chunk_id)chunk.chunkId] = data;
                nbytesToRead -= nbytesRead;
                LOG_INFO("read unknown chunk " << (nbytesToRead == 0 ? "succeeded" : "failed") << "chunk id - " << chunk.chunkId)
        }
        if (nbytesToRead > 0) return status_item_unavailable;
    }
    return status_no_error;
}

int32_t disk_read_windows::size_of_pitches(void)
{
    return sizeof(int32_t) * NUM_OF_PLANES;
}

void disk_read_windows::index_next_samples(uint32_t number_of_samples)
{
    if (m_is_index_complete) return;

    std::lock_guard<std::mutex> guard(m_mutex);

    for (uint32_t index = 0; index < number_of_samples;)
    {
        CMDiskFormat::Chunk chunk = {};
        uint32_t nbytesRead = 0;       
        m_file_indexing->read_bytes(&chunk, sizeof(chunk), nbytesRead);
        if (nbytesRead < sizeof(chunk) || chunk.chunkSize <= 0 || chunk.chunkSize > 100000000 /*invalid chunk*/)
        {
            m_is_index_complete = true;
            LOG_INFO("samples indexing is done")
            break;
        }
        unsigned long nbytesToRead = chunk.chunkSize;
        if (chunk.chunkId == CMDiskFormat::CHUNK_FRAME_META_DATA)
        {
            CMDiskFormat::FrameMetaData mdata = {};
            auto so = m_file_header.version < 10 ? 24 : (unsigned long)sizeof(mdata);
            m_file_indexing->read_bytes(&mdata, so, nbytesRead);
            file_types::sample_info sample_info;
            file_types::frame_info frame_info;
            if(Conversions::convert(mdata, frame_info) != status_no_error) continue;
            auto ts = frame_info.time_stamp;
            auto st = frame_info.stream;
            frame_info = m_streams_infos[frame_info.stream].profile.info;
            frame_info.time_stamp = ts;
            frame_info.stream = st;
            if(m_time_stamp_base == 0) m_time_stamp_base = frame_info.time_stamp;
            frame_info.time_stamp -= m_time_stamp_base;
            frame_info.number = frame_info.time_stamp;//librealsense bug - replace to time stamp when fixed
            sample_info.type = file_types::sample_type::st_image;
            sample_info.capture_time = frame_info.time_stamp;
            m_file_indexing->get_position(&sample_info.offset);
            frame_info.index_in_stream = m_image_indices[frame_info.stream].size();
            m_image_indices[frame_info.stream].push_back(m_samples_desc.size());
            m_samples_desc.push_back(std::make_shared<file_types::frame_sample>(frame_info, sample_info));
            ++index;
            LOG_VERBOSE("frame sample indexed, sample time - " << sample_info.capture_time)
        }
        else
        {
            // skip any other sections
            m_file_indexing->set_position(chunk.chunkSize, move_method::current);
        }
    }
}
