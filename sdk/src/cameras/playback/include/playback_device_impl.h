// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include "playback_device_interface.h"
#include "disk_read_interface.h"
#include "rs_stream_impl.h"

namespace rs
{
    namespace playback
    {
        class rs_frame_ref_impl : public rs_frame_ref
        {
        public:
            rs_frame_ref_impl(std::shared_ptr<rs::core::file_types::frame_sample> frame) : m_frame(frame), m_ref_count(1) {}
            std::shared_ptr<rs::core::file_types::frame_sample> get_frame() { return m_frame; }
            virtual const uint8_t *get_frame_data() const override { return m_frame->data; }
            virtual double get_frame_timestamp() const override { return m_frame->finfo.time_stamp; }
            virtual int get_frame_number() const override { return m_frame->finfo.number; }
            virtual long long get_frame_system_time() const override { return m_frame->finfo.system_time; }
            virtual int get_frame_width() const override { return m_frame->finfo.width; }
            virtual int get_frame_height() const override { return m_frame->finfo.height; }
            virtual int get_frame_framerate() const override { return m_frame->finfo.framerate; }
            virtual int get_frame_stride_x() const override { return m_frame->finfo.stride_x; }
            virtual int get_frame_stride_y() const override { return m_frame->finfo.stride_y; }
            virtual float get_frame_bpp() const override { return m_frame->finfo.bpp; }
            virtual rs_format get_frame_format() const override { return m_frame->finfo.format; }
            virtual rs_stream get_stream_type() const override { return m_frame->finfo.stream; }
            void add_ref() { m_ref_count++; }
            void release() { if(--m_ref_count == 0) delete this; }

        private:
            size_t m_ref_count;
            std::shared_ptr<rs::core::file_types::frame_sample> m_frame;
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
            virtual bool                            supports(rs_capabilities capability) const override;
            virtual bool                            supports_option(rs_option option) const override;
            virtual void                            get_option_range(rs_option option, double & min, double & max, double & step, double & def) override;
            virtual void                            set_options(const rs_option options[], size_t count, const double values[]) override;
            virtual void                            get_options(const rs_option options[], size_t count, double values[]) override;
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
            void                                    set_enabled_streams();
            void                                    start_callbacks_threads();
            void                                    join_callbacks_threads();
            void                                    signal_all();
            void                                    end_of_file();
            void                                    frame_callback_thread(rs_stream stream);
            void                                    motion_callback_thread();
            void                                    time_stamp_callback_thread();
            void                                    handle_frame_callback(std::shared_ptr<core::file_types::sample> sample);
            void                                    handle_motion_callback(std::shared_ptr<core::file_types::sample> sample);
            void                                    handle_time_stamp_callback(std::shared_ptr<core::file_types::sample> sample);

            bool                                                                                        m_wait_streams_request;
            std::condition_variable                                                                     m_all_stream_availeble_cv;
            std::mutex                                                                                  m_all_stream_availeble_mutex;
            bool                                                                                        m_is_streaming;
            std::mutex                                                                                  m_mutex;
            std::mutex                                                                                  m_pause_resume_mutex;
            std::string                                                                                 m_file_path;
            std::map<rs_stream,std::unique_ptr<rs_stream_impl>>                                         m_available_streams;
            std::map<rs_stream,std::shared_ptr<core::file_types::frame_sample>>                         m_curr_frames;
            std::map<rs_stream, sample_thread_utils<core::file_types::frame_sample,rs_frame_callback>>  m_frame_thread;
            sample_thread_utils<core::file_types::motion_sample,rs_motion_callback>                     m_motion_thread;
            sample_thread_utils<core::file_types::time_stamp_sample,rs_timestamp_callback>              m_time_stamp_thread;
            std::unique_ptr<disk_read_interface>                                                        m_disk_read;
            size_t                                                                                      m_enabled_streams_count;
        };
    }
}


