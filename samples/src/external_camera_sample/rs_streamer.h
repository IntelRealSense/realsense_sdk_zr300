// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once

#include <exception>
#include <librealsense/rs.hpp>
#include <rs/core/context.h>

class not_initialized_exception : public std::exception{};

class rs_streamer : public streamer_interface<std::function<void(rs::frame)>>
{
public:
    rs_streamer(rs::stream stream, rs::format format, unsigned int width, unsigned int height, unsigned int fps) :
        m_stream(stream), m_format(format), m_width(width), m_height(height), m_fps(fps)   {}
    
    bool init()
    {
        if(m_context.get_device_count() <= 0)
        {
            return false;
        }
        
        m_device = m_context.get_device(0);
        
        try
        {
            m_device->enable_stream(m_stream, m_width, m_height, m_format, m_fps);
        }
        catch(const rs::error& e)
        {
            std::cerr << "Failed to enable stream with requested configuration (passed to constructor)" << std::endl;
            return false;
        }
        m_is_init = true;
        return true;

    }
    
    void start_streaming(std::function<void(rs::frame)> callback)
    {
        if(!m_is_init)
        {
            throw not_initialized_exception();
        }
        m_device->set_frame_callback(m_stream, callback);
        m_device->start();
    }

    void stop_streaming()
    {
        if(!m_is_init)
        {
            throw not_initialized_exception();
        }
        m_device->stop();
        std::cout << "Stopped streaming rs_streamer" << std::endl;
    }

private:
    bool m_is_init = false;
    rs::device* m_device = nullptr;
    rs::core::context m_context; //context scope must remain available throughout the program
    rs::stream   m_stream;
    rs::format   m_format;
    unsigned int m_width;
    unsigned int m_height;
    unsigned int m_fps;
};
