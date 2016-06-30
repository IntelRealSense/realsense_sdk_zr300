// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <algorithm>
#include <limits>
#include "playback_device_impl.h"
#include "disk_read_factory.h"
#include "rs/playback/playback_device.h"

using namespace rs::core;

namespace rs
{
    namespace playback
    {
        rs_device_ex::rs_device_ex(const std::string &file_path) :
            m_file_path(file_path),
            m_is_streaming(false),
            m_is_motion_tracking (false),
            m_wait_streams_request(false)
        {

        }

        rs_device_ex::~rs_device_ex()
        {
            m_enabled_callbacks.clear();
            join_callbacks_threads();
        }

        const rs_stream_interface & rs_device_ex::get_stream_interface(rs_stream stream) const
        {
            if(m_availeble_streams.find(stream) != m_availeble_streams.end())
            {
                return *m_availeble_streams.at(stream).get();
            }
            else
            {
                LOG_ERROR("requsted stream does not exists in the file, stream - " << stream)
                return *m_empty_stream.get();
            }
        }

        const char * rs_device_ex::get_name() const
        {
            auto& dev = m_disk_read->get_device_info();
            return dev.name;
        }

        const char * rs_device_ex::get_serial() const
        {
            auto str = std::string(m_disk_read->get_device_info().serial);
            return str.c_str();
        }

        const char * rs_device_ex::get_firmware_version() const
        {
            return m_disk_read->get_device_info().firmware;
        }

        float rs_device_ex::get_depth_scale() const
        {
            auto streams_infos = m_disk_read->get_streams_infos();
            if(streams_infos.find(rs_stream::RS_STREAM_DEPTH) != streams_infos.end())
                return streams_infos.at(rs_stream::RS_STREAM_DEPTH).profile.depth_scale;
            return 0;
        }

        void rs_device_ex::enable_stream(rs_stream stream, int width, int height, rs_format format, int fps, rs_output_buffer_format output)
        {
            LOG_INFO("enable stream - " << stream << " ,width - " << width << " ,height - " << height << " ,format - " << format << " ,fps -" << fps)
            if(!m_disk_read->is_stream_available(stream, width, height, format, fps))
            {
                LOG_ERROR("configuration mode is unavailable")
                throw "configuration mode is unavailable";
            }
            m_availeble_streams[stream]->set_is_enabled(true);
            m_disk_read->enable_stream(stream, true);
        }

        void rs_device_ex::enable_stream_preset(rs_stream stream, rs_preset preset)
        {
            LOG_INFO("enable stream - " << stream << " ,preset - " << preset)
            auto streams_infos = m_disk_read->get_streams_infos();
            if(streams_infos.find(stream) != streams_infos.end())
            {
                m_availeble_streams[stream]->set_is_enabled(true);
                m_disk_read->enable_stream(stream, true);
            }
            else
            {
                LOG_ERROR("configuration mode is unavailable")
                throw "configuration mode is unavailable";
            }
        }

        void rs_device_ex::disable_stream(rs_stream stream)
        {
            LOG_INFO("disable stream - " << stream)
            auto streams_infos = m_disk_read->get_streams_infos();
            if(streams_infos.find(stream) != streams_infos.end())
                m_availeble_streams[stream]->set_is_enabled(false);
            m_disk_read->enable_stream(stream, false);
        }

        void rs_device_ex::enable_motion_tracking()
        {
            LOG_INFO("enable motion tracking")
            m_is_motion_tracking = true;
        }

        void rs_device_ex::set_stream_callback(rs_stream stream, void(*on_frame)(rs_device * device, rs_frame_ref * frame, void * user), void * user)
        {
            rs_device_ex::set_stream_callback(stream, new core::frame_callback(this, on_frame, user));
        }

        void rs_device_ex::set_stream_callback(rs_stream stream, rs_frame_callback * callback)
        {
            LOG_INFO("stream - " << stream)
            m_frame_thread[stream].m_callback = std::shared_ptr<rs_frame_callback>(callback, [](rs_frame_callback* cb)
            {cb->release();});
            m_enabled_callbacks[file_types::sample_type::st_image] = true;
        }

        void rs_device_ex::disable_motion_tracking()
        {
            LOG_INFO("disable motion tracking")
            m_is_motion_tracking = false;
        }

        void rs_device_ex::set_motion_callback(void(*on_event)(rs_device * device, rs_motion_data data, void * user), void * user)
        {
            set_motion_callback(new motion_events_callback(this, on_event, user));
        }

        void rs_device_ex::set_motion_callback(rs_motion_callback * callback)
        {
            LOG_INFO("set motion callback")
            m_motion_thread.m_callback = std::shared_ptr<rs_motion_callback>(callback, [](rs_motion_callback* cb)
            { cb->release(); });
            m_enabled_callbacks[file_types::sample_type::st_motion] = true;
        }

        void rs_device_ex::set_timestamp_callback(void(*on_event)(rs_device * device, rs_timestamp_data data, void * user), void * user)
        {
            set_timestamp_callback(new timestamp_events_callback(this, on_event, user));
        }

        void rs_device_ex::set_timestamp_callback(rs_timestamp_callback * callback)
        {
            LOG_INFO("set time stamp callback")
            m_time_stamp_thread.m_callback = std::shared_ptr<rs_timestamp_callback>(callback, [](rs_timestamp_callback* cb)
            { cb->release(); });
            m_enabled_callbacks[file_types::sample_type::st_time] = true;
        }

        void rs_device_ex::start(rs_source source)
        {
            LOG_INFO("start");
            m_max_fps = get_active_streams_max_fps();
            stop(source);
            m_disk_read->start();
        }

        void rs_device_ex::stop(rs_source source)
        {
            LOG_INFO("stop");
            set_frame_by_timestamp(0);
            if(m_is_streaming)
            {
                std::unique_lock<std::mutex> guard(m_all_stream_availeble_mutex);
                m_all_stream_availeble_cv.wait(guard);
                guard.unlock();
            }
        }

        bool rs_device_ex::is_capturing() const
        {
            return m_is_streaming;
        }

        int rs_device_ex::is_motion_tracking_active() const
        {
            return (int)(m_enabled_callbacks.at(file_types::sample_type::st_motion) && m_is_motion_tracking);
        }

        void rs_device_ex::wait_all_streams()
        {
            LOG_VERBOSE("wait all streams request");
            clear_streams_data();

            {
                std::lock_guard<std::mutex> guard(m_mutex);
                if(m_wait_streams_request)
                {
                    LOG_ERROR("read flag was set to true by another thread - no reentrance");
                    return;
                }
                m_wait_streams_request = true;
            }

            std::unique_lock<std::mutex> guard(m_all_stream_availeble_mutex);
            m_all_stream_availeble_cv.wait(guard);
            guard.unlock();
            m_wait_streams_request = false;
        }

        bool rs_device_ex::poll_all_streams()
        {
            std::lock_guard<std::mutex> guard(m_mutex);

            LOG_VERBOSE("poll all streams request")
            clear_streams_data();
            for(auto it = m_availeble_streams.begin(); it != m_availeble_streams.end(); ++it)
            {
                if(!it->second->is_enabled())continue;
                //check if there's an available new frame for this stream in the device back buffer
                if(m_curr_frames.find(it->first) == m_curr_frames.end())continue;
                it->second->set_frame(m_curr_frames[it->first]);
            }
            return m_is_streaming && all_streams_availeble();
        }

        rs_frameset * rs_device_ex::wait_all_streams_safe()
        {
            LOG_VERBOSE("wait all streams safe request")
            wait_all_streams();
            return create_frameset();
        }

        bool rs_device_ex::poll_all_streams_safe(rs_frameset ** frames)
        {
            LOG_VERBOSE("poll all streams safe request")
            if(poll_all_streams())
            {
                *frames = create_frameset();
                return true;
            }
            return false;
        }

        void rs_device_ex::release_frames(rs_frameset * frameset)
        {
            LOG_VERBOSE("release frames")
            delete frameset;
        }

        rs_frameset * rs_device_ex::clone_frames(rs_frameset * frameset)
        {
            LOG_VERBOSE("clone frames")
            frameset_impl* f = static_cast<frameset_impl*>(frameset);
            return f->clone();
        }

        bool rs_device_ex::supports(rs_capabilities capability) const
        {
            auto caps = m_disk_read->get_capabilities();
            return find(caps.begin(), caps.end(), capability) != caps.end();
        }

        bool rs_device_ex::supports_option(rs_option option) const
        {
            std::map<rs_option, double> props = m_disk_read->get_properties();
            return props.find(option) != props.end();
        }

        void rs_device_ex::get_option_range(rs_option option, double & min, double & max, double & step, double & def)
        {
            auto properties = m_disk_read->get_properties();
            if(properties.find(option) != properties.end())
            {
                min = properties[option];
                max = properties[option];
                step = 0;
                def = properties[option];
            }
        }

        void rs_device_ex::set_options(const rs_option options[], int count, const double values[])
        {
            //not availeble!!!
        }

        void rs_device_ex::get_options(const rs_option options[], int count, double values[])
        {
            std::map<rs_option, double> props = m_disk_read->get_properties();
            for(int i = 0; i < count; i++)
            {
                if(props.find(options[i]) != props.end())
                    values[i] = props.at(options[i]);
            }
        }

        rs_frame_ref * rs_device_ex::detach_frame(const rs_frameset * fs, rs_stream stream)
        {
            LOG_VERBOSE("detach frame")
            return ((frameset_impl*)fs)->detach(stream);
        }

        void rs_device_ex::release_frame(rs_frame_ref * ref)
        {
            LOG_VERBOSE("release frame")
            delete ref;
        }

        rs_frame_ref * rs_device_ex::clone_frame(rs_frame_ref * frame)
        {
            LOG_VERBOSE("clone frame")
            rs_frame_ref_impl * f =  static_cast<rs_frame_ref_impl*>(frame);
            return new rs_frame_ref_impl(f->get_frame());
        }

        const char * rs_device_ex::get_usb_port_id() const
        {
            return m_disk_read->get_device_info().usb_port_id;
        }

        bool rs_device_ex::is_real_time()
        {
            return m_disk_read->query_realtime();
        }

        void rs_device_ex::pause()
        {
            LOG_INFO("pause")
            m_disk_read->set_pause(true);
            if(m_is_streaming)
            {
                std::unique_lock<std::mutex> guard(m_all_stream_availeble_mutex);
                m_all_stream_availeble_cv.wait(guard);
                guard.unlock();
            }
        }

        void rs_device_ex::resume()
        {
            LOG_INFO("resume")
            m_disk_read->start();
        }

        bool rs_device_ex::set_frame_by_index(int index, rs_stream stream)
        {
            auto sts = m_disk_read->set_frame_by_index(index, stream);
            auto frames = m_disk_read->get_image_samples();
            for(auto it = frames.begin(); it != frames.end(); ++it)
            {
                if(!m_availeble_streams[it->first]->is_enabled())continue;
                m_availeble_streams[it->first]->set_frame(it->second);
            }
            return sts;
        }

        bool rs_device_ex::set_frame_by_timestamp(uint64_t timestamp)
        {
            auto sts = m_disk_read->set_frame_by_time_stamp(timestamp);
            auto frames = m_disk_read->get_image_samples();
            for(auto it = frames.begin(); it != frames.end(); ++it)
            {
                if(!m_availeble_streams[it->first]->is_enabled())continue;
                m_availeble_streams[it->first]->set_frame(it->second);
            }
            return sts;
        }

        void rs_device_ex::set_real_time(bool realtime)
        {
            m_disk_read->set_realtime(realtime);
        }

        int rs_device_ex::get_frame_index(rs_stream stream)
        {
            auto frame = m_availeble_streams[stream]->get_frame();
            if(!frame)
            {
                LOG_ERROR("frame is null")
                return 0;
            }
            LOG_VERBOSE("frame number - " << frame->info.index_in_stream)
            return frame->info.index_in_stream;
        }

        int rs_device_ex::get_frame_count(rs_stream stream)
        {
            return m_disk_read->query_number_of_frames(stream);
        }

        int rs_device_ex::get_frame_count()
        {
            uint32_t nframes = std::numeric_limits<uint32_t>::max();
            for(auto it = m_availeble_streams.begin(); it != m_availeble_streams.end(); ++it)
            {
                auto stream = it->first;
                uint32_t nof = get_frame_count(stream);
                if (nof < nframes && nof > 0)
                    nframes = nof;
            }
            return nframes;
        }

        bool rs_device_ex::init()
        {
            if(disk_read_factory::create_disk_read(m_file_path.c_str(), m_disk_read) != status::status_no_error)
            {
                return false;
            }
            auto streams_infos = m_disk_read->get_streams_infos();
            for(auto it = streams_infos.begin(); it != streams_infos.end(); ++it)
            {
                m_availeble_streams[it->first] = std::unique_ptr<rs_stream_impl>(new rs_stream_impl(streams_infos[it->first]));
            }
            m_enabled_callbacks[file_types::sample_type::st_time] = false;
            m_enabled_callbacks[file_types::sample_type::st_motion] = false;
            m_enabled_callbacks[file_types::sample_type::st_image] = false;

            file_types::stream_info si;
            memset(&si, 0, sizeof(file_types::stream_info));
            m_empty_stream = std::unique_ptr<rs_stream_impl>(new rs_stream_impl(si));
            m_disk_read->set_callbak([this](bool pause)
            {
                m_is_streaming = !pause;
                if(pause)
                {
                    for(auto it = m_frame_thread.begin(); it != m_frame_thread.end(); ++it)
                    {
                        std::lock_guard<std::mutex> guard(it->second.m_mutex);
                        it->second.m_cv.notify_one();
                    }
                    {
                        std::lock_guard<std::mutex> guard(m_motion_thread.m_mutex);
                        m_motion_thread.m_cv.notify_one();
                    }
                    {
                        std::lock_guard<std::mutex> guard(m_time_stamp_thread.m_mutex);
                        m_time_stamp_thread.m_cv.notify_one();
                    }
                    join_callbacks_threads();
                    std::unique_lock<std::mutex> guard(m_all_stream_availeble_mutex);
                    m_all_stream_availeble_cv.notify_one();
                    guard.unlock();
                }
                else
                    start_callbacks_threads();
            });
            m_disk_read->set_callbak([this](std::shared_ptr<file_types::sample> sample)
            {
                switch(sample->type)
                {
                    case file_types::sample_type::st_motion:
                    {
                        if(!m_enabled_callbacks[sample->type])break;
                        auto motion = std::dynamic_pointer_cast<file_types::motion>(sample);
                        m_motion_thread.m_sample = motion;
                        if(m_disk_read->query_realtime())
                        {
                            std::lock_guard<std::mutex> guard(m_motion_thread.m_mutex);
                            m_motion_thread.m_cv.notify_one();
                        }
                        else
                        {
                            m_motion_thread.m_callback->on_event(motion->info.data);
                        }
                        break;
                    }
                    case file_types::sample_type::st_time:
                    {
                        if(!m_enabled_callbacks[sample->type])break;
                        auto time_stamp = std::dynamic_pointer_cast<file_types::time_stamp>(sample);
                        m_time_stamp_thread.m_sample = time_stamp;
                        if(m_disk_read->query_realtime())
                        {
                            std::lock_guard<std::mutex> guard(m_time_stamp_thread.m_mutex);
                            m_time_stamp_thread.m_cv.notify_one();
                        }
                        else
                        {
                            m_time_stamp_thread.m_callback->on_event(time_stamp->info.data);
                        }
                        break;
                    }
                    case file_types::sample_type::st_image:
                    {
                        std::lock_guard<std::mutex> guard(m_mutex);

                        auto frame = std::dynamic_pointer_cast<file_types::frame>(sample);
                        auto stream = frame->info.stream;
                        m_curr_frames[stream] = frame;
                        if(m_enabled_callbacks[sample->type])
                        {
                            if(m_disk_read->query_realtime())
                            {
                                m_frame_thread[stream].m_sample = frame;
                                std::lock_guard<std::mutex> guard(m_frame_thread[stream].m_mutex);
                                m_frame_thread[stream].m_cv.notify_one();
                            }
                            else
                            {
                                m_frame_thread[stream].m_callback->on_frame(this, new rs_frame_ref_impl(m_curr_frames[stream]));
                            }
                        }
                        else
                        {
                            if(m_wait_streams_request)
                            {
                                if(!m_availeble_streams[stream]->is_enabled())break;
                                m_availeble_streams[stream]->set_frame(frame);
                                if(all_streams_availeble())
                                {
                                    //signal to "wait_for_frame" to end wait.
                                    std::lock_guard<std::mutex> guard(m_all_stream_availeble_mutex);
                                    m_all_stream_availeble_cv.notify_one();
                                }
                                LOG_VERBOSE((m_wait_streams_request ? "not " : "") << "all streams are availeble")
                            }
                        }
                        break;
                    }
                }
            });

            return true;
        }

        bool rs_device_ex::all_streams_availeble()
        {
            double max_excepted_time_diff = 1000.0 / (double)m_max_fps / 2.0;
            for(auto it1 = m_availeble_streams.begin(); it1 != m_availeble_streams.end(); ++it1)
            {
                if(!it1->second->is_enabled())continue;
                if(!it1->second->has_data())
                {
                    LOG_VERBOSE("frame drop - no data");
                    return false;
                }
                auto ts1 = it1->second->get_frame()->info.sync;
                for(auto it2 = m_availeble_streams.begin(); it2 != m_availeble_streams.end(); ++it2)
                {
                    if(!it2->second->is_enabled())continue;
                    if(!it2->second->has_data())
                    {
                        LOG_VERBOSE("frame drop - no data");
                        return false;
                    }
                    auto ts2 = it2->second->get_frame()->info.sync;
                    if(abs(ts2 - ts1) > max_excepted_time_diff)
                    {
                        LOG_VERBOSE("frame drop, first ts = " << ts1 << " second ts = " << ts2);
                        return false;
                    }
                }
            }
            return true;
        }

        void rs_device_ex::clear_streams_data()
        {
            for(auto it = m_availeble_streams.begin(); it != m_availeble_streams.end(); ++it)
            {
                it->second->clear_data();
            }
        }

        frameset_impl* rs_device_ex::create_frameset()
        {
            std::map<rs_stream, std::shared_ptr<rs_frame_ref>> frames;
            for(auto it = m_availeble_streams.begin(); it != m_availeble_streams.end(); ++it)
            {
                if(!it->second->is_enabled())continue;
                frames[it->first] = std::make_shared<rs_frame_ref_impl>(it->second->get_frame());
            }
            return new frameset_impl(frames);
        }

        uint32_t rs_device_ex::get_active_streams_max_fps()
        {
            uint32_t rv = 0;
            for(auto it = m_availeble_streams.begin(); it != m_availeble_streams.end(); ++it)
            {
                if(it->second->is_enabled() && (uint32_t)it->second->get_framerate() > rv)
                    rv = it->second->get_framerate();
            }
            return rv;
        }

        void rs_device_ex::start_callbacks_threads()
        {
            LOG_FUNC_SCOPE();
            for(auto it = m_frame_thread.begin(); it != m_frame_thread.end(); ++it)
            {
                m_frame_thread[it->first].m_thread = std::thread(&rs_device_ex::frame_callback_thread, this, it->first);
            }
            if(m_is_motion_tracking)
            {
                m_motion_thread.m_thread = std::thread(&rs_device_ex::motion_callback_thread, this);
                m_time_stamp_thread.m_thread = std::thread(&rs_device_ex::time_stamp_callback_thread, this);
            }
        }

        void rs_device_ex::join_callbacks_threads()
        {
            LOG_FUNC_SCOPE();
            for(auto it = m_frame_thread.begin(); it != m_frame_thread.end(); ++it)
            {
                if(it->second.m_thread.joinable())
                    it->second.m_thread.join();
            }
            if(m_is_motion_tracking)
            {
                if(m_motion_thread.m_thread.joinable())
                    m_motion_thread.m_thread.join();
                if(m_time_stamp_thread.m_thread.joinable())
                    m_time_stamp_thread.m_thread.join();
            }
        }

        void rs_device_ex::frame_callback_thread(rs_stream stream)
        {
            while(true)
            {
                std::unique_lock<std::mutex> guard(m_frame_thread[stream].m_mutex);
                m_frame_thread[stream].m_cv.wait(guard);
                guard.unlock();

                if(m_is_streaming)
                    m_frame_thread[stream].m_callback->on_frame(this, new rs_frame_ref_impl(m_frame_thread[stream].m_sample));
                else
                    break;
            }
        }

        void rs_device_ex::motion_callback_thread()
        {
            while(true)
            {
                std::unique_lock<std::mutex> guard(m_motion_thread.m_mutex);
                m_motion_thread.m_cv.wait(guard);
                if(m_is_streaming)
                    m_motion_thread.m_callback->on_event(m_motion_thread.m_sample->info.data);
                else
                    break;
                guard.unlock();
            }
        }

        void rs_device_ex::time_stamp_callback_thread()
        {
            while(true)
            {
                std::unique_lock<std::mutex> guard(m_time_stamp_thread.m_mutex);
                m_time_stamp_thread.m_cv.wait(guard);
                if(m_is_streaming)
                    m_time_stamp_thread.m_callback->on_event(m_time_stamp_thread.m_sample->info.data);
                else
                    break;
                guard.unlock();
            }
        }

        /************************************************************************************************************/
        //rs::device extention
        /************************************************************************************************************/

        bool device::is_real_time()
        {
            return ((rs_device_ex*)this)->is_real_time();
        }

        void device::pause()
        {
            ((rs_device_ex*)this)->pause();
        }

        void device::resume()
        {
            ((rs_device_ex*)this)->resume();
        }

        bool device::set_frame_by_index(int index, rs::stream stream)
        {
            return((rs_device_ex*)this)->set_frame_by_index(index, (rs_stream)stream);
        }

        bool device::set_frame_by_timestamp(uint64_t timestamp)
        {
            return ((rs_device_ex*)this)->set_frame_by_timestamp(timestamp);
        }

        void device::set_real_time(bool realtime)
        {
            ((rs_device_ex*)this)->set_real_time(realtime);
        }

        int device::get_frame_index(rs::stream stream)
        {
            return ((rs_device_ex*)this)->get_frame_index((rs_stream)stream);
        }

        int device::get_frame_count(rs::stream stream)
        {
            return ((rs_device_ex*)this)->get_frame_count((rs_stream)stream);
        }

        int device::get_frame_count()
        {
            return ((rs_device_ex*)this)->get_frame_count();
        }
    }
}
