// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <memory>
#include "include/file.h"
#include "disk_read.h"
#include "linux/v1/disk_read.h"
#include "windows/v10/disk_read.h"
#include "rs/utils/log_utils.h"

namespace rs
{
    namespace playback
    {
        class disk_read_factory
        {

        public:
            static rs::core::status create_disk_read(const char *file_name, std::unique_ptr<disk_read_interface> &disk_read)
            {
                std::unique_ptr<rs::core::file> file_ = std::unique_ptr<rs::core::file>(new rs::core::file());

                rs::core::status status = file_->open(file_name, rs::core::open_file_option::read);
                if (status != rs::core::status_no_error)
                {
                    std::string str = file_name;
                    throw std::runtime_error("failed to open file for playback, file path - " + str);
                }

                /* Get the file header */
                status = file_->set_position(0, rs::core::move_method::begin);
                if (status != rs::core::status_no_error) return status;

                uint32_t nbytesRead = 0;
                int32_t file_type_id;
                status = file_->read_bytes(&file_type_id, sizeof(file_type_id), nbytesRead);
                if (status != rs::core::status_no_error) return status;

                if (file_type_id == UID('R', 'S', 'L', '2'))
                {
                    LOG_INFO("create disk read for Linux file format version 2")
                    disk_read = std::unique_ptr<disk_read_interface>(new playback::disk_read(file_name));
                    return disk_read->init();
                }

                if (file_type_id == UID('R', 'S', 'L', '1'))
                {
                    LOG_INFO("create disk read for Linux file format version 1")
                    disk_read = std::unique_ptr<disk_read_interface>(new linux::v1::disk_read(file_name));
                    return disk_read->init();
                }                

                if (file_type_id == UID('R', 'S', 'C', 'F'))
                {
                    LOG_INFO("create disk read for Windows file format")
                    disk_read = std::unique_ptr<disk_read_interface>(new windows::v10::disk_read(file_name));
                    return disk_read->init();
                }
                LOG_ERROR("failed to create disk read")
                return rs::core::status_file_read_failed;
            }
        };
    }
}
