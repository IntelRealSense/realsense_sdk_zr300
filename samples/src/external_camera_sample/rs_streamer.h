#pragma once

#include <exception>
#include <librealsense/rs.hpp>
#include <rs/core/context.h>

class not_initialized_exception : public std::exception{};

class rs_streamer
{
public:
    bool init()
    {
        if(m_context.get_device_count() > 0)
        {
            is_init = true;
            m_device = m_context.get_device(0);
        }
        return is_init;

    }
    void start_streaming(rs::stream stream, rs::format format, unsigned int width, unsigned int height, unsigned int fps,
               std::function<void(rs::frame)> callback)
    {
        if(!is_init)
        {
            throw not_initialized_exception();
        }
        m_device->enable_stream(stream, width, height, format, fps);
        m_device->set_frame_callback(stream, callback);
        m_device->start();
    }

    void stop_streaming()
    {
        if(!is_init)
        {
            throw not_initialized_exception();
        }
        m_device->stop();
        std::cout << "Stopped streaming rs_streamer" << std::endl;
    }

private:
    bool is_init = false;
    rs::device* m_device = nullptr;
    rs::core::context m_context; //context scope must remain available throughout the program
};
