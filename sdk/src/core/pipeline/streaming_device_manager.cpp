// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "streaming_device_manager.h"
#include "rs/utils/librealsense_conversion_utils.h"
#include "rs/utils/log_utils.h"

using namespace std;
using namespace rs::utils;

namespace rs
{
    namespace core
    {
        streaming_device_manager::streaming_device_manager(video_module_interface::actual_module_config & complete_module_config,
                                                           std::function<void(std::shared_ptr<correlated_sample_set> sample_set)> non_blocking_set_sample,
                                                           const std::unique_ptr<context_interface> & context) :
            m_non_blocking_set_sample(non_blocking_set_sample),
            m_device(nullptr),
            m_active_sources(static_cast<rs::source>(0))
        {
            //start librealsense device
            //TODO : get the device defined in modules_config.
            m_device = context->get_device(0); //device memory managed by the context

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
                if (!complete_module_config[stream].is_enabled)
                {
                    continue;
                }

                bool is_matching_stream_mode_found = false;
                //enable the streams
                rs::stream librealsense_stream = convert_stream_type(stream);
                auto stream_mode_count = m_device->get_stream_mode_count(librealsense_stream);
                for(auto stream_mode_index = 0; stream_mode_index < stream_mode_count; stream_mode_index++)
                {
                    int width, height, frame_rate;
                    rs::format format;
                    m_device->get_stream_mode(librealsense_stream, stream_mode_index, width, height, format, frame_rate);
                    bool is_acceptable_stream_mode = (width == complete_module_config[stream].size.width &&
                                                      height == complete_module_config[stream].size.height &&
                                                      frame_rate == complete_module_config[stream].frame_rate);
                    if(is_acceptable_stream_mode)
                    {
                        m_device->enable_stream(librealsense_stream, width, height, format, frame_rate);
                        is_matching_stream_mode_found = true;
                        break;
                    }
                }

                if(!is_matching_stream_mode_found)
                {
                    LOG_WARN("stream index " << static_cast<int32_t>(stream) << "was requested, but no valid configuration mode was found");
                    continue;
                }

                //define callbacks to the actual streams and set them.
                m_stream_callback_per_stream[stream] = [stream, this](rs::frame frame)
                {
                    std::shared_ptr<correlated_sample_set> sample_set(new correlated_sample_set(), sample_set_releaser());
                    (*sample_set)[stream] = image_interface::create_instance_from_librealsense_frame(frame, image_interface::flag::any, nullptr);
                    m_non_blocking_set_sample(sample_set);
                };

                m_device->set_frame_callback(librealsense_stream, m_stream_callback_per_stream[stream]);

                m_active_sources = rs::source::video;
            }

            //configure motions
            if (m_device->supports(rs::capabilities::motion_events))
            {
                vector<motion_type> all_motions;
                for(auto stream_index = 0; stream_index < static_cast<int32_t>(motion_type::max); stream_index++)
                {
                    all_motions.push_back(static_cast<motion_type>(stream_index));
                }

                vector<motion_type> actual_motions;
                for(auto &motion: all_motions)
                {
                    if (complete_module_config[motion].is_enabled)
                    {
                        actual_motions.push_back(motion);
                    }
                }

                //single callback registration to motion callbacks.
                if(actual_motions.size() > 0)
                {
                    //enable motion from the selected module configuration
                    m_motion_callback = [actual_motions, this](rs::motion_data entry)
                    {
                        std::shared_ptr<correlated_sample_set> sample_set(new correlated_sample_set(), sample_set_releaser());
                        for(auto actual_motion : actual_motions)
                        {
                            auto & actual_motion_sample = (*sample_set)[actual_motion];
                            actual_motion_sample.timestamp = entry.timestamp_data.timestamp;
                            actual_motion_sample.type = actual_motion;
                            actual_motion_sample.frame_number = entry.timestamp_data.frame_number;
                            actual_motion_sample.data[0] = entry.axes[0];
                            actual_motion_sample.data[1] = entry.axes[1];
                            actual_motion_sample.data[2] = entry.axes[2];
                        }

                        m_non_blocking_set_sample(sample_set);
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

            m_non_blocking_set_sample = nullptr;
        }
    }
}

