// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "rs/utils/log_utils.h"
#include "device_streaming_guard.h"

using namespace std;
using namespace rs::utils;

namespace rs
{
    namespace core
    {
        device_streaming_guard::device_streaming_guard(rs::device *device,
                                                       rs::source enabled_sources) :
            m_device(device),
            m_enabled_sources(enabled_sources)
        {
            if(!m_device)
            {
                throw std::runtime_error("got invalid device");
            }

            m_device->start(m_enabled_sources);
        }

        device_streaming_guard::~device_streaming_guard()
        {
            try
            {
                m_device->stop(m_enabled_sources);
            }
            catch(...)
            {
                LOG_ERROR("failed to stop librealsense device");
            }
            m_device = nullptr;
            m_enabled_sources = static_cast<rs::source>(0);
        }
    }
}

