// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <vector>
#include <queue>
#include <map>
#include <set>
#include <list>
#include <mutex>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include "compression/compression_mock.h"
#include "include/file_types.h"
#include "rs/core/image_interface.h"
#include "include/file.h"
#include "rs/core/types.h"

namespace rs
{
    namespace record
    {
        struct configuration
        {
            std::string                                                     m_file_path;
            core::file_types::device_info                                   m_device_info;
            std::vector<core::file_types::device_cap>                       m_options;
            std::map<rs_stream, core::file_types::stream_profile>           m_stream_profiles;
            core::file_types::coordinate_system                             m_coordinate_system;
            std::vector<rs_capabilities>                                    m_capabilities;
            rs_motion_intrinsics                                            m_motion_intrinsics;
            playback::capture_mode                                          m_capture_mode;
        };

        class disk_write
        {
        public:
            disk_write(void);
            ~disk_write(void);
            bool start();
            void stop();
            void set_pause(bool pause);
            bool is_configured() {return m_is_configured;}
            core::status configure(const configuration &config);
            void record_sample(std::shared_ptr<core::file_types::sample> &sample);

        private:
            void write_thread();
            void write_header(uint8_t stream_count, core::file_types::coordinate_system cs, playback::capture_mode capture_mode);
            void write_device_info(core::file_types::device_info info);
            void write_sw_info();
            //report which images and motions streams were captured
            //required since rs_stream doesn't include motion stream info
            void write_capabilities(std::vector<rs_capabilities> capabilities);
            void write_stream_info(std::map<rs_stream, core::file_types::stream_profile> profiles);
            void write_motion_intrinsics(const rs_motion_intrinsics &motion_intrinsics);
            void write_properties(const std::vector<core::file_types::device_cap> &properties);
            void write_first_frame_offset();
            void write_stream_num_of_frames();
            //sample type is written separatly since we need to know how to read the sample info
            void write_sample_info(std::shared_ptr<rs::core::file_types::sample> &sample);
            void write_sample(std::shared_ptr<rs::core::file_types::sample> &sample);
            void write_image_data(std::shared_ptr<rs::core::file_types::sample> &sample);

            bool allow_sample(std::shared_ptr<rs::core::file_types::sample> &sample);
            uint32_t get_min_fps(const std::map<rs_stream, core::file_types::stream_profile>& stream_profiles);

            std::mutex                                                      m_main_mutex; //protect m_samples_queue, m_stop_thred
            std::mutex                                                      m_notify_write_thread_mutex;
            std::condition_variable                                         m_notify_write_thread_cv;
            std::thread                                                     m_thread;
            bool                                                            m_stop_writing;
            std::queue<std::shared_ptr<core::file_types::sample>>           m_samples_queue;
            core::compression                                               m_compression;
            std::unique_ptr<core::file>                                     m_file;
            bool                                                            m_paused;
            std::map<rs_stream, int64_t>                                    m_offsets;
            std::map<rs_stream, int32_t>                                    m_number_of_frames;
            bool                                                            m_is_configured;
            std::map<rs_stream, uint32_t>                                   m_samples_count;
            uint32_t                                                        m_min_fps;
        };
    }
}
