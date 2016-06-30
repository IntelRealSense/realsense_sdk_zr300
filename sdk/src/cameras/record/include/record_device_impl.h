// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "record_device_interface.h"
#include "disk_write.h"
#include "include/callback.h"

namespace rs
{
    namespace record
    {
        class rs_device_ex : public device_interface
        {
        public:
            rs_device_ex(const std::string& file_path, rs_device *device);
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
            virtual void                            release_frames(rs_frameset * frameset) override;
            virtual rs_frameset *                   clone_frames(rs_frameset * frameset) override;
            virtual bool                            supports(rs_capabilities capability) const override;
            virtual bool                            supports_option(rs_option option) const override;
            virtual void                            get_option_range(rs_option option, double & min, double & max, double & step, double & def) override;
            virtual void                            set_options(const rs_option options[], int count, const double values[]) override;
            virtual void                            get_options(const rs_option options[], int count, double values[]) override;
            virtual rs_frame_ref *                  detach_frame(const rs_frameset * fs, rs_stream stream) override;
            virtual void                            release_frame(rs_frame_ref * ref) override;
            virtual rs_frame_ref *                  clone_frame(rs_frame_ref * frame) override;
            virtual const char *                    get_usb_port_id() const;

            virtual void                            pause_record() override;
            virtual void                            resume_record() override;

            void                                    write_motion(rs_motion_data motion);
            void                                    write_time_stamp(rs_timestamp_data time_stamp);
            void                                    write_color_frame(rs_frame_ref * rec_ref, rs_frame_ref * usr_ref);
            void                                    write_depth_frame(rs_frame_ref * rec_ref, rs_frame_ref * usr_ref);
            void                                    write_infrared_frame(rs_frame_ref * rec_ref, rs_frame_ref * usr_ref);
            void                                    write_infrared2_frame(rs_frame_ref * rec_ref, rs_frame_ref * usr_ref);
            void                                    write_fisheye_frame(rs_frame_ref * rec_ref, rs_frame_ref * usr_ref);

        private:
            void set_device_info();
            void write_sample();
            void set_capabilities();
            void set_profiles();
            void write_frame(rs_stream stream, rs_frame_ref *ref);
            uint64_t get_sync_time();
            std::vector<core::file_types::device_cap> read_all_options();

            bool                                                                    m_is_streaming;
            rs_device *                                                             m_device;
            disk_write                                                              m_disk_write;
            std::map<rs_stream, bool>                                               m_active_streams;
            std::string                                                             m_file_path;
            std::vector<core::file_types::device_cap>                               m_modifyied_options;
            std::map<rs_stream,std::shared_ptr<rs_frame_callback>>                  m_user_frame_callbacks;
            std::map<rs_stream,core::frame_callback*>                               m_frame_callbacks;
            std::shared_ptr<rs_motion_callback>                                     m_user_motion_callbacks;
            core::motion_events_callback*                                           m_motion_callbacks;
            std::shared_ptr<rs_timestamp_callback>                                  m_user_timestamp_callbacks;
            core::timestamp_events_callback*                                        m_timestamp_callbacks;
            std::chrono::high_resolution_clock::time_point                          m_sync_base;
            std::vector<rs_capabilities>                                            m_capabilities;
            rs_source                                                               m_source;
        };
    }
}



