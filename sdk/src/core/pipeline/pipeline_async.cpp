// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "rs/core/pipeline_async.h"
#include "pipeline_async_impl.h"
#include "rs_sdk_version.h"

using namespace std;
using namespace rs::utils;

namespace rs
{
    namespace core
    {
        pipeline_async::pipeline_async(const mode operation_mode, const char * file_path) :
            m_pimpl(new pipeline_async_impl(operation_mode, file_path))
        {}

        status pipeline_async::add_cv_module(video_module_interface *cv_module)
        {
               return m_pimpl->add_cv_module(cv_module);
        }

        status pipeline_async::query_cv_module(uint32_t index, video_module_interface **cv_module) const
        {
            return m_pimpl->query_cv_module(index, cv_module);
        }

        status pipeline_async::query_default_config(uint32_t index, video_module_interface::supported_module_config &default_config) const
        {
            return m_pimpl->query_default_config(index, default_config);
        }

        status pipeline_async::set_config(const video_module_interface::supported_module_config &config)
        {
            return m_pimpl->set_config(config);
        }

        status pipeline_async::query_current_config(video_module_interface::actual_module_config &current_config) const
        {
            return m_pimpl->query_current_config(current_config);
        }

        status pipeline_async::start(callback_handler * app_callbacks_handler)
        {   
            return m_pimpl->start(app_callbacks_handler);
        }

        status pipeline_async::stop()
        {
            return m_pimpl->stop();
        }

        status pipeline_async::reset()
        {
            return m_pimpl->reset();
        }

        rs::device * pipeline_async::get_device()
        {
            return m_pimpl->get_device();
        }

        pipeline_async::~pipeline_async()
        {
            delete m_pimpl;
        }
    }
}

