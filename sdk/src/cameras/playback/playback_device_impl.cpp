// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#ifdef WIN32
#define NOMINMAX
#endif
#include <algorithm>
#include <type_traits>
#include "playback_device_impl.h"
#include "disk_read_factory.h"
#include "rs/playback/playback_device.h"

using namespace rs::core;

namespace rs
{
    namespace playback
    {
        class frame_callback : public rs_frame_callback
        {
            void(*fptr)(rs_device * dev, rs_frame_ref * frame, void * user);
            void * user;
            rs_device * device;
        public:
            frame_callback() : frame_callback(nullptr, nullptr, nullptr) {}
            frame_callback(rs_device * dev, void(*on_frame)(rs_device *, rs_frame_ref *, void *), void * user) : fptr(on_frame), user(user), device(dev) {}

            operator bool() { return fptr != nullptr; }
            void on_frame (rs_device * device, rs_frame_ref * frame) override
            {
                if (fptr)
                {
                    try { fptr(device, frame, user); }
                    catch (...) {}
                }
            }
            void release() override { delete this; }
        };

        class motion_events_callback : public rs_motion_callback
        {
            void(*fptr)(rs_device * dev, rs_motion_data data, void * user);
            void        * user;
            rs_device   * device;
        public:
            motion_events_callback() : motion_events_callback(nullptr, nullptr, nullptr) {}
            motion_events_callback(rs_device * dev, void(*fptr)(rs_device *, rs_motion_data, void *), void * user) : fptr(fptr), user(user), device(dev) {}

            operator bool() { return fptr != nullptr; }

            void on_event(rs_motion_data data) override
            {
                if (fptr)
                {
                    try { fptr(device, data, user); }
                    catch (...) {}
                }
            }

            void release() override
            {

            }
            ~motion_events_callback()
            {

            }
        };

        class timestamp_events_callback : public rs_timestamp_callback
        {
            void(*fptr)(rs_device * dev, rs_timestamp_data data, void * user);
            void        * user;
            rs_device   * device;
        public:
            timestamp_events_callback() : timestamp_events_callback(nullptr, nullptr, nullptr) {}
            timestamp_events_callback(rs_device * dev, void(*fptr)(rs_device *, rs_timestamp_data, void *), void * user) : fptr(fptr), user(user), device(dev) {}

            operator bool() { return fptr != nullptr; }
            void on_event(rs_timestamp_data data) override
            {
                if (fptr)
                {
                    try { fptr(device, data, user); }
                    catch (...) {}
                }
            }
            void release() override { }
        };

        rs_device_ex::rs_device_ex(const std::string &file_path) :
            m_file_path(file_path),
            m_is_streaming(false),
            m_wait_streams_request(false),
            m_enabled_streams_count(0)
        {

        }

        rs_device_ex::~rs_device_ex()
        {
            stop(rs_source::RS_SOURCE_ALL)            ;
        }

        const rs_stream_interface & rs_device_ex::get_stream_interface(rs_stream stream) const
        {
            if(m_available_streams.find(stream) != m_available_streams.end())
            {
                return *m_available_streams.at(stream).get();
            }
            else
            {
                LOG_ERROR("requsted stream does not exists in the file, stream - " << stream)
                return *m_available_streams.at(rs_stream::RS_STREAM_COUNT).get();
            }
        }

        const char * rs_device_ex::get_name() const
        {
            return get_camera_info(rs_camera_info::RS_CAMERA_INFO_DEVICE_NAME);
        }

        const char * rs_device_ex::get_serial() const
        {
            return get_camera_info(rs_camera_info::RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER);
        }

        const char * rs_device_ex::get_firmware_version() const
        {
             return get_camera_info(rs_camera_info::RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION);
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
            LOG_INFO("enable stream - " << stream << " ,width - " << width << " ,height - " << height << " ,format - " << format << " ,fps -" << fps);

            auto streams_infos = m_disk_read->get_streams_infos();
            if(streams_infos.find(stream) == streams_infos.end())
            {
                LOG_ERROR("unsupported stream");
                throw std::runtime_error("unsupported stream");
            }
            if(!m_disk_read->is_stream_profile_available(stream, width, height, format, fps))
            {
                LOG_ERROR("configuration mode is unavailable");
                std::stringstream ss;
                ss << "configuration mode of " << width << "X" << height << "X" <<  fps << " is unavailable";
                throw std::runtime_error(ss.str());
            }
            if(!m_available_streams[stream]->is_enabled())
            {
                m_available_streams[stream]->set_is_enabled(true);
                m_disk_read->enable_stream(stream, true);
                m_enabled_streams_count++;
            }
        }

        void rs_device_ex::enable_stream_preset(rs_stream stream, rs_preset preset)//enables the single available configuration
        {
            LOG_INFO("enable stream - " << stream << " ,preset - " << preset)
            auto streams_infos = m_disk_read->get_streams_infos();
            if(streams_infos.find(stream) != streams_infos.end())
            {
                if(!m_available_streams[stream]->is_enabled())
                {
                    m_available_streams[stream]->set_is_enabled(true);
                    m_disk_read->enable_stream(stream, true);
                    m_enabled_streams_count++;
                }
            }
            else
            {
                LOG_ERROR("unsupported stream");
                throw std::runtime_error("unsupported stream");
            }
        }

        void rs_device_ex::disable_stream(rs_stream stream)
        {
            LOG_INFO("disable stream - " << stream)
            auto streams_infos = m_disk_read->get_streams_infos();
            if(streams_infos.find(stream) != streams_infos.end())
            {
                if(m_available_streams[stream]->is_enabled())
                {
                    m_available_streams[stream]->set_is_enabled(false);
                    m_disk_read->enable_stream(stream, false);
                    m_enabled_streams_count--;
                }
            }
        }

        void rs_device_ex::enable_motion_tracking()
        {
            LOG_INFO("enable motion tracking")
            m_disk_read->enable_motions_callback(true);
        }

        void rs_device_ex::set_stream_callback(rs_stream stream, void(*on_frame)(rs_device * device, rs_frame_ref * frame, void * user), void * user)
        {
            rs_device_ex::set_stream_callback(stream, new frame_callback(this, on_frame, user));
        }

        void rs_device_ex::set_stream_callback(rs_stream stream, rs_frame_callback * callback)
        {
            LOG_INFO("stream - " << stream)
            m_frame_thread[stream].callback = std::shared_ptr<rs_frame_callback>(callback, [](rs_frame_callback* cb)
            {cb->release();});
        }

        void rs_device_ex::disable_motion_tracking()
        {
            LOG_INFO("disable motion tracking")
            m_disk_read->enable_motions_callback(false);
        }

        void rs_device_ex::set_motion_callback(void(*on_event)(rs_device * device, rs_motion_data data, void * user), void * user)
        {
            set_motion_callback(new motion_events_callback(this, on_event, user));
        }

        void rs_device_ex::set_motion_callback(rs_motion_callback * callback)
        {
            LOG_INFO("set motion callback")
            m_imu_thread.motion_callback = std::shared_ptr<rs_motion_callback>(callback, [](rs_motion_callback* cb)
            { cb->release(); });
        }

        void rs_device_ex::set_timestamp_callback(void(*on_event)(rs_device * device, rs_timestamp_data data, void * user), void * user)
        {
            set_timestamp_callback(new timestamp_events_callback(this, on_event, user));
        }

        void rs_device_ex::set_timestamp_callback(rs_timestamp_callback * callback)
        {
            LOG_INFO("set time stamp callback")
            m_imu_thread.time_stamp_callback = std::shared_ptr<rs_timestamp_callback>(callback, [](rs_timestamp_callback* cb)
            { cb->release(); });
        }

        void rs_device_ex::start(rs_source source)
        {
            LOG_INFO("start");
            set_enabled_streams();
            resume();
        }

        void rs_device_ex::stop(rs_source source)
        {
            LOG_INFO("stop");
            if(m_disk_read == nullptr)
                return;
            m_enabled_streams_count = 0;
            pause();
            m_disk_read->reset();
            if(!wait_for_active_frames())
                throw std::runtime_error("failed to stop playback device, not all frames returned within the time limit");
        }

        bool rs_device_ex::is_capturing() const
        {
            return m_is_streaming;
        }

        int rs_device_ex::is_motion_tracking_active() const
        {
            return (int)(m_imu_thread.motion_callback && m_disk_read->is_motion_tracking_enabled());
        }

        void rs_device_ex::wait_all_streams()
        {
            LOG_FUNC_SCOPE();

            if(m_frame_thread.size() > 0)//frame callbacks are enabled
            {
                pause();
                throw std::runtime_error("calling to \"wait_for_frames\" (synchronous mode) is not allowed if \"set_frame_callback\" was called (asynchronous mode)");
            }

            if(m_disk_read->query_capture_mode() == capture_mode::asynced)
                throw std::runtime_error("this file was not recorded in synced mode (wait for frames). the file can be played only in asynced mode (frame callbacks)");

            {
                std::lock_guard<std::mutex> guard(m_mutex);
                if(m_wait_streams_request)
                {
                    LOG_ERROR("read flag was set to true by another thread - no reentrance");
                    return;
                }
                m_wait_streams_request = true;
            }

            std::unique_lock<std::mutex> guard(m_all_stream_available_mutex);
            if(m_is_streaming)
            {
                m_all_stream_available_cv.wait(guard);
                guard.unlock();
            }
        }

        bool rs_device_ex::poll_all_streams()
        {
            LOG_FUNC_SCOPE();

            if(m_frame_thread.size() > 0)//frame callbacks are enabled
            {
                pause();
                throw std::runtime_error("calling to \"poll_for_frames\" (synchronous mode) is not allowed if \"set_frame_callback\" was called (asynchronous mode)");
            }

            if(m_disk_read->query_capture_mode() == capture_mode::asynced)
                throw std::runtime_error("this file was not recorded in synced mode (wait for frames). the file can be played only in asynced mode (frame callbacks)");

            std::lock_guard<std::mutex> guard(m_mutex);

            if(all_streams_available())
            {
                for(auto it = m_curr_frames.begin(); it != m_curr_frames.end(); ++it)
                {
                    m_available_streams[it->first]->set_frame(it->second);
                }
                return m_is_streaming;
            }
            return false;
        }

        bool rs_device_ex::supports(rs_capabilities capability) const
        {
            auto caps = m_disk_read->get_capabilities();
            return find(caps.begin(), caps.end(), capability) != caps.end();
        }

        bool rs_device_ex::supports(rs_camera_info info_param) const
        {
            const std::map<rs_camera_info, std::string>& camera_info = m_disk_read->get_camera_info();
            return camera_info.find(info_param) != camera_info.end();
        }

        bool rs_device_ex::supports_option(rs_option option) const
        {
            std::map<rs_option, double> props = m_disk_read->get_properties();
            return props.find(option) != props.end();
        }

        void rs_device_ex::get_option_range(rs_option option, double & min, double & max, double & step, double & def)
        {
            //return the current value as range
            auto properties = m_disk_read->get_properties();
            if(properties.find(option) != properties.end())
            {
                min = properties[option];
                max = properties[option];
                step = 0;
                def = properties[option];
            }
        }

        void rs_device_ex::set_options(const rs_option options[], size_t count, const double values[])
        {
            for(uint32_t i = 0; i < count; i++)
            {
                rs_option option = options[i];
                double value = values[i];
                switch(option)
                {
                    case rs_option::RS_OPTION_TOTAL_FRAME_DROPS: m_disk_read->set_total_frame_drop_count(value); break;
                    default: break;
                }
            }
        }

        void rs_device_ex::get_options(const rs_option options[], size_t count, double values[])
        {
            std::map<rs_option, double> props = m_disk_read->get_properties();
            for(size_t i = 0; i < count; i++)
            {
                if(props.find(options[i]) != props.end())
                    values[i] = props.at(options[i]);
            }
        }

        void rs_device_ex::release_frame(rs_frame_ref * ref)
        {
            LOG_VERBOSE("release frame");
            auto stream_type = ref->get_stream_type();
            std::lock_guard<std::mutex> guard(m_frame_thread[stream_type].mutex);
            delete ref;
            m_frame_thread[stream_type].active_samples_count--;
            m_frame_thread[stream_type].sample_deleted_cv.notify_one();
        }

        rs_frame_ref * rs_device_ex::clone_frame(rs_frame_ref * frame)
        {
            //this method is obsolete
            throw std::runtime_error("not implemented");
        }

        const char * rs_device_ex::get_usb_port_id() const
        {
            return "Disk";
        }

        const char * rs_device_ex::get_camera_info(rs_camera_info info_type) const
        {
            const std::map<rs_camera_info, std::string>& camera_info = m_disk_read->get_camera_info();
            try{
                return camera_info.at(info_type).c_str();
            }catch(const std::out_of_range& e)
            {
                std::ostringstream oss;
                oss << "camera info " << info_type << "is not supported for this device";
                throw std::runtime_error(oss.str());
            }
        }

        rs_motion_intrinsics rs_device_ex::get_motion_intrinsics() const
        {
            return m_disk_read->get_motion_intrinsics();
        }

        rs_extrinsics rs_device_ex::get_motion_extrinsics_from(rs_stream from) const
        {
            rs_extrinsics empty = {0};
            auto infos = m_disk_read->get_streams_infos();
            if(infos.find(from) != infos.end())
            {
                //check that motion extrinsics are not equal to an array of zeros
                if(memcmp(&infos[from].profile.motion_extrinsics, &empty, sizeof(empty)) != 0)
                    return infos[from].profile.motion_extrinsics;
            }
            throw std::runtime_error("No motion extrinsics available");
        }

        void rs_device_ex::start_fw_logger(char fw_log_op_code, int grab_rate_in_ms, std::timed_mutex &mutex)
        {
            //not available!!!
        }

        void rs_device_ex::stop_fw_logger()
        {
            //not available!!!
        }

        const char * rs_device_ex::get_option_description(rs_option option) const
        {
            return nullptr;
        }

        bool rs_device_ex::is_real_time()
        {
            return m_disk_read->query_realtime();
        }

        void rs_device_ex::end_of_file()
        {
            m_is_streaming = false;
            signal_all();
            join_callbacks_threads();
        }

        void rs_device_ex::internal_pause()
        {
            m_is_streaming = false;
            m_disk_read->pause();
            signal_all();
            join_callbacks_threads();
        }

        //if pause is called during wait for frame, the wait will return imideatly, there is no guarantee which data is available
        void rs_device_ex::pause()
        {
            LOG_INFO("pause");
            std::lock_guard<std::mutex> guard(m_pause_resume_mutex);
            internal_pause();
        }

        void rs_device_ex::resume()
        {
            LOG_INFO("resume");
            std::lock_guard<std::mutex> guard(m_pause_resume_mutex);
            internal_pause();
            m_is_streaming = true;
            start_callbacks_threads();
            m_disk_read->resume();
        }

        bool rs_device_ex::set_frame_by_index(int index, rs_stream stream)
        {
            auto frames = m_disk_read->set_frame_by_index(index, stream);
            for(auto it = frames.begin(); it != frames.end(); ++it)
            {
                if(!m_available_streams[it->first]->is_enabled())
                {
                    LOG_ERROR("stream is not enabled");
                    throw std::runtime_error("stream is not enabled");
                }
                m_available_streams[it->first]->set_frame(it->second);
            }
            return !frames.empty();
        }

        bool rs_device_ex::set_frame_by_timestamp(uint64_t timestamp)
        {
            LOG_FUNC_SCOPE();
            auto frames = m_disk_read->set_frame_by_time_stamp(timestamp);
            for(auto it = frames.begin(); it != frames.end(); ++it)
            {
                if(!m_available_streams[it->first]->is_enabled())
                {
                    LOG_ERROR("stream is not enabled");
                    throw std::runtime_error("stream is not enabled");
                }
                m_available_streams[it->first]->set_frame(it->second);
            }
            return !frames.empty();
        }

        void rs_device_ex::set_real_time(bool realtime)
        {
            m_disk_read->set_realtime(realtime);
        }

        int rs_device_ex::get_frame_index(rs_stream stream)
        {
            auto frame = m_available_streams[stream]->get_frame();
            if(!frame)
            {
                LOG_ERROR("frame is null")
                return 0;
            }
            LOG_VERBOSE("frame number - " << frame->finfo.index_in_stream)
            return frame->finfo.index_in_stream;
        }

        int rs_device_ex::get_frame_count(rs_stream stream)
        {
            return m_disk_read->query_number_of_frames(stream);
        }

        int rs_device_ex::get_frame_count()
        {
            uint32_t nframes = std::numeric_limits<uint32_t>::max();
            for(auto it = m_available_streams.begin(); it != m_available_streams.end(); ++it)
            {
                auto stream = it->first;
                uint32_t nof = get_frame_count(stream);
                if (nof < nframes && nof > 0)
                    nframes = nof;
            }
            return nframes;
        }

        playback::file_info rs_device_ex::get_file_info()
        {
            return m_disk_read->query_file_info();
        }

        void rs_device_ex::handle_frame_callback(std::shared_ptr<file_types::sample> sample)
        {
            if(!sample)
                throw std::runtime_error("null sample");
            auto frame = std::dynamic_pointer_cast<file_types::frame_sample>(sample);
            if(!frame)
                throw std::runtime_error("null frame");
            auto stream = frame->finfo.stream;
            {
                std::lock_guard<std::mutex> guard(m_mutex);
                m_curr_frames[stream] = frame;
            }
            if(m_frame_thread.size() > 0)//async
            {
                if(m_frame_thread.find(stream) == m_frame_thread.end()) return;
                if(m_disk_read->query_realtime())
                {
                    std::unique_lock<std::mutex> guard(m_frame_thread[stream].mutex);
                    if(m_frame_thread[stream].sample != nullptr)
                        m_disk_read->update_frame_drop_count(stream, 1);
                    m_frame_thread[stream].sample = frame;
                    guard.unlock();
                    m_frame_thread[stream].sample_ready_cv.notify_one();
                }
                else//asynced reader non realtime mode
                {
                    m_frame_thread[stream].active_samples_count++;
                    m_frame_thread[stream].callback->on_frame(this, new file_types::rs_frame_ref_impl(m_curr_frames[stream]));
                }
            }
            else
            {
                if(!m_disk_read->query_realtime())//synced reader non realtime mode
                {
                    while(!m_wait_streams_request && m_is_streaming)
                        std::this_thread::sleep_for (std::chrono::milliseconds(5));//TODO:[mk] replace with cv
                }
                if(m_wait_streams_request)
                {
                    std::lock_guard<std::mutex> guard(m_mutex);

                    if(all_streams_available())
                    {
                        for(auto it = m_curr_frames.begin(); it != m_curr_frames.end(); ++it)
                        {
                            m_available_streams[it->first]->set_frame(it->second);
                        }
                        //signal to "wait_for_frame" to end wait.
                        std::lock_guard<std::mutex> guard(m_all_stream_available_mutex);
                        m_all_stream_available_cv.notify_one();
                        m_wait_streams_request = false;
                        m_curr_frames.clear();
                        LOG_VERBOSE("all streams are available");
                    }
                }

                if(m_enabled_streams_count == m_curr_frames.size())
                {
                    m_disk_read->update_frame_drop_count(stream, 1);
                }
            }
        }

        void rs_device_ex::frame_callback_thread(rs_stream stream)
        {
            auto pred = [this, stream]()->bool{ return (m_frame_thread[stream].sample != nullptr) || (m_is_streaming == false);};

            while(m_is_streaming)
            {
                std::unique_lock<std::mutex> guard(m_frame_thread[stream].mutex);
                m_frame_thread[stream].sample_ready_cv.wait(guard, pred);
                file_types::rs_frame_ref_impl * frame_ref = nullptr;
                if(m_is_streaming)
                {
                    frame_ref = new file_types::rs_frame_ref_impl(m_frame_thread[stream].sample);
                    m_frame_thread[stream].active_samples_count++;
                    m_frame_thread[stream].sample = nullptr;
                }
                guard.unlock();
                if(frame_ref)
                {
                    m_frame_thread[stream].callback->on_frame(this, frame_ref);
                }
            }
        }

        void rs_device_ex::handle_motion_callback(std::shared_ptr<file_types::sample> sample)
        {
            if(m_disk_read->query_realtime())
            {
                std::unique_lock<std::mutex> guard(m_imu_thread.mutex);
                if(m_imu_thread.samples.size() >= m_imu_thread.max_queue_size)
                {
                    m_imu_thread.samples.pop();
                    m_disk_read->update_imu_drop_count(1);
                }
                m_imu_thread.samples.push(sample);
                guard.unlock();
                m_imu_thread.sample_ready_cv.notify_one();
            }
            else
            {
                m_imu_thread.push_sample_to_user(sample);
            }
        }

        void rs_device_ex::motion_callback_thread()
        {
            auto pred = [this]()->bool{ return (m_imu_thread.samples.empty() == false) || (m_is_streaming == false);};

            while(m_is_streaming || !m_imu_thread.samples.empty())
            {
                std::unique_lock<std::mutex> guard(m_imu_thread.mutex);
                m_imu_thread.sample_ready_cv.wait(guard, pred);
                std::queue<std::shared_ptr<core::file_types::sample>> data;
                std::swap(m_imu_thread.samples, data);
                guard.unlock();
                while (!data.empty())
                {
                    m_imu_thread.push_sample_to_user(data.front());
                    data.pop();
                }
            }
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
                m_available_streams[it->first] = std::unique_ptr<rs_stream_impl>(new rs_stream_impl(streams_infos[it->first]));
            }
            for(auto it = m_available_streams.begin(); it != m_available_streams.end(); ++it)
            {
                m_available_streams[it->first]->create_extrinsics(m_available_streams);
            }
            file_types::stream_info si;
            memset(&si, 0, sizeof(file_types::stream_info));
            m_available_streams[rs_stream::RS_STREAM_COUNT] = std::unique_ptr<rs_stream_impl>(new rs_stream_impl(si));

            m_disk_read->set_callback((std::function<void()>)([this]() { end_of_file(); }));

            std::function<void(std::shared_ptr<core::file_types::sample>)> cb_func = [this](std::shared_ptr<file_types::sample> sample)
            {
                switch(sample->info.type)
                {
                    case file_types::sample_type::st_image:         handle_frame_callback(sample); break;
                    case file_types::sample_type::st_motion:        handle_motion_callback(sample); break;
                    case file_types::sample_type::st_time:          handle_motion_callback(sample); break;
                    case file_types::sample_type::st_debug_event:   break;
                }
            };

            m_disk_read->set_callback(cb_func);

            return true;
        }

        bool rs_device_ex::all_streams_available()
        {
            if(m_curr_frames.size() != m_enabled_streams_count) return false;
            for(auto it1 = m_curr_frames.begin(); it1 != m_curr_frames.end(); ++it1)
            {
                if(!it1->second)return false;
                auto capture_time_1 = it1->second->info.capture_time;
                for(auto it2 = m_curr_frames.begin(); it2 != m_curr_frames.end(); ++it2)
                {
                    if(!it2->second)return false;
                    auto capture_time_2 = it2->second->info.capture_time;
                    if(capture_time_1 != capture_time_2)
                    {
                        LOG_VERBOSE("frame drop, first capture time = " << capture_time_1 << " second capture time = " << capture_time_2);
                        return false;
                    }
                }
            }
            return true;
        }

        void rs_device_ex::set_enabled_streams()
        {
            if(m_enabled_streams_count > 0) return;
            for(auto it = m_available_streams.begin(); it != m_available_streams.end(); ++it)
            {
                if(it->first == rs_stream::RS_STREAM_COUNT) continue;
                if(it->second->is_enabled())
                {
                    auto is_async = m_frame_thread.size() > 0;
                    auto frame_callback_exist = m_frame_thread.find(it->first) != m_frame_thread.end();
                    if(!is_async || (is_async && frame_callback_exist))
                    {
                        m_disk_read->enable_stream(it->first, true);
                        m_enabled_streams_count++;
                    }
                }
                else
                    m_disk_read->enable_stream(it->first, false);

            }
        }

        void rs_device_ex::signal_all()
        {
            for(auto it = m_frame_thread.begin(); it != m_frame_thread.end(); ++it)
            {
                std::lock_guard<std::mutex> guard(it->second.mutex);
                it->second.sample_ready_cv.notify_one();
            }
            {
                std::lock_guard<std::mutex> guard(m_imu_thread.mutex);
                m_imu_thread.sample_ready_cv.notify_one();
            }
            std::unique_lock<std::mutex> guard(m_all_stream_available_mutex);
            m_all_stream_available_cv.notify_one();
            guard.unlock();
        }

        void rs_device_ex::start_callbacks_threads()
        {
            LOG_FUNC_SCOPE();
            for(auto it = m_frame_thread.begin(); it != m_frame_thread.end(); ++it)
            {
                it->second.active_samples_count = 0;
                it->second.thread = std::thread(&rs_device_ex::frame_callback_thread, this, it->first);
            }
            if(m_disk_read->is_motion_tracking_enabled())
            {
                m_imu_thread.thread = std::thread(&rs_device_ex::motion_callback_thread, this);
                m_imu_thread.max_queue_size = LIBREALSENSE_IMU_BUFFER_SIZE; // librealsense motion buffer size, there is no requirment for the buffers size to match.
            }
        }

        bool rs_device_ex::wait_for_active_frames()
        {
            for(auto it = m_frame_thread.begin(); it != m_frame_thread.end(); ++it)
            {
                //wait for all frames to return
                auto pred = [it]() -> bool
                {
                    return it->second.active_samples_count == 0;
                };

                std::unique_lock<std::mutex> locker(it->second.mutex);
                auto all_frames_back = it->second.sample_deleted_cv.wait_for(locker, std::chrono::seconds(5), pred);
                if(!all_frames_back)
                    return false;
                locker.unlock();
            }
            return true;
        }

        void rs_device_ex::join_callbacks_threads()
        {
            LOG_FUNC_SCOPE();
            for(auto it = m_frame_thread.begin(); it != m_frame_thread.end(); ++it)
            {
                if(it->second.thread.joinable())
                    it->second.thread.join();
            }
            if(m_disk_read && m_disk_read->is_motion_tracking_enabled())
            {
                if(m_imu_thread.thread.joinable())
                    m_imu_thread.thread.join();
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

        file_info device::get_file_info()
        {
            return ((rs_device_ex*)this)->get_file_info();
        }
    }
}
