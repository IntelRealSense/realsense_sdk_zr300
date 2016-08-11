// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <memory>
#include "rs/playback/playback_context.h"
#include "playback_device_impl.h"

namespace rs
{
    namespace playback
    {
        context::context(const char *file_path) : m_init_status(false)
        {
            m_devices = new rs_device*[1];
            m_devices[0] = new rs_device_ex(file_path);
            m_init_status = ((rs_device_ex*)m_devices[0])->init();
        }

        context::~context()
        {
            for(auto i = 0; i < 1; i++)
            {
                if(m_devices[i])
                    delete m_devices[i];
            }
            delete[] m_devices;
        }

        int context::get_device_count() const
        {
            return m_init_status ? 1 : 0;
        }

        rs::device * context::get_device(int index)
        {
            return (rs::device*)get_playback_device();
        }

        device * context::get_playback_device()
        {
            return m_init_status ? (device*)m_devices[0] : nullptr;
        }
    }
}
