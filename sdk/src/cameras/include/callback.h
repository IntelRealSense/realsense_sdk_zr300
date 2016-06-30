#pragma once
#include <librealsense/rs.hpp>

namespace rs
{
    namespace core
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
    }
}
