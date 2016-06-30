// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved

#include "linux/disk_read_linux.h"
#include "file/file.h"
#include "rs/utils/log_utils.h"

using namespace rs::core;
using namespace rs::playback;

disk_read_linux::~disk_read_linux(void)
{
    LOG_FUNC_SCOPE();
    set_pause(true);
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

status disk_read_linux::read_headers(void)
{
    /* Get the file header */
    m_file_info->set_position(0, move_method::begin);
    uint32_t nbytesRead;
    unsigned long nbytesToRead = 0;
    file_types::disk_format::file_header fh;
    m_file_info->read_bytes(&fh, sizeof(fh), nbytesRead);
    m_file_header = fh.data;
    if (nbytesRead < sizeof(m_file_header)) return status_item_unavailable;
    if (m_file_header.id != UID('R', 'S', 'L', 'X')) return status_param_unsupported;
    m_coordinate_system = m_file_header.coordinate_system;

    /* Get all chunks */
    for (;;)
    {
        file_types::chunk_info chunk = {};
        m_file_info->read_bytes(&chunk, sizeof(chunk), nbytesRead);
        if (nbytesRead < sizeof(chunk)) break;
        if (chunk.id == file_types::chunk_id::chunk_sample_type) break;
        nbytesToRead = chunk.size;
        switch (chunk.id)
        {
            case file_types::chunk_id::chunk_device_info:
            {
                file_types::disk_format::device_info dinfo;
                m_file_info->read_bytes(&dinfo, std::min(nbytesToRead, (unsigned long)sizeof(dinfo)), nbytesRead);
                m_device_info = dinfo.data;
                nbytesToRead -= nbytesRead;
                LOG_INFO("read device info chunk " << (nbytesToRead == 0 ? "succeeded" : "failed"))
            }
            break;
            case file_types::chunk_id::chunk_properties:
                do
                {
                    file_types::device_cap devcap = {};
                    m_file_info->read_bytes(&devcap, std::min(nbytesToRead, (unsigned long)sizeof(devcap)), nbytesRead);
                    m_options[devcap.label] = devcap.value;
                    nbytesToRead -= nbytesRead;
                }
                while (nbytesToRead > 0 && nbytesRead > 0);
                LOG_INFO("read properties chunk " << (nbytesToRead == 0 ? "succeeded" : "failed"))
                break;
            case file_types::chunk_id::chunk_serializeable:
            {
                rs_option label = (rs_option)0;
                m_file_info->read_bytes(&label, std::min(nbytesToRead, (unsigned long)sizeof(label)), nbytesRead);
                nbytesToRead -= nbytesRead;
                std::vector<uint8_t> data(nbytesToRead);
                m_file_info->read_bytes(data.data(), nbytesToRead, nbytesRead);
                nbytesToRead -= nbytesRead;
                LOG_INFO("read serializeable chunk " << (nbytesToRead == 0 ? "succeeded" : "failed"))
            }
            break;
            case file_types::chunk_id::chunk_stream_info:
                for (int i = 0; i < m_file_header.nstreams; i++)
                {
                    file_types::disk_format::stream_info stream_info1 = {};
                    m_file_info->read_bytes(&stream_info1, std::min(nbytesToRead, (unsigned long)sizeof(stream_info1)), nbytesRead);
                    m_streams_infos[stream_info1.data.stream] = stream_info1.data;
                    nbytesToRead -= nbytesRead;
                }
                LOG_INFO("read stream info chunk " << (nbytesToRead == 0 ? "succeeded" : "failed"))
                break;
            case file_types::chunk_id::chunk_sw_info:
            {
                file_types::disk_format::sw_info swinfo;
                m_file_info->read_bytes(&swinfo, std::min(nbytesToRead, (unsigned long)sizeof(swinfo)), nbytesRead);
                m_sw_info = swinfo.data;
                nbytesToRead -= nbytesRead;
                LOG_INFO("read sw info chunk " << (nbytesToRead == 0 ? "succeeded" : "failed"))
            }
            break;
            case file_types::chunk_id::chunk_capabilities:
            {
                std::vector<rs_capabilities> caps(chunk.size / sizeof(rs_capabilities));
                m_file_info->read_bytes(caps.data(), chunk.size, nbytesRead);
                m_capabilities = caps;
                nbytesToRead -= nbytesRead;
                LOG_INFO("read capabilities chunk " << (nbytesToRead == 0 ? "succeeded" : "failed"))
            }
            break;
            default:
                std::vector<uint8_t> data(nbytesToRead);
                m_file_info->read_bytes(&data[0], nbytesToRead, nbytesRead);
                m_unknowns[chunk.id] = data;
                nbytesToRead -= nbytesRead;
                LOG_INFO("read unknown chunk " << (nbytesToRead == 0 ? "succeeded" : "failed") << "chunk id - " << chunk.id)
        }
        if (nbytesToRead > 0) return status_item_unavailable;
    }
    return status_no_error;
}

void disk_read_linux::index_next_samples(uint32_t number_of_samples)
{
    if (m_is_indexed) return;

    std::lock_guard<std::mutex> guard(m_mutex);

    for (uint32_t index = 0; index < number_of_samples;)
    {
        file_types::chunk_info chunk = {};
        uint32_t nbytesRead;
        m_file_index->read_bytes(&chunk, sizeof(chunk), nbytesRead);
        if (nbytesRead < sizeof(chunk) || chunk.size <= 0 || chunk.size > 100000000 /*invalid chunk*/)
        {
            m_is_indexed = true;
            LOG_INFO("samples indexing is done")
            break;
        }
        if(chunk.id == file_types::chunk_id::chunk_sample_type)
        {
            file_types::sample_type st;
            m_file_index->read_bytes(&st, std::min((long unsigned)chunk.size, (unsigned long)sizeof(st)), nbytesRead);
            file_types::chunk_info chunk2 = {};
            m_file_index->read_bytes(&chunk2, sizeof(chunk2), nbytesRead);
            switch(st)
            {
                case file_types::sample_type::st_image:
                {
                    file_types::disk_format::frame_info fi = {};
                    m_file_index->read_bytes(&fi, std::min((long unsigned)chunk2.size, (unsigned long)sizeof(fi)), nbytesRead);
                    file_types::frame_info frame_info = fi.data;
                    frame_info.index_in_stream = m_image_indexes[frame_info.stream].size();
                    m_image_indexes[frame_info.stream].push_back(m_samples.size());
                    m_samples.push_back(std::make_shared<file_types::frame>(frame_info));
                    ++index;
                    LOG_VERBOSE("frame sample indexed, sample time - " << frame_info.sync)
                    break;
                }
                case file_types::sample_type::st_motion:
                {
                    file_types::disk_format::motion_info mi = {};
                    m_file_index->read_bytes(&mi, std::min((long unsigned)chunk2.size, (unsigned long)sizeof(mi)), nbytesRead);
                    file_types::motion_info motion_info = mi.data;
                    m_samples.push_back(std::make_shared<file_types::motion>(motion_info));
                    ++index;
                    LOG_VERBOSE("motion sample indexed, sample time - " << motion_info.sync)
                    break;
                }
                case file_types::sample_type::st_time:
                {
                    file_types::disk_format::time_stamp_info tsi = {};
                    m_file_index->read_bytes(&tsi, std::min((long unsigned)chunk2.size, (unsigned long)sizeof(tsi)), nbytesRead);
                    file_types::time_stamp_info time_stamp_info = tsi.data;
                    m_samples.push_back(std::make_shared<file_types::time_stamp>(time_stamp_info));
                    ++index;
                    LOG_VERBOSE("time stamp sample indexed, sample time - " << time_stamp_info.sync)
                    break;
                }
            }
        }
        else
        {
            m_file_index->set_position(chunk.size, move_method::current);
            continue;
        }
    }
}

int32_t disk_read_linux::size_of_pitches(void)
{
    return 0;
}
