// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "streaming_device_manager.h"
#include "sample_set_releaser.h"
#include "rs/utils/librealsense_conversion_utils.h"
#include "rs/utils/log_utils.h"

using namespace std;
using namespace rs::utils;

namespace rs
{
    namespace core
    {
        streaming_device_manager::streaming_device_manager(video_module_interface::actual_module_config &module_config,
                                                           std::function<void(std::shared_ptr<correlated_sample_set> sample_set)> non_blocking_notify_sample,
                                                           rs::device *device) :
            m_non_blocking_notify_sample(non_blocking_notify_sample),
            m_device(device),
            m_active_sources(static_cast<rs::source>(0))
        {
            if(device == nullptr)
            {
                throw std::runtime_error("got invalid device");
            }

            //start with no active sources
            m_active_sources = static_cast<rs::source>(0);

            //configure streams
            vector<stream_type> all_streams;
            for(auto stream_index = 0; stream_index < static_cast<int32_t>(stream_type::max); stream_index++)
            {
                all_streams.push_back(static_cast<stream_type>(stream_index));
            }

            for(auto &stream : all_streams)
            {
                if (!module_config[stream].is_enabled)
                {
                    continue;
                }


                //define callbacks to the actual streams and set them.
                m_stream_callback_per_stream[stream] = [stream, this](rs::frame frame)
                {
                    std::shared_ptr<correlated_sample_set> sample_set(new correlated_sample_set(), sample_set_releaser());
                    (*sample_set)[stream] = image_interface::create_instance_from_librealsense_frame(frame, image_interface::flag::any);
                    if(m_non_blocking_notify_sample)
                    {
                        m_non_blocking_notify_sample(sample_set);
                    };
                };

                m_device->set_frame_callback(convert_stream_type(stream), m_stream_callback_per_stream[stream]);

                m_active_sources = rs::source::video;
            }

            //configure motions
            if (m_device->supports(rs::capabilities::motion_events))
            {
                bool is_motion_required = false;
                for(auto motion_index = 0; motion_index < static_cast<int32_t>(motion_type::max); motion_index++)
                {
                    auto motion = static_cast<motion_type>(motion_index);
                    if (module_config[motion].is_enabled)
                    {
                        is_motion_required = true;
                        break;
                    }
                }

                //single callback registration to motion callbacks.
                if(is_motion_required)
                {
                    //enable motion from the selected module configuration
                    m_motion_callback = [this](rs::motion_data entry)
                    {
                        std::shared_ptr<correlated_sample_set> sample_set(new correlated_sample_set(), sample_set_releaser());

                        auto actual_motion = convert_motion_type(static_cast<rs::event>(entry.timestamp_data.source_id));

                        //check that the motion type is relevant
                        int sdk_motion_type_index = static_cast<int>(actual_motion);
                        if(sdk_motion_type_index == 0 || sdk_motion_type_index >= static_cast<int>(rs::core::motion_type::max))
                        {
                            LOG_ERROR("failed to convert motion type");
                            return;
                        }

                        (*sample_set)[actual_motion].timestamp = entry.timestamp_data.timestamp;
                        (*sample_set)[actual_motion].type = actual_motion;
                        (*sample_set)[actual_motion].frame_number = entry.timestamp_data.frame_number;
                        (*sample_set)[actual_motion].data[0] = entry.axes[0];
                        (*sample_set)[actual_motion].data[1] = entry.axes[1];
                        (*sample_set)[actual_motion].data[2] = entry.axes[2];

                        if(m_non_blocking_notify_sample)
                        {
                            m_non_blocking_notify_sample(sample_set);
                        };
                    };

                    m_device->enable_motion_tracking(m_motion_callback);

                    if(m_active_sources == rs::source::video)
                    {
                        m_active_sources = rs::source::all_sources;
                    }
                    else // none sources
                    {
                        m_active_sources = rs::source::motion_data;
                    }
                }
            }

            m_device->start(m_active_sources);
        }

        streaming_device_manager::~streaming_device_manager()
        {
            try
            {
                m_device->stop(m_active_sources);
            }
            catch(...)
            {
                LOG_ERROR("failed to stop librealsense device");
            }
            m_device = nullptr;
            m_active_sources = static_cast<rs::source>(0);
            m_stream_callback_per_stream.clear();
            m_motion_callback = nullptr;

            m_non_blocking_notify_sample = nullptr;
        }
    }
}

