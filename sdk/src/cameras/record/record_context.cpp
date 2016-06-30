// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "rs/record/record_context.h"
#include "include/record_device_impl.h"

namespace rs
{
    namespace record
    {
        context::context(const std::string& file_path)
        {
            for(auto i = 0; i < get_device_count(); i++)
            {
                rs_error * e = nullptr;
                auto r = rs_get_device(handle, i, &e);
                rs::error::handle(e);
                m_devices.push_back(std::unique_ptr<rs_device>(new rs_device_ex(file_path, r)));
            }
        }

        context::~context()
        {

        }

        rs::device * context::get_device(int index)
        {
            return (rs::device*)get_record_device(index);
        }

        device * context::get_record_device(int index)
        {
            return (device*)m_devices[index].get();
        }
    }
}

