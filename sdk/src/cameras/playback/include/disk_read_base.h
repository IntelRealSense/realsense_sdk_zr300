// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <vector>
#include <queue>
#include <map>
#include <set>
#include <list>
#include <memory>
#include <mutex>
#include <thread>
#include <chrono>
#include "compression/compression_mock.h"
#include "include/file_types.h"
#include "status.h"
#include "disk_read_interface.h"
#include "file/file.h"

namespace rs
{
    namespace playback
    {
        class disk_read_base : public disk_read_interface
        {
        public:
            disk_read_base(const char *m_file_name);
            virtual ~disk_read_base(void);
            virtual core::status init() override;
            virtual void start() override;
            virtual void enable_stream(rs_stream stream, bool state) override;
            virtual void set_realtime(bool realtime) override;
            virtual void set_pause(bool m_pause) override;
            virtual bool set_frame_by_index(uint32_t index, rs_stream stream_type) override;
            virtual bool set_frame_by_time_stamp(uint64_t ts) override;
            virtual bool query_realtime() override { return m_realtime; }
            virtual bool is_stream_available(rs_stream stream, int width, int height, rs_format format, int framerate) override;
            virtual bool query_projection_change() { return m_projection_changed ; }
            virtual uint32_t query_number_of_frames(rs_stream stream_type) override;
            virtual int32_t query_coordinate_system() override { return m_coordinate_system; }
            virtual std::shared_ptr<core::file_types::frame> get_image_sample(rs_stream stream) override;
            virtual std::map<rs_stream, std::shared_ptr<core::file_types::frame> > get_image_samples() override;
            virtual const core::file_types::device_info& get_device_info() override { return m_device_info; }
            virtual std::map<rs_stream, core::file_types::stream_info> get_streams_infos() override {return m_streams_infos; }
            virtual std::map<rs_option, double> get_properties() override { return m_options; }
            virtual std::vector<rs_capabilities> get_capabilities() { return m_capabilities; }
            virtual void set_callbak(std::function<void(std::shared_ptr<core::file_types::sample>)> handler) { m_sample_callback = handler;}
            virtual void set_callbak(std::function<void(bool pause)> handler) { m_stop_callback = handler; }

        protected:
            virtual rs::core::status read_headers(void) = 0;
            virtual void index_next_samples(uint32_t number_of_samples) = 0;
            virtual int32_t size_of_pitches(void) = 0;
            virtual rs::core::status read_image_buffer(std::shared_ptr<rs::core::file_types::frame> &frame);
            void read_thread();
            core::file_types::version query_sdk_version();
            core::file_types::version query_librealsense_version();
            core::status process_image_metadata(std::unique_ptr<core::file_types::sample> &sample, int32_t id, const std::vector<uint8_t>& data);
            core::status get_image_offset(rs_stream stream, int64_t &offset);
            void read_next_sample();
            void update_time_base(uint64_t time);
            void find_nearest_frames(uint32_t sample_index, rs_stream stream);
            bool all_samples_bufferd();
            int64_t calc_sleep_time(std::shared_ptr<core::file_types::sample> sample);

            static const int                                                NUMBER_OF_SAMPLES_TO_INDEX = 8;
            bool                                                            m_projection_changed;
            bool                                                            m_pause;
            bool                                                            m_realtime;
            bool                                                            m_is_indexed;
            uint32_t                                                        m_main_index;
            std::string                                                     m_file_name;
            std::mutex                                                      m_mutex;
            std::thread                                                     m_thread;
            core::compression                                               m_compression;
            std::unique_ptr<core::file>                                     m_file_info;
            std::unique_ptr<core::file>                                     m_file_index;
            std::unique_ptr<core::file>                                     m_file_data;
            std::chrono::high_resolution_clock::time_point                  m_clock_sys;
            uint64_t                                                        m_curr_ts;
            int64_t                                                         m_base_ts;
            core::file_types::sw_info                                       m_sw_info;
            core::file_types::file_header                                   m_file_header;
            core::file_types::device_info                                   m_device_info;
            core::file_types::coordinate_system                             m_coordinate_system;
            std::map<rs_option, double>                                     m_options;
            std::map<rs_stream, core::file_types::stream_info>              m_streams_infos;
            std::map<rs_stream, std::vector<uint32_t>>                      m_image_indexes;
            std::map<rs_stream, std::shared_ptr<core::file_types::frame>>   m_current_image_sample;
            std::map<rs_stream, uint32_t>                                   m_buffered_samples;
            std::queue<std::shared_ptr<core::file_types::sample>>           m_prefetched_samples;
            std::map<core::file_types::chunk_id, std::vector<uint8_t>>      m_unknowns;
            std::vector<std::shared_ptr<core::file_types::sample>>          m_samples;
            std::vector<rs_capabilities>                                    m_capabilities;
            std::function<void(std::shared_ptr<core::file_types::sample>)>  m_sample_callback;
            std::function<void(bool pause)>                                 m_stop_callback;
            std::vector<rs_stream>                                          m_active_streams;
        };
    }
}
