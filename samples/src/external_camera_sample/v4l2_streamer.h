// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once

#include <future>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include "streamer_interface.h"

class v4l_streamer : public streamer_interface<std::function<void(void*, v4l2_buffer, v4l2_format, std::function<void()>)>>
{
public:
    
    v4l_streamer(std::string device_name, uint32_t width, uint32_t height, uint32_t pixel_formal, uint32_t field) :
        m_device_name(device_name), m_requested_format(), m_streaming(false), m_fd(-1)
    {
        m_requested_format = {};
        m_requested_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        m_requested_format.fmt.pix.width = width;
        m_requested_format.fmt.pix.height = height;
        m_requested_format.fmt.pix.pixelformat = pixel_formal;
        m_requested_format.fmt.pix.field = field;
    }
    
    bool init()
    {
        m_fd = open_camera_io(m_device_name);
    
        if(m_fd < 0)
        {
            std::cerr << "Failed to open device " << m_device_name << std::endl;
            return false;
        }
    
        v4l2_capability cap = {};
        if(xioctl(m_fd, VIDIOC_QUERYCAP, &cap) < 0)
        {
            if(errno == EINVAL)
            {
                std::cerr << m_device_name + " is no V4L2 device" << std::endl;
                return false;
            }
            else
            {
                std::cerr << "Failed ioctl: VIDIOC_QUERYCAP" << std::endl;
                return false;
            }
        }
        
        if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
        {
            std::cerr << m_device_name + " is not a video capture device" << std::endl;
            return false;;
        }
        if(!(cap.capabilities & V4L2_CAP_STREAMING))
        {
            std::cerr << m_device_name + " does not support streaming I/O" << std::endl;
            return false;
        }
    
        // Uncomment the following line to print supported yuyv profiles of the device
        // list_yuyv_profile();
    
        //Set the requested pixel format
        if(xioctl(m_fd, VIDIOC_S_FMT, &m_requested_format) < 0)
        {
            std::cerr << "Failed to set pixel format: "
                      << m_requested_format.fmt.pix.width << "x" << m_requested_format.fmt.pix.height
                      << " pixel format: " << m_requested_format.fmt.pix.pixelformat << std::endl;
            return false;
        }
    
        /* Buggy driver paranoia. */
        unsigned int min = m_requested_format.fmt.pix.width * 2;
        if (m_requested_format.fmt.pix.bytesperline < min)
            m_requested_format.fmt.pix.bytesperline = min;
        min = m_requested_format.fmt.pix.bytesperline * m_requested_format.fmt.pix.height;
        if (m_requested_format.fmt.pix.sizeimage < min)
            m_requested_format.fmt.pix.sizeimage = min;

        return init_buffer_pool(m_requested_format.fmt.pix.sizeimage);;
    }
    
    void start_streaming(std::function<void(void*, v4l2_buffer, v4l2_format, std::function<void()>)> frame_callback)
    {
        if(frame_callback == nullptr)
        {
            throw std::invalid_argument("nullptr frame_callback");
        }

        m_streamingThread = std::async(std::launch::async, &v4l_streamer::streaming_proc, this, frame_callback);
    }

    void stop_streaming()
    {
        m_streaming = false;
        if(m_streamingThread.valid())
        {
            try {
                std::future_status sts = m_streamingThread.wait_for(std::chrono::seconds(1));
                if (sts == std::future_status::timeout)
                {
                    std::cerr << "Timeout while waiting for streaming thread to finish" << std::endl;
                }
            }
            catch (const std::exception &e) {
                std::cerr << "Exception while waiting for streaming thread to finish. Message: " << e.what()
                          << std::endl;
            }
        }
        close_camera_io();
        std::cout << "Stopped streaming v4l2_streamer" << std::endl;
    }
    
    bool init_buffer_pool(uint32_t buffer_size)
    {
        v4l2_requestbuffers req = {};
    
        req.count = m_buffer_pool_size;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_USERPTR;
    
        if (-1 == xioctl(m_fd, VIDIOC_REQBUFS, &req))
        {
            std::cerr << "Failed to switch device to user pointer i/o, errno = " << errno << std::endl;
            return false;
        }

        //init buffer pool
        
        m_buffer_pool.resize(m_buffer_pool_size);
        for (int i = 0; i < m_buffer_pool_size; ++i)
        {
            m_buffer_pool[i] = std::vector<uint8_t>(buffer_size);
        }
        
        return true;
    }
    
    static int xioctl(int fh, int request, void *arg)
    {
        int r;
        
        do {
            r = ioctl(fh, request, arg);
        } while (-1 == r && EINTR == errno);
        
        return r;
    }
    
private:

    int open_camera_io(const std::string& dev_name)
    {
        struct stat st;

        if (stat(dev_name.c_str(), &st) == -1)
        {
            std::cerr << "Cannot identify '" << dev_name << "'. Error " << errno << " : " << strerror(errno) << std::endl;
            return -1;
        }

        if (!S_ISCHR(st.st_mode))
        {
            std::cerr << dev_name << " is not a device" << std::endl;
            return -1;
        }

        return open(dev_name.c_str(), O_RDWR | O_NONBLOCK, 0);
    }

    void close_camera_io()
    {
        if(m_fd > 0)
        {
            close(m_fd);
        }
    }

    bool list_yuyv_profile()
    {
        if (m_fd <= 0)
        {
            return false;
        }

        v4l2_fmtdesc fmt_desc = {0};

        fmt_desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        while (xioctl(m_fd, VIDIOC_ENUM_FMT, &fmt_desc)==0)
        {
            if(fmt_desc.pixelformat == V4L2_PIX_FMT_YUYV)
            {
                struct v4l2_frmsizeenum frmsize = {0};

                frmsize.pixel_format = fmt_desc.pixelformat;

                while (xioctl(m_fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0)
                {
                    if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
                    {
                        struct v4l2_frmivalenum frmival = {0};
                        frmival.pixel_format = frmsize.pixel_format;
                        frmival.width = frmsize.discrete.width;
                        frmival.height = frmsize.discrete.height;
                        while (xioctl(m_fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) == 0)
                        {
                            if (frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE)
                            {
                                std::cout << "found profile:" <<
                                          "\n\tPixel Format: " << fmt_desc.pixelformat <<
                                          "\n\tResolution: " << frmsize.discrete.width << "x" << frmsize.discrete.height <<
                                          "\n\tFps: " << frmival.discrete.numerator << "/" << frmival.discrete.denominator << std::endl;
                            }
                            frmival.index++;
                        }
                    }
                    frmsize.index++;
                }
            }
            fmt_desc.index++;
        }
        return false;
    }

    void streaming_proc(std::function<void(void*, v4l2_buffer, v4l2_format, std::function<void()>)> frame_callback)
    {
        //Query current video format
        v4l2_format format = {};
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if(xioctl(m_fd, VIDIOC_G_FMT, &format) < 0){
            std::cerr << "Failed ioctl operation: VIDIOC_S_FMT" << std::endl;
            return;
        }
    
        //Pre-streaming enqueue buffers
        for (unsigned int i = 0; i < m_buffer_pool.size(); ++i)
        {
            v4l2_buffer buf = {};
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_USERPTR;
            buf.index = i;
            buf.m.userptr = reinterpret_cast<unsigned long>(m_buffer_pool[i].data());
            buf.length = m_buffer_pool[i].size();
        
            if (-1 == xioctl(m_fd, VIDIOC_QBUF, &buf))
            {
                std::cerr << "Failed to enqueue buffers (VIDIOC_QBUF)" << std::endl;
                return;
            }
        }
        
        // Start streaming
        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if(xioctl(m_fd, VIDIOC_STREAMON, &type) < 0){
            std::cerr << "Failed ioctl operation: VIDIOC_STREAMON" << std::endl;
            return;
        }

        class stream_stopper{
        public:
            stream_stopper(int fd, int type):
                m_fd(fd), m_type(type) {}
            ~stream_stopper()
            {
                // Stop streaming
                if(xioctl(m_fd, VIDIOC_STREAMOFF, &m_type) < 0){
                    std::cerr << "Failed ioctl operation: VIDIOC_STREAMOFF" << std::endl;
                }
            }
        private:
            int m_fd = -1;
            int m_type;
        } raii_stream_stopper(m_fd, type);
    
        m_streaming = true;
        
        while (m_streaming)
        {
            v4l2_buffer buf = {};
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_USERPTR;
            if (-1 == xioctl(m_fd, VIDIOC_DQBUF, &buf))
            {
                if (errno == EAGAIN)
                {
                    continue;
                }
                std::cerr << "VIDIOC_DQBUF, errno = " << errno << std::endl;
                return;
            }
            
            int fd = m_fd;
            auto on_buf_free = [buf, fd]() mutable
            {
                if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                {
                    std::cerr << "Failed to restore buffer to buffer pool" << std::endl;
                }
            };
            
            frame_callback(reinterpret_cast<void*>(buf.m.userptr), buf, format, on_buf_free);
        }
        std::cout << "v4l2 streaming thread finished" << std::endl;
    }
    
    const uint32_t m_buffer_pool_size = 10;
    std::string m_device_name;
    v4l2_format m_requested_format;
    std::vector<std::vector<uint8_t>> m_buffer_pool;
    std::atomic<bool> m_streaming;
    int m_fd;
    std::future<void> m_streamingThread;
};