// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "rs/core/pipeline_async.h"
#include "pipeline_async_impl.h"

using namespace std;
using namespace rs::utils;

namespace rs
{
    namespace core
    {
        pipeline_async::pipeline_async(const char * playback_file_path) :
            m_pimpl(new pipeline_async_impl(playback_file_path))
        {}

        status pipeline_async::add_cv_module(video_module_interface *cv_module)
        {
               return m_pimpl->add_cv_module(cv_module);
        }

        status pipeline_async::query_cv_module(uint32_t index, video_module_interface **cv_module)
        {
            return m_pimpl->query_cv_module(index, cv_module);
        }

        status pipeline_async::query_available_config(uint32_t index, pipeline_config &available_config)
        {
            return m_pimpl->query_available_config(index, available_config);
        }

        status pipeline_async::set_config(const pipeline_config &config)
        {
            return m_pimpl->set_config(config);
        }

        status pipeline_async::query_current_config(pipeline_config &current_pipeline_config)
        {
            return m_pimpl->query_current_config(current_pipeline_config);
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

        pipeline_async::~pipeline_async()
        {
            delete m_pimpl;
        }
    }
}

