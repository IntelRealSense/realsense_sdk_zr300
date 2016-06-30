// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <vector>
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
#include "file/file.h"

namespace rs
{
    namespace record
    {
        class disk_write
        {
        public:
            disk_write(void);
            ~disk_write(void);
            core::status start();
            void stop();
            void set_pause(bool pause);
            void set_device_info(core::file_types::device_info &info) { m_device_info = info; }
            void set_profiles(std::map<rs_stream, rs::core::file_types::stream_profile> &profiles) { m_stream_profiles = profiles; }
            //report which images and motions streams were captured
            //required since rs_stream doesn't include motion stream info
            void set_capabilities(std::vector<rs_capabilities> capabilities) { m_capabilities = capabilities; }
            void set_properties(const std::vector<rs::core::file_types::device_cap> &properties) { m_options = properties; }
            void set_coordinate_system(rs::core::file_types::coordinate_system coordinate_system) { m_coordinate_system = coordinate_system; }
            void set_file_path(std::string file_path) { m_file_path = file_path; }
            void record_sample(std::vector<std::shared_ptr<rs::core::file_types::sample>> &samples);

        protected:
            void write_thread();
            void write_header();
            void write_device_info();
            void write_sw_info();
            void write_capabilities();
            void write_stream_info();
            void write_properties();
            void write_first_frame_offset();
            void write_stream_num_of_frames();
            //sample type is written separatly since we need to know how to read the sample info
            void write_sample_type(std::shared_ptr<rs::core::file_types::sample> &sample);
            void write_sample(std::shared_ptr<rs::core::file_types::sample> &sample);
            void write_image_data(std::shared_ptr<rs::core::file_types::sample> &sample);

            std::mutex                                                      m_main_mutex; //protect m_samples_queue, m_stop_thred
            std::mutex                                                      m_notify_write_thread_mutex;
            std::condition_variable                                         m_notify_write_thread_cv;
            std::string                                                     m_file_path;
            std::thread                                                     m_thread;
            bool                                                            m_stop_writing;
            std::vector<std::shared_ptr<core::file_types::sample>>          m_samples_queue;
            core::file_types::device_info                                   m_device_info;
            std::map<rs_stream, core::file_types::stream_profile>           m_stream_profiles;
            std::vector<core::file_types::device_cap>                       m_options;
            core::compression                                               m_compression;
            std::unique_ptr<core::file>                                     m_file;
            bool                                                            m_paused;
            core::file_types::coordinate_system                             m_coordinate_system;
            std::vector<rs_capabilities>                                    m_capabilities;
            std::map<rs_stream, int64_t>                                    m_offsets;
            std::map<rs_stream, int32_t>                                    m_number_of_frames;

        };
    }
}
