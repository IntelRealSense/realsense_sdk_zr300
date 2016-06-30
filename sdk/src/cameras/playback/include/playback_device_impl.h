// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <memory>
#include <mutex>
#include <thread>
#include "playback_device_interface.h"
#include "disk_read_interface.h"
#include "include/callback.h"
#include <condition_variable>

namespace rs
{
    namespace playback
    {
        class rs_frame_ref_impl : public rs_frame_ref
        {
            // rs_frame_ref interface
        public:
            rs_frame_ref_impl(std::shared_ptr<rs::core::file_types::frame> frame) : m_frame(frame) {}
            std::shared_ptr<rs::core::file_types::frame> get_frame() { return m_frame; }
            virtual const uint8_t *get_frame_data() const override { return m_frame->data; }
            virtual int get_frame_timestamp() const override { return m_frame->info.time_stamp; }
            virtual int get_frame_number() const override { return m_frame->info.number; }
            virtual long long get_frame_system_time() const override { return m_frame->info.system_time; }
            virtual int get_frame_width() const override { return m_frame->info.width; }
            virtual int get_frame_height() const override { return m_frame->info.height; }
            virtual int get_frame_framerate() const override { return m_frame->info.framerate; }
            virtual int get_frame_stride() const override { return m_frame->info.stride; }
            virtual int get_frame_bpp() const override { return m_frame->info.bpp; }
            virtual rs_format get_frame_format() const override { return m_frame->info.format; }
        private:
            std::shared_ptr<rs::core::file_types::frame> m_frame;
        };

        class frameset_impl : public rs_frameset
        {
        public:
            frameset_impl() {}
            frameset_impl(std::map<rs_stream, std::shared_ptr<rs_frame_ref>> frames) : m_frames(frames) {}
            ~frameset_impl() {}
            frameset_impl* clone()
            {
                std::map<rs_stream, std::shared_ptr<rs_frame_ref>> frames;
                for(auto it = m_frames.begin(); it != m_frames.end(); ++it)
                {
                    frames[it->first] = it->second;
                }
                return new frameset_impl(frames);
            }
            rs_frame_ref * detach(rs_stream stream)
            {
                auto frame = std::dynamic_pointer_cast<rs_frame_ref_impl>(m_frames[stream]);
                auto rv = new rs_frame_ref_impl(frame->get_frame());
                m_frames.erase(stream);
                return rv;
            }
            rs_frame_ref * get_frame(rs_stream stream)
            {
                return m_frames.find(stream) != m_frames.end() ? m_frames[stream].get() : nullptr;
            }

        private:
            std::map<rs_stream, std::shared_ptr<rs_frame_ref>> m_frames;
        };

        template <class sample_type, class user_callback> class sample_thread_utils //replace name
        {
        public:
            std::thread                                 m_thread;
            std::mutex                                  m_mutex;
            std::condition_variable                     m_cv;
            std::shared_ptr<sample_type>                m_sample;
            std::shared_ptr<user_callback>              m_callback;
        };

        class rs_stream_impl : public rs_stream_interface
        {
            // rs_stream_interface interface
        public:
            rs_stream_impl() : m_is_enabled(false) {}
            rs_stream_impl(rs::core::file_types::stream_info &stream_info) : m_stream_info(stream_info), m_is_enabled(false) {}
            void set_frame(std::shared_ptr<rs::core::file_types::frame> &frame) { m_frame = frame;}
            std::shared_ptr<rs::core::file_types::frame> get_frame() { return m_frame; }
            void clear_data() { m_frame.reset(); }
            void set_is_enabled(bool state) { m_is_enabled = state; }
            virtual rs_extrinsics get_extrinsics_to(const rs_stream_interface &r) const override { return m_stream_info.profile.extrinsics; }
            virtual float get_depth_scale() const override { return m_stream_info.profile.depth_scale; }
            virtual rs_intrinsics get_intrinsics() const override { return m_stream_info.profile.intrinsics; }
            virtual rs_intrinsics get_rectified_intrinsics() const override { return m_stream_info.profile.rect_intrinsics; }
            virtual rs_format get_format() const override { return m_stream_info.profile.info.format; }
            virtual int get_framerate() const override { return m_stream_info.profile.frame_rate; }
            virtual int get_frame_number() const override { return m_frame ? m_frame->info.number : 0; }
            virtual long long get_frame_system_time() const override { return m_frame ? m_frame->info.system_time : 0; }
            virtual const uint8_t *get_frame_data() const override { return m_frame ? m_frame->data : nullptr; }
            virtual int get_mode_count() const override { return get_format() == rs_format::RS_FORMAT_ANY ? 0 : 1; }
            virtual void get_mode(int mode, int *w, int *h, rs_format *f, int *fps) const override
            {
                *w = mode == 0 ? m_stream_info.profile.info.width : 0;
                *h = mode == 0 ? m_stream_info.profile.info.height : 0;
                *f = mode == 0 ? m_stream_info.profile.info.format : rs_format::RS_FORMAT_ANY;
                *fps = mode == 0 ? m_stream_info.profile.frame_rate : 0;
            }
            virtual bool is_enabled() const override { return m_is_enabled; }
            virtual bool has_data() const { return m_frame ? true : false; }

        private:
            bool                                               m_is_enabled;
            rs::core::file_types::stream_info                  m_stream_info;
            std::shared_ptr<rs::core::file_types::frame>       m_frame;
        };

        class rs_device_ex : public device_interface
        {
        public:
            rs_device_ex(const std::string &file_path);
            virtual ~rs_device_ex();
            virtual const rs_stream_interface &     get_stream_interface(rs_stream stream) const override;
            virtual const char *                    get_name() const override;
            virtual const char *                    get_serial() const override;
            virtual const char *                    get_firmware_version() const override;
            virtual float                           get_depth_scale() const override;
            virtual void                            enable_stream(rs_stream stream, int width, int height, rs_format format, int fps, rs_output_buffer_format output) override;
            virtual void                            enable_stream_preset(rs_stream stream, rs_preset preset) override;
            virtual void                            disable_stream(rs_stream stream) override;
            virtual void                            enable_motion_tracking() override;
            virtual void                            set_stream_callback(rs_stream stream, void(*on_frame)(rs_device * device, rs_frame_ref * frame, void * user), void * user) override;
            virtual void                            set_stream_callback(rs_stream stream, rs_frame_callback * callback) override;
            virtual void                            disable_motion_tracking() override;
            virtual void                            set_motion_callback(void(*on_event)(rs_device * device, rs_motion_data data, void * user), void * user) override;
            virtual void                            set_motion_callback(rs_motion_callback * callback) override;
            virtual void                            set_timestamp_callback(void(*on_event)(rs_device * device, rs_timestamp_data data, void * user), void * user) override;
            virtual void                            set_timestamp_callback(rs_timestamp_callback * callback) override;
            virtual void                            start(rs_source source) override;
            virtual void                            stop(rs_source source) override;
            virtual bool                            is_capturing() const override;
            virtual int                             is_motion_tracking_active() const override;
            virtual void                            wait_all_streams() override;
            virtual bool                            poll_all_streams() override;
            virtual rs_frameset *                   wait_all_streams_safe() override;
            virtual bool                            poll_all_streams_safe(rs_frameset ** frames) override;
            virtual void                            release_frames(rs_frameset * frameset_impl) override;
            virtual rs_frameset *                   clone_frames(rs_frameset * frameset_impl) override;
            virtual bool                            supports(rs_capabilities capability) const override;
            virtual bool                            supports_option(rs_option option) const override;
            virtual void                            get_option_range(rs_option option, double & min, double & max, double & step, double & def) override;
            virtual void                            set_options(const rs_option options[], int count, const double values[]) override;
            virtual void                            get_options(const rs_option options[], int count, double values[]) override;
            virtual rs_frame_ref *                  detach_frame(const rs_frameset * fs, rs_stream stream) override;
            virtual void                            release_frame(rs_frame_ref * ref) override;
            virtual rs_frame_ref *                  clone_frame(rs_frame_ref * frame) override;
            virtual const char *                    get_usb_port_id() const;

            virtual bool                            init() override;
            virtual bool                            is_real_time() override;
            virtual void                            pause() override;
            virtual void                            resume() override;
            virtual bool                            set_frame_by_index(int index, rs_stream stream) override;
            virtual bool                            set_frame_by_timestamp(uint64_t timestamp) override;
            virtual void                            set_real_time(bool realtime) override;
            virtual int                             get_frame_index(rs_stream stream) override;
            virtual int                             get_frame_count(rs_stream stream) override;
            virtual int                             get_frame_count() override;


        private:
            bool                                    all_streams_availeble();
            void                                    clear_streams_data();
            frameset_impl*                          create_frameset();
            uint32_t                                get_active_streams_max_fps();
            void                                    start_callbacks_threads();
            void                                    join_callbacks_threads();
            void                                    frame_callback_thread(rs_stream stream);
            void                                    motion_callback_thread();
            void                                    time_stamp_callback_thread();

            bool                                                                                m_wait_streams_request;
            std::condition_variable                                                             m_all_stream_availeble_cv;
            std::mutex                                                                          m_all_stream_availeble_mutex;
            bool                                                                                m_is_streaming;
            bool                                                                                m_is_motion_tracking;
            uint32_t                                                                            m_max_fps;
            std::mutex                                                                          m_mutex;
            std::string                                                                         m_file_path;
            std::map<rs_stream,std::unique_ptr<rs_stream_impl>>                                 m_availeble_streams;
            std::map<rs_stream,std::shared_ptr<core::file_types::frame>>                        m_curr_frames;
            std::unique_ptr<rs_stream_impl>                                                     m_empty_stream;
            std::map<core::file_types::sample_type,bool>                                        m_enabled_callbacks;
            std::map<rs_stream, sample_thread_utils<core::file_types::frame,rs_frame_callback>> m_frame_thread;
            sample_thread_utils<core::file_types::motion,rs_motion_callback>                    m_motion_thread;
            sample_thread_utils<core::file_types::time_stamp,rs_timestamp_callback>             m_time_stamp_thread;
            std::unique_ptr<disk_read_interface>                                                m_disk_read;
        };
    }
}


