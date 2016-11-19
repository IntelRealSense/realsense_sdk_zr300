#pragma once

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <future>
#include <iostream>

class v4l_streamer
{
public:

    void start_streaming(std::function<void(void*, v4l2_buffer, v4l2_format)> frame_callback)
    {
        if(frame_callback == nullptr)
        {
            throw std::invalid_argument("nullptr frame_callback");
        }

        const std::string dev_name("/dev/video0");

        m_fd = open_camera_io(dev_name);

        if(m_fd < 0)
        {
            throw std::runtime_error(dev_name + " has failed to open");
        }

        v4l2_capability cap = {};
        if(xioctl(m_fd, VIDIOC_QUERYCAP, &cap) < 0)
        {
            if(errno == EINVAL)
            {
                throw std::runtime_error(dev_name + " is no V4L2 device");
            }
            else std::runtime_error("VIDIOC_QUERYCAP");
        }
        if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) throw std::runtime_error(dev_name + " is no video capture device");
        if(!(cap.capabilities & V4L2_CAP_STREAMING)) throw std::runtime_error(dev_name + " does not support streaming I/O");
    
        if(find_and_set_yuyv_profile() == false)
        {
            throw std::runtime_error(dev_name + " cannot stream with YUYV (VGA or higher)");
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
    
    int set_format(int width, int height, uint32_t v4l_format, uint32_t field)
    {
        
        struct v4l2_format format = {};
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        format.fmt.pix.pixelformat = v4l_format;
        format.fmt.pix.width = width;
        format.fmt.pix.height = height;
        format.fmt.pix.field = field;
        return xioctl(m_fd, VIDIOC_S_FMT, &format);
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

        return open(dev_name.c_str(), O_RDWR, 0);
    }

    void close_camera_io()
    {
        if(m_fd > 0)
        {
            close(m_fd);
        }
    }

    bool find_and_set_yuyv_profile()
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
//                                std::cout << "found profile:\n" <<
//                                          pixel_format_to_string((rs::format)convert_to_rs_format(fmt_desc.pixelformat)) << " " <<
//                                          frmsize.discrete.width << "x" <<
//                                          frmsize.discrete.height << ", Fps: " <<
//                                          frmival.discrete.numerator << "/" <<
//                                          frmival.discrete.denominator << std::endl;

                                if(frmsize.discrete.width >= 640 && frmsize.discrete.height >= 480 && frmival.discrete.denominator >= 30)
                                {
                                    if(set_format(640, 480, V4L2_PIX_FMT_YUYV, V4L2_FIELD_INTERLACED) < 0)
                                        continue;
                                    
                                    return true;
                                    
                                }
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

    void streaming_proc(std::function<void(void*, v4l2_buffer, v4l2_format)> frame_callback)
    {
        m_streaming = true;
        struct v4l2_requestbuffers bufrequest;
        bufrequest.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        bufrequest.memory = V4L2_MEMORY_MMAP;
        bufrequest.count = 1;

        if(xioctl(m_fd, VIDIOC_REQBUFS, &bufrequest) < 0){
            perror("VIDIOC_REQBUFS");
            return ;
        }

        v4l2_buffer buffer = {};
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = 0;

        if(xioctl(m_fd, VIDIOC_QUERYBUF, &buffer) < 0){
            perror("VIDIOC_QUERYBUF");
            return ;
        }

        //Only using 1 buffer so no need to iterate for bufrequest.count
        void* buffer_start = mmap(
            NULL,
            buffer.length,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            m_fd,
            buffer.m.offset
        );

        if(buffer_start == MAP_FAILED){
            std::cerr << "Failed to map memory using mmmap" << std::endl;
            return;
        }

        memset(buffer_start, 0, buffer.length);
        memset(&buffer, 0, sizeof(buffer));

        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = 0;

        v4l2_format format = {};
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if(xioctl(m_fd, VIDIOC_G_FMT, &format) < 0){
            std::cerr << "Failed ioctl operation: VIDIOC_S_FMT" << std::endl;
            return;
        }

        // Start streaming
        int type = buffer.type;
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
        } raii_stream_stopper(m_fd, buffer.type);

        while (m_streaming)
        {
            for(int i = 0; i < bufrequest.count; i++) //redundant since bufrequest.count is 1
            {
                buffer.index = i;

                //queue buffer
                if(xioctl(m_fd, VIDIOC_QBUF, &buffer) < 0){
                    std::cerr << "Failed ioctl operation: VIDIOC_QBUF" << std::endl;
                    return;
                }

                //dequeue buffer
                if(xioctl(m_fd, VIDIOC_DQBUF, &buffer) < 0){
                    std::cerr << "Failed ioctl operation: VIDIOC_DQBUF" << std::endl;
                    return;
                }

                frame_callback(buffer_start, buffer, format);
            }
        }
        std::cout << "v4l2 streaming thread finished" << std::endl;
    }
    
    std::atomic<bool> m_streaming;
    int m_fd = 0;
    std::future<void> m_streamingThread;
};