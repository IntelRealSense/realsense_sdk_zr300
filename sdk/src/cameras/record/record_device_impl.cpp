// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

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
            default: return rs_capabilities::RS_CAPABILITIES_MAX_ENUM;
        }
    }
}

namespace rs
{
    namespace record
    {
        rs_device_ex::rs_device_ex(const std::string &file_path, rs_device *device) :
            m_device(device),
            m_file_path(file_path),
            m_is_streaming(false)
        {

        }

        rs_device_ex::~rs_device_ex()
        {
            m_disk_write.stop();
            if(m_is_streaming)
                stop(m_source);
        }

        const rs_stream_interface & rs_device_ex::get_stream_interface(rs_stream stream) const
        {
            return m_device->get_stream_interface(stream);
        }

        const char * rs_device_ex::get_name() const
        {
            return m_device->get_name();
        }

        const char * rs_device_ex::get_serial() const
        {
            return m_device->get_serial();
        }

        const char * rs_device_ex::get_firmware_version() const
        {
            return m_device->get_firmware_version();
        }

        float rs_device_ex::get_depth_scale() const
        {
            return m_device->get_depth_scale();
        }

        void rs_device_ex::enable_stream(rs_stream stream, int width, int height, rs_format format, int fps, rs_output_buffer_format output)
        {
            LOG_INFO("enable stream - " << stream << " ,width - " << width << " ,height - " << height << " ,format - " << format << " ,fps -" << fps)
            m_device->enable_stream(stream, width, height, format, fps, output);
            m_active_streams[stream] = true;
        }

        void rs_device_ex::enable_stream_preset(rs_stream stream, rs_preset preset)
        {
            LOG_INFO("enable stream - " << stream << " ,preset - " << preset)
            m_device->enable_stream_preset(stream, preset);
            m_active_streams[stream] = true;
        }

        void rs_device_ex::disable_stream(rs_stream stream)
        {
            LOG_INFO("disable stream - " << stream)
            m_device->disable_stream(stream);
            m_active_streams[stream] = false;
        }

        void rs_device_ex::enable_motion_tracking()
        {
            LOG_INFO("enable motion tracking")
            m_device->enable_motion_tracking();
        }

        void rs_device_ex::set_stream_callback(rs_stream stream, void(*on_frame)(rs_device * device, rs_frame_ref * frame, void * user), void * user)
        {
            set_stream_callback(stream, new core::frame_callback(this, on_frame, user));
        }

        void rs_device_ex::set_stream_callback(rs_stream stream, rs_frame_callback * callback)
        {
            LOG_INFO("stream - " << stream)
            m_user_frame_callbacks[stream] = std::shared_ptr<rs_frame_callback>(callback, [](rs_frame_callback* cb)
            {cb->release();});
            switch(stream)
            {
                case rs_stream::RS_STREAM_COLOR:
                {
                    m_frame_callbacks[stream] = new core::frame_callback(this, [](rs_device * device, rs_frame_ref * fref, void * user)
                    {
                        auto this_device = (rs_device_ex*)user;
                        auto clone = this_device->clone_frame(fref);
                        this_device->write_color_frame(clone, fref);
                    }, this);
                    break;
                }
                case rs_stream::RS_STREAM_DEPTH:
                {
                    m_frame_callbacks[stream] = new core::frame_callback(this, [](rs_device * device, rs_frame_ref * fref, void * user)
                    {
                        auto this_device = (rs_device_ex*)user;
                        auto clone = this_device->clone_frame(fref);
                        this_device->write_depth_frame(clone, fref);
                    }, this);
                    break;
                }
                case rs_stream::RS_STREAM_INFRARED:
                {
                    m_frame_callbacks[stream] = new core::frame_callback(this, [](rs_device * device, rs_frame_ref * fref, void * user)
                    {
                        auto this_device = (rs_device_ex*)user;
                        auto clone = this_device->clone_frame(fref);
                        this_device->write_infrared_frame(clone, fref);
                    }, this);
                    break;
                }
                case rs_stream::RS_STREAM_INFRARED2:
                {
                    m_frame_callbacks[stream] = new core::frame_callback(this, [](rs_device * device, rs_frame_ref * fref, void * user)
                    {
                        auto this_device = (rs_device_ex*)user;
                        auto clone = this_device->clone_frame(fref);
                        this_device->write_infrared2_frame(clone, fref);
                    }, this);
                    break;
                }
                case rs_stream::RS_STREAM_FISHEYE:
                {
                    m_frame_callbacks[stream] = new core::frame_callback(this, [](rs_device * device, rs_frame_ref * fref, void * user)
                    {
                        auto this_device = (rs_device_ex*)user;
                        auto clone = this_device->clone_frame(fref);
                        this_device->write_fisheye_frame(clone, fref);
                    }, this);
                    break;
                }
                default:return;
            }
            m_device->set_stream_callback(stream, m_frame_callbacks[stream]);
        }

        void rs_device_ex::disable_motion_tracking()
        {
            LOG_INFO("disable motion tracking")
            m_device->disable_motion_tracking();
        }

        void rs_device_ex::set_motion_callback(void(*on_event)(rs_device * device, rs_motion_data data, void * user), void * user)
        {
            set_motion_callback(new motion_events_callback(this, on_event, user));
        }

        void rs_device_ex::set_motion_callback(rs_motion_callback * callback)
        {
            LOG_INFO("set motion callback")
            m_user_motion_callbacks = std::shared_ptr<rs_motion_callback>(callback, [](rs_motion_callback* cb)
            { cb->release(); });
            m_motion_callbacks = new motion_events_callback(this, [](rs_device * device, rs_motion_data data, void * user)
            {
                auto this_device = (rs_device_ex*)user; this_device->write_motion(data);
            }, this);
            m_device->set_motion_callback(m_motion_callbacks);
            m_capabilities.push_back(rs_capabilities::RS_CAPABILITIES_MOTION_EVENTS);
        }

        void rs_device_ex::set_timestamp_callback(void(*on_event)(rs_device * device, rs_timestamp_data data, void * user), void * user)
        {
            set_timestamp_callback(new timestamp_events_callback(this, on_event, user));
        }

        void rs_device_ex::set_timestamp_callback(rs_timestamp_callback * callback)
        {
            LOG_INFO("set time stamp callback")
            m_user_timestamp_callbacks = std::shared_ptr<rs_timestamp_callback>(callback, [](rs_timestamp_callback* cb)
            { cb->release(); });
            m_timestamp_callbacks = new timestamp_events_callback(this, [](rs_device * device, rs_timestamp_data data, void * user)
            {
                auto this_device = (rs_device_ex*)user; this_device->write_time_stamp(data);
            }, this);
            m_device->set_timestamp_callback(m_timestamp_callbacks);
        }

        void rs_device_ex::start(rs_source source)
        {
            LOG_INFO("start");
            m_source = source;
            set_device_info();
            m_disk_write.set_properties(read_all_options());
            m_disk_write.set_file_path(m_file_path.c_str());
            set_capabilities();
            set_profiles();
            m_sync_base = std::chrono::high_resolution_clock::now();
            m_disk_write.start();
            m_device->start(source);
            m_is_streaming = true;
        }

        void rs_device_ex::stop(rs_source source)
        {
            LOG_INFO("stop");
            pause_record();
            m_device->stop(source);
            m_is_streaming = false;
        }

        bool rs_device_ex::is_capturing() const
        {
            return m_device->is_capturing();
        }

        int rs_device_ex::is_motion_tracking_active() const
        {
            return m_device->is_motion_tracking_active();
        }

        void rs_device_ex::wait_all_streams()
        {
            LOG_VERBOSE("wait all streams request")
            m_device->wait_all_streams();
            write_sample();
        }

        bool rs_device_ex::poll_all_streams()
        {
            LOG_VERBOSE("poll all streams request")
            auto rv = m_device->poll_all_streams();
            if(rv)
            {
                write_sample();
            }
            return rv;
        }

        rs_frameset * rs_device_ex::wait_all_streams_safe()
        {
            LOG_VERBOSE("wait all streams safe request")
            auto frames = m_device->wait_all_streams_safe();
            for(auto it = m_active_streams.begin(); it != m_active_streams.end(); ++it)
            {
                if(!it->second)continue;
                auto stream = it->first;
                auto clone = m_device->clone_frame(frames->get_frame(stream));
                write_frame(it->first, clone);
            }
            return frames;
        }

        bool rs_device_ex::poll_all_streams_safe(rs_frameset ** frames)
        {
            LOG_VERBOSE("poll all streams safe request")
            auto rv = m_device->poll_all_streams_safe(frames);
            if(rv)
            {
                for(auto it = m_active_streams.begin(); it != m_active_streams.end(); ++it)
                {
                    if(!it->second)continue;
                    auto stream = it->first;
                    auto clone = m_device->clone_frame((*frames)->get_frame(stream));
                    write_frame(it->first, clone);
                }
                return frames;
            }
            return rv;
        }

        void rs_device_ex::release_frames(rs_frameset * frameset)
        {
            LOG_VERBOSE("release frames")
            m_device->release_frames(frameset);
        }

        rs_frameset * rs_device_ex::clone_frames(rs_frameset * frameset)
        {
            LOG_VERBOSE("clone frames")
            return m_device->clone_frames(frameset);
        }

        bool rs_device_ex::supports(rs_capabilities capability) const
        {
            return m_device->supports(capability);
        }

        bool rs_device_ex::supports_option(rs_option option) const
        {
            return m_device->supports_option(option);
        }

        void rs_device_ex::get_option_range(rs_option option, double & min, double & max, double & step, double & def)
        {
            m_device->get_option_range(option, min, max, step, def);
        }

        void rs_device_ex::set_options(const rs_option options[], int count, const double values[])
        {
            LOG_INFO("set options, options count - " << count)
            m_device->set_options(options, count, values);
            if(!m_device->is_capturing()) return;
            for(auto i = 0; i < count; i++)
            {
                file_types::device_cap dc = {options[i], values[i]};
                m_modifyied_options.push_back(dc);
            }
        }

        void rs_device_ex::get_options(const rs_option options[], int count, double values[])
        {
            m_device->get_options(options, count, values);
        }

        rs_frame_ref * rs_device_ex::detach_frame(const rs_frameset * fs, rs_stream stream)
        {
            LOG_VERBOSE("detach frame")
            return m_device->detach_frame(fs, stream);
        }

        void rs_device_ex::release_frame(rs_frame_ref * ref)
        {
            LOG_VERBOSE("release frame")
            m_device->release_frame(ref);
        }

        rs_frame_ref * rs_device_ex::clone_frame(rs_frame_ref * frame)
        {
            LOG_VERBOSE("clone frame")
            return m_device->clone_frame(frame);
        }

        const char * rs_device_ex::get_usb_port_id() const
        {
            return m_device->get_usb_port_id();
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

        void rs_device_ex::set_device_info()
        {
            core::file_types::device_info info;
            memset(&info, 0, sizeof(device_info));
            strcpy(info.name, get_name());
            strcpy(info.serial, get_serial());
            strcpy(info.firmware, get_firmware_version());
            strcpy(info.usb_port_id, get_usb_port_id());
            info.rotation = rotation::rotation_0_degree;
            m_disk_write.set_device_info(info);
        }

        uint64_t rs_device_ex::get_sync_time()
        {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(now - m_sync_base).count();
        }

        std::vector<rs::core::file_types::device_cap> rs_device_ex::read_all_options()
        {
            std::vector<rs_option> options;
            for(int option = rs_option::RS_OPTION_COLOR_BACKLIGHT_COMPENSATION; option < rs_option::RS_OPTION_COUNT; option++)
            {
                if(m_device->supports_option((rs_option)option))
                    options.push_back((rs_option)option);
            }
            std::vector<double> values;
            for(auto o : options)
            {
                std::vector<rs_option> opts(1);
                std::vector<double> vals(opts.size());
                m_device->get_options(opts.data(), opts.size(), vals.data());
                values.push_back(vals[0]);
            }
            std::vector<file_types::device_cap> rv(values.size());
            for(size_t i = 0; i < rv.size(); ++i)
            {
                rv[i].label = options[i];
                rv[i].value = values[i];
            }
            return rv;
        }

        void rs_device_ex::write_motion(rs_motion_data motion)
        {
            std::vector<std::shared_ptr<file_types::sample>> samples;
            samples.push_back(std::shared_ptr<file_types::sample>(
            new file_types::motion(motion, get_sync_time()),[](file_types::sample* s) {}));
            m_disk_write.record_sample(samples);
            m_user_motion_callbacks->on_event(motion);
        }

        void rs_device_ex::write_time_stamp(rs_timestamp_data time_stamp)
        {
            std::vector<std::shared_ptr<file_types::sample>> samples;
            samples.push_back(std::shared_ptr<file_types::sample>(
            new file_types::time_stamp(time_stamp, get_sync_time()),[](file_types::sample* s) {}));
            m_disk_write.record_sample(samples);
            m_user_timestamp_callbacks->on_event(time_stamp);
        }

        void rs_device_ex::write_color_frame(rs_frame_ref * rec_ref, rs_frame_ref *usr_ref)
        {
            write_frame(rs_stream::RS_STREAM_COLOR, rec_ref);
            m_user_frame_callbacks[rs_stream::RS_STREAM_COLOR]->on_frame(this, usr_ref);
        }

        void rs_device_ex::write_depth_frame(rs_frame_ref * rec_ref, rs_frame_ref *usr_ref)
        {
            write_frame(rs_stream::RS_STREAM_DEPTH, rec_ref);
            m_user_frame_callbacks[rs_stream::RS_STREAM_DEPTH]->on_frame(this, usr_ref);
        }

        void rs_device_ex::write_infrared_frame(rs_frame_ref * rec_ref, rs_frame_ref *usr_ref)
        {
            write_frame(rs_stream::RS_STREAM_INFRARED, rec_ref);
            m_user_frame_callbacks[rs_stream::RS_STREAM_INFRARED]->on_frame(this, usr_ref);
        }

        void rs_device_ex::write_infrared2_frame(rs_frame_ref * rec_ref, rs_frame_ref *usr_ref)
        {
            write_frame(rs_stream::RS_STREAM_INFRARED2, rec_ref);
            m_user_frame_callbacks[rs_stream::RS_STREAM_INFRARED2]->on_frame(this, usr_ref);
        }

        void rs_device_ex::write_fisheye_frame(rs_frame_ref * rec_ref, rs_frame_ref * usr_ref)
        {
            write_frame(rs_stream::RS_STREAM_FISHEYE, rec_ref);
            m_user_frame_callbacks[rs_stream::RS_STREAM_FISHEYE]->on_frame(this, usr_ref);
        }

        void rs_device_ex::write_frame(rs_stream stream, rs_frame_ref * ref)
        {
            std::vector<std::shared_ptr<file_types::sample>> samples;
            auto frame = new file_types::frame(stream, ref, get_sync_time());
            samples.push_back(std::shared_ptr<file_types::sample>(frame,
                              [this, ref](file_types::sample* f)
            {
                m_device->release_frame(ref);
            }));
            m_disk_write.record_sample(samples);
        }

        void rs_device_ex::write_sample()
        {
            std::vector<std::shared_ptr<file_types::sample>> samples;

            for(auto iter = m_active_streams.begin(); iter != m_active_streams.end(); ++iter)
            {
                auto stream = iter->first;
                if(!iter->second)continue;
                file_types::frame frame(stream, m_device->get_stream_interface(stream), get_sync_time());
                samples.push_back(std::shared_ptr<file_types::sample>(frame.copy(),
                [](file_types::sample* f) { delete[] (static_cast<file_types::frame*>(f))->data; }));
            }
            m_disk_write.record_sample(samples);
        }

        void rs_device_ex::set_capabilities()
        {
            for(auto it = m_active_streams.begin(); it != m_active_streams.end(); ++it)
            {
                if(!it->second)
                    continue;
                auto cap = get_capability(it->first);
                if(cap == rs_capabilities::RS_CAPABILITIES_MAX_ENUM)continue;
                m_capabilities.push_back(cap);
            }
            m_disk_write.set_capabilities(m_capabilities);
        }

        void rs_device_ex::set_profiles()
        {
            std::map<rs_stream, file_types::stream_profile> profiles;
            for(auto it = m_active_streams.begin(); it != m_active_streams.end(); ++it)
            {
                if(!it->second)
                    continue;
                auto& si = m_device->get_stream_interface(it->first);
                auto intr = si.get_intrinsics();
                file_types::frame_info fi = {intr.width, intr.height, si.get_format()};
                rs_intrinsics intrinsics = intr;
                rs_intrinsics rect_intrinsics =  si.get_rectified_intrinsics();
                rs_extrinsics extrinsics = si.get_extrinsics_to(m_device->get_stream_interface(rs_stream::RS_STREAM_DEPTH));
                auto depth_scale = m_device->get_depth_scale();
                profiles[it->first] = {fi, si.get_framerate(), intrinsics, rect_intrinsics, extrinsics, depth_scale};
            }
            m_disk_write.set_profiles(profiles);
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

    }
}
