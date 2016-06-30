// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <memory>
#include "rs/playback/playback_context.h"
#include "playback_device_impl.h"

namespace rs
{
    namespace playback
    {
        context::context(const std::string& file_path) : m_init_status(false)
        {
            m_device = std::unique_ptr<rs_device>(new rs_device_ex(file_path));
            m_init_status = ((rs_device_ex*)m_device.get())->init();
        }

        context::~context()
        {

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
            return m_init_status ? (device*)m_device.get() : nullptr;
        }
    }
}
