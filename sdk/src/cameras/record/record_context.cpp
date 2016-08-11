// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "rs/record/record_context.h"
#include "include/record_device_impl.h"

namespace rs
{
    namespace record
    {
        context::context(const char *file_path)
        {
            m_devices = new rs_device*[get_device_count()];
            for(auto i = 0; i < get_device_count(); i++)
            {
                m_devices[i] = new rs_device_ex(file_path, (rs_device*)(m_context.get_device(i)));//revert casting to cpp wrapper done by librealsense
            }
        }

        context::~context()
        {
            for(auto i = 0; i < get_device_count(); i++)
            {
                if(m_devices[i])
                    delete m_devices[i];
            }
            delete[] m_devices;
        }

        rs::device * context::get_device(int index)
        {
            return (rs::device*)get_record_device(index);
        }

        device * context::get_record_device(int index)
        {
            return (device*)m_devices[index];
        }
    }
}

