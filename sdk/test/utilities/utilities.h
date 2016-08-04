// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs/core/image_interface.h"
#include "rs/utils/librealsense_conversion_utils.h"
#include "image/image_utils.h"
#include "librealsense/rs.hpp"
//opengl api
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#include <chrono>
#include <thread>
#include <vector>

namespace glutils
{
    static void gl_close(GLFWwindow * window)
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    static inline void make_depth_histogram(std::vector<uint8_t>& rgb_image, const uint16_t depth_image[])
    {
        static uint32_t histogram[0x10000];
        memset(histogram, 0, sizeof(histogram));
        auto size = rgb_image.size() / 3;
        for(size_t i = 0; i < size; ++i) ++histogram[depth_image[i]];
        for(size_t i = 2; i < 0x10000; ++i) histogram[i] += histogram[i-1]; // Build a cumulative histogram for the indices in [1,0xFFFF]
        for(size_t i = 0; i < size; ++i)
        {
            if(uint16_t d = depth_image[i])
            {
                int f = histogram[d] * 255 / histogram[0xFFFF]; // 0-255 based on histogram location
                rgb_image[i*3 + 0] = 255 - f;
                rgb_image[i*3 + 1] = 0;
                rgb_image[i*3 + 2] = f;
            }
            else
            {
                rgb_image[i*3 + 0] = 20;
                rgb_image[i*3 + 1] = 5;
                rgb_image[i*3 + 2] = 0;
            }
        }
    }

    static void gl_render(GLFWwindow * window, rs::frame& frame)
    {
        std::vector<uint8_t> rgb;
        GLuint texture = 0;
        auto width = frame.get_width();
        auto height = frame.get_height();
        const void * imageDataToDisplay = frame.get_data();
        if(!imageDataToDisplay)return;
        rgb.resize(width * height * 3);
        auto format = frame.get_format();
        int gl_color_format = GL_NONE;
        switch(format)
        {
            case rs::format::rgb8:
            case rs::format::bgr8: gl_color_format = GL_RGB; break;
            case rs::format::rgba8:
            case rs::format::bgra8: gl_color_format = GL_RGBA; break;
            default: break;
        }

        glPushMatrix();
        glOrtho(0, width, height, 0, -1, +1);

        if(frame.get_stream_type() != rs::stream::color)
        {
            make_depth_histogram(rgb,reinterpret_cast<const uint16_t *>(imageDataToDisplay));
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
        }
        else
            glTexImage2D(GL_TEXTURE_2D, 0, gl_color_format,width ,height , 0, gl_color_format, GL_UNSIGNED_BYTE, imageDataToDisplay);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, texture);
        glEnable(GL_TEXTURE_2D);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(0, 0);
        glTexCoord2f(0, 1); glVertex2f(0, height);
        glTexCoord2f(1, 1); glVertex2f(width, height);
        glTexCoord2f(1, 0); glVertex2f(width, 0);
        glEnd();
        glDisable(GL_TEXTURE_2D);
        glPopMatrix();
        glfwSwapBuffers(window);
        glfwPollEvents();
        //std::this_thread::sleep_for (std::chrono::milliseconds(15));
    }

    static void gl_render(GLFWwindow * window, rs::device* device, rs::stream stream)
    {
        std::vector<uint8_t> rgb;
        GLuint texture = 0;
        auto width = device->get_stream_width(stream);
        auto height = device->get_stream_height(stream);
        const void * imageDataToDisplay = device->get_frame_data(stream);
        if(!imageDataToDisplay)return;
        rgb.resize(width * height * 3);
        auto format = device->get_stream_format(stream);
        int gl_color_format = GL_NONE;
        switch(format)
        {
            case rs::format::rgb8:
            case rs::format::bgr8: gl_color_format = GL_RGB; break;
            case rs::format::rgba8:
            case rs::format::bgra8: gl_color_format = GL_RGBA; break;
            default: break;
        }

        glPushMatrix();
        glOrtho(0, width, height, 0, -1, +1);
        if(stream == rs::stream::depth)
        {
            make_depth_histogram(rgb,reinterpret_cast<const uint16_t *>(imageDataToDisplay));
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
        }
        else
            glTexImage2D(GL_TEXTURE_2D, 0, gl_color_format,width ,height , 0, gl_color_format, GL_UNSIGNED_BYTE, imageDataToDisplay);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, texture);
        glEnable(GL_TEXTURE_2D);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(0, 0);
        glTexCoord2f(0, 1); glVertex2f(0, height);
        glTexCoord2f(1, 1); glVertex2f(width, height);
        glTexCoord2f(1, 0); glVertex2f(width, 0);
        glEnd();
        glDisable(GL_TEXTURE_2D);
        glPopMatrix();
        glfwSwapBuffers(window);
        glfwPollEvents();
        //std::this_thread::sleep_for (std::chrono::milliseconds(15));
    }

    static void display_image(const rs::core::image_interface * image, std::string title)
    {
        auto info = image->query_info();

        int gl_internal_format = GL_RGB, gl_format, gl_pixel_size;
        switch(rs::utils::convert_pixel_format(info.format))
        {
            case rs::format::rgb8:
                gl_format = GL_RGB;
                gl_pixel_size = GL_UNSIGNED_BYTE;
                break;
            case rs::format::bgr8:
                gl_format = GL_BGR;
                gl_pixel_size = GL_UNSIGNED_BYTE;
                break;
            case rs::format::yuyv:
                gl_format = GL_LUMINANCE_ALPHA;
                gl_pixel_size = GL_UNSIGNED_BYTE;
                break;
            case rs::format::rgba8:
                gl_format = GL_RGBA;
                gl_pixel_size = GL_UNSIGNED_BYTE;
                break;
            case rs::format::bgra8:
                gl_format = GL_BGRA;
                gl_pixel_size = GL_UNSIGNED_BYTE;
                break;
            case rs::format::y8:
                gl_format = GL_LUMINANCE;
                gl_pixel_size = GL_UNSIGNED_BYTE;
                break;
            default:
                throw "no support format";
        }

        int windowWidth = info.width, windowHeight = info.height;
        const void * imageDataToDisplay = image->query_data();
        glfwInit();
        GLFWwindow * window = glfwCreateWindow(windowWidth, windowHeight, title.c_str(), nullptr, nullptr);
        glfwMakeContextCurrent(window);
        GLuint texture = 0;
        for(int i = 0; !(glfwWindowShouldClose(window) || i > 100); i++)
        {
            glPushMatrix();
            glOrtho(0, windowWidth, windowHeight, 0, -1, +1);
            glTexImage2D(GL_TEXTURE_2D, 0, gl_internal_format, windowWidth, windowHeight, 0, gl_format , gl_pixel_size, imageDataToDisplay);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glBindTexture(GL_TEXTURE_2D, texture);
            glEnable(GL_TEXTURE_2D);
            glBegin(GL_QUADS);
            glTexCoord2f(0, 0); glVertex2f(0, 0);
            glTexCoord2f(0, 1); glVertex2f(0, windowHeight);
            glTexCoord2f(1, 1); glVertex2f(windowWidth, windowHeight);
            glTexCoord2f(1, 0); glVertex2f(windowWidth, 0);
            glEnd();
            glDisable(GL_TEXTURE_2D);
            glPopMatrix();

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
        glfwDestroyWindow(window);
        glfwTerminate();
    }
}


