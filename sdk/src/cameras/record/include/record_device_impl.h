// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <mutex>
#include "record_device_interface.h"
#include "disk_write.h"

namespace rs
{
    namespace record
    {
        class frame_callback;
        class rs_device_ex : public device_interface
        {
            friend class frame_callback;
            friend class motion_events_callback;
            friend class timestamp_events_callback;
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
            virtual bool                            supports(rs_capabilities capability) const override;
            virtual bool                            supports(rs_camera_info info_param) const override;
            virtual bool                            supports_option(rs_option option) const override;
            virtual void                            get_option_range(rs_option option, double & min, double & max, double & step, double & def) override;
            virtual void                            set_options(const rs_option options[], size_t count, const double values[]) override;
            virtual void                            get_options(const rs_option options[], size_t count, double values[]) override;
            virtual void                            release_frame(rs_frame_ref * ref) override;
            virtual rs_frame_ref *                  clone_frame(rs_frame_ref * frame) override;
            virtual const char *                    get_usb_port_id() const;

            virtual const char *                    get_camera_info(rs_camera_info) const;
            virtual rs_motion_intrinsics            get_motion_intrinsics() const;
            virtual rs_extrinsics                   get_motion_extrinsics_from(rs_stream from) const;
            virtual void                            start_fw_logger(char fw_log_op_code, int grab_rate_in_ms, std::timed_mutex &mutex);
            virtual void                            stop_fw_logger();
            virtual const char *                    get_option_description(rs_option option) const;

            virtual void                            pause_record() override;
            virtual void                            resume_record() override;
            virtual bool                            set_compression(rs_stream stream, record::compression_level compression_level) override;
            virtual record::compression_level       get_compression(rs_stream stream) override;

        private:
            void write_samples();
            void write_frame(std::shared_ptr<core::file_types::sample> frame);
            void write_frameset(rs_frameset * frameset);
            core::status configure_disk_write();
            std::vector<rs_capabilities> get_capabilities();
            std::map<rs_stream, core::file_types::stream_profile> get_profiles();
            std::vector<core::file_types::device_cap> read_all_options();
            std::map<rs_camera_info, std::pair<uint32_t, const char *> > get_all_camera_info();
            uint64_t get_capture_time();
            void update_active_streams(rs_stream stream, bool state);

            bool                                                                    m_is_streaming;
            std::mutex                                                              m_is_streaming_mutex;
            rs_device *                                                             m_device;
            disk_write                                                              m_disk_write;
            std::vector<rs_stream>                                                  m_active_streams;
            std::string                                                             m_file_path;
            std::vector<core::file_types::device_cap>                               m_modifyied_options;
            std::chrono::high_resolution_clock::time_point                          m_capture_time_base;
            std::vector<rs_capabilities>                                            m_capabilities;
            rs_source                                                               m_source;
            bool                                                                    m_is_motion_tracking_enabled;
            playback::capture_mode                                                  m_capture_mode;
            std::map<rs_stream, compression_level>                                  m_compression_config;
        };
    }
}
