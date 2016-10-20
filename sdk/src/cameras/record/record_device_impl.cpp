// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <algorithm>
#include "record_device_impl.h"
#include "image/image_utils.h"
#include "rs/record/record_device.h"
#include "rs/utils/log_utils.h"

using namespace rs::core;

namespace
{
    static rs_capabilities get_capability(rs_stream stream)
    {
        switch(stream)
        {
            case rs_stream::RS_STREAM_COLOR: return rs_capabilities::RS_CAPABILITIES_COLOR;
            case rs_stream::RS_STREAM_DEPTH: return rs_capabilities::RS_CAPABILITIES_DEPTH;
            case rs_stream::RS_STREAM_INFRARED: return rs_capabilities::RS_CAPABILITIES_INFRARED;
            case rs_stream::RS_STREAM_INFRARED2: return rs_capabilities::RS_CAPABILITIES_INFRARED2;
            case rs_stream::RS_STREAM_FISHEYE: return rs_capabilities::RS_CAPABILITIES_FISH_EYE;
            default: return rs_capabilities::RS_CAPABILITIES_COUNT;
        }
    }
}

namespace rs
{
    namespace record
    {
        class frame_callback : public rs_frame_callback
        {
        public:
            frame_callback(rs_stream stream, void * user, void(*on_frame)(rs_device *, rs_frame_ref *, void *), rs_device_ex * dev) :
                m_stream(stream), m_user_callback(nullptr), m_user_callback_ptr(on_frame), m_user(user), m_device(dev) {}
            frame_callback(rs_stream stream, rs_frame_callback * user_callback, rs_device_ex * device) :
                m_stream(stream), m_user_callback(user_callback), m_device(device), m_user_callback_ptr(nullptr) {}
            void on_frame (rs_device * device, rs_frame_ref * frame) override
            {
                auto clone = device->clone_frame(frame);
                m_device->write_frame(m_stream, clone);
                m_user_callback_ptr == nullptr ? m_user_callback->on_frame(m_device, frame) : m_user_callback_ptr(device, frame, m_user);
            }
            void release() override
            {
                if(m_user_callback)
                    m_user_callback->release();
                delete this;
            }
        private:
            void(*m_user_callback_ptr)(rs_device * dev, rs_frame_ref * frame, void * m_user);
            void * m_user;
            rs_stream m_stream;
            rs_frame_callback * m_user_callback;
            rs_device_ex * m_device;
        };

        class motion_events_callback : public rs_motion_callback
        {
        public:
            motion_events_callback(void * user, void(*on_event)(rs_device *, rs_motion_data, void *), rs_device_ex * dev) :
                m_user_callback(nullptr), m_user_callback_ptr(on_event), m_user(user), m_device(dev) {}
            motion_events_callback(rs_motion_callback * user_callback, rs_device_ex * device) :
                m_user_callback(user_callback), m_device(device), m_user_callback_ptr(nullptr) {}
            void on_event (rs_motion_data data) override
            {
                std::shared_ptr<file_types::sample> sample = std::shared_ptr<file_types::sample>(
                new file_types::motion_sample(data, m_device->get_capture_time()),[](file_types::sample* s) {});
                m_device->m_disk_write.record_sample(sample);
                m_user_callback_ptr == nullptr ? m_user_callback->on_event(data) : m_user_callback_ptr(m_device, data, m_user);
            }
            void release() override
            {
                if(m_user_callback)
                    m_user_callback->release();
                delete this;
            }
        private:
            void(*m_user_callback_ptr)(rs_device * dev, rs_motion_data data, void * user);
            void * m_user;
            rs_motion_callback * m_user_callback;
            rs_device_ex * m_device;
        };

        class timestamp_events_callback : public rs_timestamp_callback
        {
        public:
            timestamp_events_callback(void * user, void(*on_event)(rs_device *, rs_timestamp_data, void *), rs_device_ex * dev) :
                m_user_callback(nullptr), m_user_callback_ptr(on_event), m_user(user), m_device(dev) {}
            timestamp_events_callback(rs_timestamp_callback * user_callback, rs_device_ex * device) :
                m_user_callback(user_callback), m_device(device), m_user_callback_ptr(nullptr) {}
            void on_event (rs_timestamp_data data) override
            {
                std::shared_ptr<file_types::sample> sample  = std::shared_ptr<file_types::sample>(
                new file_types::time_stamp_sample(data, m_device->get_capture_time()),[](file_types::sample* s) {});
                m_device->m_disk_write.record_sample(sample);
                m_user_callback_ptr == nullptr ? m_user_callback->on_event(data) : m_user_callback_ptr(m_device, data, m_user);
            }
            void release() override
            {
                if(m_user_callback)
                    m_user_callback->release();
                delete this;
            }
        private:
            void(*m_user_callback_ptr)(rs_device * dev, rs_timestamp_data data, void * user);
            void * m_user;
            rs_timestamp_callback * m_user_callback;
            rs_device_ex * m_device;
        };

        rs_device_ex::rs_device_ex(const std::string &file_path, rs_device *device) :
            m_device(device),
            m_file_path(file_path),
            m_is_streaming(false),
            m_capture_mode(playback::capture_mode::synced)
        {

        }

        rs_device_ex::~rs_device_ex()
        {
            m_disk_write.stop();
            stop(m_source);
        }

        const rs_stream_interface & rs_device_ex::get_stream_interface(rs_stream stream) const
        {
            LOG_FUNC_SCOPE();
            return m_device->get_stream_interface(stream);
        }

        const char * rs_device_ex::get_name() const
        {
            LOG_FUNC_SCOPE();
            return m_device->get_name();
        }

        const char * rs_device_ex::get_serial() const
        {
            LOG_FUNC_SCOPE();
            return m_device->get_serial();
        }

        const char * rs_device_ex::get_firmware_version() const
        {
            LOG_FUNC_SCOPE();
            return m_device->get_firmware_version();
        }

        float rs_device_ex::get_depth_scale() const
        {
            LOG_FUNC_SCOPE();
            return m_device->get_depth_scale();
        }

        void rs_device_ex::enable_stream(rs_stream stream, int width, int height, rs_format format, int fps, rs_output_buffer_format output)
        {
            LOG_FUNC_SCOPE();
            LOG_INFO("enable stream - " << stream << " ,width - " << width << " ,height - " << height << " ,format - " << format << " ,fps -" << fps);
            m_device->enable_stream(stream, width, height, format, fps, output);
            update_active_streams(stream, true);
        }

        void rs_device_ex::enable_stream_preset(rs_stream stream, rs_preset preset)
        {
            LOG_FUNC_SCOPE();
            LOG_INFO("enable stream - " << stream << " ,preset - " << preset);
            m_device->enable_stream_preset(stream, preset);
            update_active_streams(stream, true);
        }

        void rs_device_ex::disable_stream(rs_stream stream)
        {
            LOG_FUNC_SCOPE();
            LOG_INFO("disable stream - " << stream);
            m_device->disable_stream(stream);
            update_active_streams(stream, false);
        }

        void rs_device_ex::enable_motion_tracking()
        {
            LOG_FUNC_SCOPE();
            LOG_INFO("enable motion tracking");
            m_device->enable_motion_tracking();
        }

        void rs_device_ex::set_stream_callback(rs_stream stream, void(*on_frame)(rs_device * device, rs_frame_ref * frame, void * user), void * user)
        {
            LOG_FUNC_SCOPE();
            LOG_INFO("stream - " << stream);
            auto rec_callback = new frame_callback(stream, user, on_frame, this);
            m_device->set_stream_callback(stream, rec_callback);
            m_capture_mode = playback::capture_mode::asynced;
        }

        void rs_device_ex::set_stream_callback(rs_stream stream, rs_frame_callback * callback)
        {
            LOG_FUNC_SCOPE();
            LOG_INFO("stream - " << stream);
            auto rec_callback = new frame_callback(stream, callback, this);
            m_device->set_stream_callback(stream, rec_callback);
            m_capture_mode = playback::capture_mode::asynced;
        }

        void rs_device_ex::disable_motion_tracking()
        {
            LOG_FUNC_SCOPE();
            LOG_INFO("disable motion tracking");
            m_device->disable_motion_tracking();
            m_is_motion_tracking_enabled = false;
        }

        void rs_device_ex::set_motion_callback(void(*on_event)(rs_device * device, rs_motion_data data, void * user), void * user)
        {
            LOG_FUNC_SCOPE();
            LOG_INFO("set motion callback");
            auto rec_callback = new motion_events_callback(user, on_event, this);
            m_device->set_motion_callback(rec_callback);
            if(supports(rs_capabilities::RS_CAPABILITIES_MOTION_EVENTS))
                m_is_motion_tracking_enabled = true;
        }

        void rs_device_ex::set_motion_callback(rs_motion_callback * callback)
        {
            LOG_FUNC_SCOPE();
            LOG_INFO("set motion callback");
            auto rec_callback = new motion_events_callback(callback, this);
            m_device->set_motion_callback(rec_callback);
            if(supports(rs_capabilities::RS_CAPABILITIES_MOTION_EVENTS))
                m_is_motion_tracking_enabled = true;
        }

        void rs_device_ex::set_timestamp_callback(void(*on_event)(rs_device * device, rs_timestamp_data data, void * user), void * user)
        {
            LOG_FUNC_SCOPE();
            LOG_INFO("set motion callback");
            auto rec_callback = new timestamp_events_callback(user, on_event, this);
            m_device->set_timestamp_callback(rec_callback);
            if(supports(rs_capabilities::RS_CAPABILITIES_MOTION_EVENTS))
                m_is_motion_tracking_enabled = true;
        }

        void rs_device_ex::set_timestamp_callback(rs_timestamp_callback * callback)
        {
            LOG_FUNC_SCOPE();
            LOG_INFO("set time stamp callback");
            auto rec_callback = new timestamp_events_callback(callback, this);
            m_device->set_timestamp_callback(rec_callback);
            if(supports(rs_capabilities::RS_CAPABILITIES_MOTION_EVENTS))
                m_is_motion_tracking_enabled = true;
        }

        void rs_device_ex::start(rs_source source)
        {
            LOG_FUNC_SCOPE();
            LOG_INFO("start");
            m_source = source;
            if(m_disk_write.is_configured())
            {
                resume_record();
            }
            else
            {
                status sts = configure_disk_write();
                if (sts == status::status_no_error)
                {
                    m_capture_time_base = std::chrono::high_resolution_clock::now();
                    m_disk_write.start();
                }
            }
            m_device->start(source);
            std::lock_guard<std::mutex> guard(m_is_streaming_mutex);
            m_is_streaming = true;
        }

        void rs_device_ex::stop(rs_source source)
        {
            LOG_FUNC_SCOPE();
            std::lock_guard<std::mutex> guard(m_is_streaming_mutex);
            if(!m_is_streaming) return;
            LOG_INFO("stop");
            m_device->stop(source);
            pause_record();
            m_is_streaming = false;
        }

        bool rs_device_ex::is_capturing() const
        {
            LOG_FUNC_SCOPE();
            return m_device->is_capturing();
        }

        int rs_device_ex::is_motion_tracking_active() const
        {
            LOG_FUNC_SCOPE();
            return m_device->is_motion_tracking_active();
        }

        void rs_device_ex::wait_all_streams()
        {
            LOG_FUNC_SCOPE();
            m_device->wait_all_streams();
            write_samples();
        }

        bool rs_device_ex::poll_all_streams()
        {
            LOG_FUNC_SCOPE();
            auto rv = m_device->poll_all_streams();
            if(rv)
            {
                write_samples();
            }
            return rv;
        }

        bool rs_device_ex::supports(rs_capabilities capability) const
        {
            LOG_FUNC_SCOPE();
            return m_device->supports(capability);
        }

        bool rs_device_ex::supports(rs_camera_info info_param) const
        {
            LOG_FUNC_SCOPE();
            return m_device->supports(info_param);
        }

        bool rs_device_ex::supports_option(rs_option option) const
        {
            LOG_FUNC_SCOPE();
            return m_device->supports_option(option);
        }

        void rs_device_ex::get_option_range(rs_option option, double & min, double & max, double & step, double & def)
        {
            LOG_FUNC_SCOPE();
            m_device->get_option_range(option, min, max, step, def);
        }

        void rs_device_ex::set_options(const rs_option options[], size_t count, const double values[])
        {
            LOG_FUNC_SCOPE();
            LOG_INFO("set options, options count - " << count)
            m_device->set_options(options, count, values);
            if(!m_device->is_capturing()) return;
            for(size_t i = 0; i < count; i++)
            {
                file_types::device_cap dc = {options[i], values[i]};
                m_modifyied_options.push_back(dc);
            }
        }

        void rs_device_ex::get_options(const rs_option options[], size_t count, double values[])
        {
            LOG_FUNC_SCOPE();
            m_device->get_options(options, count, values);
        }

        void rs_device_ex::release_frame(rs_frame_ref * ref)
        {
            LOG_FUNC_SCOPE();
            m_device->release_frame(ref);
        }

        rs_frame_ref * rs_device_ex::clone_frame(rs_frame_ref * frame)
        {
            LOG_FUNC_SCOPE();
            return m_device->clone_frame(frame);
        }

        const char * rs_device_ex::get_usb_port_id() const
        {
            LOG_FUNC_SCOPE();
            return m_device->get_usb_port_id();
        }

        const char * rs_device_ex::get_camera_info(rs_camera_info type) const
        {
            LOG_FUNC_SCOPE();
            return m_device->get_camera_info(type);
        }

        rs_motion_intrinsics rs_device_ex::get_motion_intrinsics() const
        {
            LOG_FUNC_SCOPE();
            //save empty calibration data in case motion calibration data is not valid
            try
            {
                return m_device->get_motion_intrinsics();
            }
            catch(...)
            {
                LOG_ERROR("failed to read motion intrinsics");
                rs_motion_intrinsics rv = {};
                return rv;
            }
        }

        rs_extrinsics rs_device_ex::get_motion_extrinsics_from(rs_stream from) const
        {
            LOG_FUNC_SCOPE();
            return m_device->get_motion_extrinsics_from(from);
        }

        void rs_device_ex::start_fw_logger(char fw_log_op_code, int grab_rate_in_ms, std::timed_mutex &mutex)
        {
            m_device->start_fw_logger(fw_log_op_code, grab_rate_in_ms, mutex);
        }

        void rs_device_ex::stop_fw_logger()
        {
            m_device->stop_fw_logger();
        }

        const char * rs_device_ex::get_option_description(rs_option option) const
        {
            return m_device->get_option_description(option);
        }

        void rs_device_ex::pause_record()
        {
            LOG_INFO("pause record")
            m_disk_write.set_pause(true);
        }

        void rs_device_ex::resume_record()
        {
            LOG_INFO("resume record")
            m_disk_write.set_pause(false);
        }

        core::file_types::device_info rs_device_ex::get_device_info()
        {
            LOG_FUNC_SCOPE();
            core::file_types::device_info info = {};
            try
            {
                strcpy(info.name, get_camera_info(rs_camera_info::RS_CAMERA_INFO_DEVICE_NAME));
                strcpy(info.serial, get_camera_info(rs_camera_info::RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER));
                strcpy(info.camera_firmware, get_camera_info(rs_camera_info::RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION));
                if(m_device->supports(rs_capabilities::RS_CAPABILITIES_MOTION_EVENTS))
                    strcpy(info.motion_module_firmware, get_camera_info(rs_camera_info::RS_CAMERA_INFO_MOTION_MODULE_FIRMWARE_VERSION));
                if(m_device->supports(rs_capabilities::RS_CAPABILITIES_ADAPTER_BOARD))
                    strcpy(info.adapter_board_firmware, get_camera_info(rs_camera_info::RS_CAMERA_INFO_ADAPTER_BOARD_FIRMWARE_VERSION));
                strcpy(info.usb_port_id, get_usb_port_id());
            }
            catch(...)
            {
                LOG_ERROR("failed to read device info");
            }
            return info;
        }

        uint64_t rs_device_ex::get_capture_time()
        {
            LOG_FUNC_SCOPE();
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::microseconds>(now - m_capture_time_base).count();
        }

        std::vector<rs::core::file_types::device_cap> rs_device_ex::read_all_options()
        {
            LOG_FUNC_SCOPE();
            std::vector<file_types::device_cap> rv;
            try
            {
                std::vector<rs_option> options;
                for(int option = 0; option < rs_option::RS_OPTION_COUNT; option++)
                {
                    if(m_device->supports_option((rs_option)option))
                        options.push_back((rs_option)option);
                }
                std::vector<double> values(options.size());
                m_device->get_options(options.data(), options.size(), values.data());
                rv.resize(options.size());
                for(size_t i = 0; i < rv.size(); ++i)
                {
                    rv[i].label = options[i];
                    rv[i].value = values[i];
                }
            }
            catch(...)
            {
                LOG_ERROR("failed to read device options");
            }
            return rv;
        }
        std::map<rs_camera_info, std::pair<uint32_t, const char*>> rs_device_ex::get_all_camera_info()
        {
            std::map<rs_camera_info, std::pair<uint32_t, const char*>> info_map;
            for(int i=0; i< static_cast<int>(rs_camera_info::RS_CAMERA_INFO_COUNT); i++)
            {
                rs_camera_info cam_info_id = static_cast<rs_camera_info>(i);
                if(m_device->supports(cam_info_id) == false)
                {
                    continue;
                }
                const char* info = m_device->get_camera_info(cam_info_id);
                uint32_t len = static_cast<uint32_t>(std::strlen(info));
                uint32_t string_size = len + 1; //"+1" for the '\0' char
                info_map.emplace(cam_info_id,  std::pair<uint32_t, const char*> {string_size, info});
            }
            return info_map;
        }

        void rs_device_ex::write_frame(rs_stream stream, rs_frame_ref * ref)
        {
            auto frame = new file_types::frame_sample(stream, ref, get_capture_time());
            std::shared_ptr<file_types::sample> sample = std::shared_ptr<file_types::sample>(frame,
                    [this, ref](file_types::sample* f)
            {
                m_device->release_frame(ref);
            });
            m_disk_write.record_sample(sample);
        }

        void rs_device_ex::write_samples()
        {
            auto capture_time = get_capture_time();
            for(auto it = m_active_streams.begin(); it != m_active_streams.end(); ++it)
            {
#ifndef lrs_empty_first_frames_workaround
                if(m_device->get_stream_interface(*it).get_frame_number() == 0) continue;
#endif
                file_types::frame_sample frame(*it, m_device->get_stream_interface(*it), capture_time);
                std::shared_ptr<file_types::sample> sample = std::shared_ptr<file_types::sample>(frame.copy(),
                [](file_types::sample* f) { delete[] (static_cast<file_types::frame_sample*>(f))->data; delete f;});
                m_disk_write.record_sample(sample);
            }
        }

        core::status rs_device_ex::configure_disk_write()
        {
            configuration config = {};
            config.m_capabilities = get_capabilities();
            config.m_coordinate_system = file_types::coordinate_system::rear_default;
            config.m_device_info = get_device_info();
            config.m_file_path = m_file_path;
            config.m_options = read_all_options();
            config.m_stream_profiles = get_profiles();
            config.m_motion_intrinsics = get_motion_intrinsics();
            config.m_capture_mode = m_capture_mode;
            config.m_camera_info = get_all_camera_info();
            return m_disk_write.configure(config);
        }

        std::vector<rs_capabilities> rs_device_ex::get_capabilities()
        {
            for(auto it = m_active_streams.begin(); it != m_active_streams.end(); ++it)
            {
                auto cap = get_capability(*it);
                if(cap == rs_capabilities::RS_CAPABILITIES_COUNT)continue;
                m_capabilities.push_back(cap);
            }
            if((m_source == RS_SOURCE_ALL || m_source == RS_SOURCE_MOTION_TRACKING) && m_is_motion_tracking_enabled)
            {
                m_capabilities.push_back(rs_capabilities::RS_CAPABILITIES_MOTION_EVENTS);
            }
            return m_capabilities;
        }

        std::map<rs_stream, file_types::stream_profile> rs_device_ex::get_profiles()
        {
            std::map<rs_stream, file_types::stream_profile> profiles;
            for(auto it = m_active_streams.begin(); it != m_active_streams.end(); ++it)
            {
                auto& si = m_device->get_stream_interface(*it);
                auto intr = si.get_intrinsics();
                file_types::frame_info fi = {intr.width, intr.height, si.get_format()};
                fi.framerate = si.get_framerate();
                rs_intrinsics intrinsics = intr;
                rs_intrinsics rect_intrinsics = {};
                rs_extrinsics extrinsics = {};

                //save empty calibration data in case rectified intrinsics data is not valid
                try {rect_intrinsics = si.get_rectified_intrinsics();}
                catch(...) {LOG_WARN("failed to read rectified intrinsics of stream - " << *it);}

                //save empty calibration data in case extrinsics data is not valid
                try {extrinsics = si.get_extrinsics_to(m_device->get_stream_interface(rs_stream::RS_STREAM_DEPTH));}
                catch(...) {LOG_WARN("failed to read extrinsics of stream - " << *it);}

                auto depth_scale = *it == rs_stream::RS_STREAM_DEPTH ? m_device->get_depth_scale() : 0;
                profiles[*it] = {fi, si.get_framerate(), intrinsics, rect_intrinsics, extrinsics, depth_scale};

                //save empty calibration data in case motion calibration data is not valid
                try { profiles[*it].motion_extrinsics = m_device->get_motion_extrinsics_from(*it); }
                catch(...)
                {
                    LOG_WARN("failed to read motion extrinsics of stream - " << *it)
                    rs_extrinsics ext = {};
                    profiles[*it].motion_extrinsics = ext;
                }
            }
            return profiles;
        }

        void rs_device_ex::update_active_streams(rs_stream stream, bool state)
        {
            if(state)
            {
                if(std::find(m_active_streams.begin(), m_active_streams.end(), stream) == m_active_streams.end())
                    m_active_streams.push_back(stream);
            }
            else
            {
                m_active_streams.erase(std::remove(m_active_streams.begin(), m_active_streams.end(), stream), m_active_streams.end());
            }
        }
        /************************************************************************************************************/
        //rs::device extention
        /************************************************************************************************************/

        void device::pause_record()
        {
            ((rs_device_ex*)this)->pause_record();
        }

        void device::resume_record()
        {
            ((rs_device_ex*)this)->resume_record();
        }

        void device::set_compression(bool compress)
        {
            throw std::runtime_error("not implemented");
        }
    }
}
