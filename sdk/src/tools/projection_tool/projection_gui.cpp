// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "projection_gui.h"

/* standard library */
#include <iostream>

/* public methods */
projection_gui::projection_gui(int d_width, int d_height, int c_width, int c_height) :
    m_depth_width(d_width),
    m_depth_height(d_height),
    m_color_width(c_width),
    m_color_height(c_height)
{
    m_text_image = cv::Mat::zeros(480, 640, CV_8UC4);
    m_text_image.setTo(cv::Scalar(255, 255, 255));
    cv::putText(m_text_image, m_main_window_name, cv::Point(200, 20), CV_FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(0, 0, 255));
    cv::putText(m_text_image, "SHOW/HIDE basic projection images:", cv::Point(10, 60), CV_FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(0, 0, 255));
    cv::putText(m_text_image, "Press 4: show/hide UVMap", cv::Point(20, 80), CV_FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(0, 0, 255));
    cv::putText(m_text_image, "Press 5: show/hide InversedUVMap", cv::Point(20, 100), CV_FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(0, 0, 255));
    cv::putText(m_text_image, "DRAWING:", cv::Point(10, 130), CV_FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(0, 0, 255));
    cv::putText(m_text_image, "To draw points hold down LEFT MOUSE BUTTON", cv::Point(20, 150), CV_FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(0, 0, 255));
    cv::putText(m_text_image, "To clear images press X", cv::Point(20, 170), CV_FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(0, 0, 255));
    cv::putText(m_text_image, "To close application press ESC", cv::Point(10, 200), CV_FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(0, 0, 255));
    cv::putText(m_text_image, "CONSOLE:", cv::Point(10, 230), CV_FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(0, 0, 255));
    cv::putText(m_text_image, "To show HELP run the tool with -help option", cv::Point(20, 250), CV_FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(0, 0, 255));
    cv::putText(m_text_image, "To use PLAYBACK run the tool with -file option", cv::Point(20, 270), CV_FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(0, 0, 255));
    cv::putText(m_text_image, "To change DEPTH resolution use -depth option", cv::Point(20, 290), CV_FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(0, 0, 255));
    cv::putText(m_text_image, "To change COLOR resolution use -color option", cv::Point(20, 310), CV_FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(0, 0, 255));
}

int projection_gui::create_image(const void* raw_data, image_type image, const int type)
{
    //only color or depth image
    switch(type)
    {
        case CV_8UC1:
            if (image == image_type::world)
            {
                m_world_image = cv::Mat(m_depth_height, m_depth_width, type, const_cast<void*>(raw_data)).clone();
                if (m_world_image.empty())
                {
                    std::cerr << "World image is empty" << std::endl;
                    return -1;
                }
            }
            break;
        case CV_8UC4:
            if (image == image_type::color)
            {
                m_color_image = cv::Mat(m_color_height, m_color_width, type, const_cast<void*>(raw_data)).clone();
                if (m_color_image.empty())
                {
                    std::cerr << "Color image is empty" << std::endl;
                    return -1;
                }
            }
            if (image == image_type::uvmap)
            {
                m_uvmap_image = cv::Mat(m_depth_height, m_depth_width, type, const_cast<void*>(raw_data)).clone();
                m_uvmap_set = true;
                if (m_uvmap_image.empty())
                {
                    std::cerr << "UVMap image is empty" << std::endl;
                    return -1;
                }
            }
            break;
        case CV_16UC1:
            if (image == image_type::depth)
            {
                m_depth_image = cv::Mat(m_depth_height, m_depth_width, type, const_cast<void*>(raw_data)).clone();
                if (m_depth_image.empty())
                {
                    std::cerr << "Depth image is empty" << std::endl;
                    return -1;
                }
            }
            if (image == image_type::invuvmap)
            {
                m_invuvmap_image = cv::Mat(m_color_height, m_color_width, type, const_cast<void*>(raw_data)).clone();
                m_invuvmap_set = true;
                if (m_invuvmap_image.empty())
                {
                    std::cerr << "Inversed UVMap image is empty" << std::endl;
                    return -1;
                }
            }
            break;
        default:
            std::cerr << "Unable to create image" << std::endl;
            return -1;
    }
    return 0;
}

void projection_gui::convert_to_visualized_images()
{
    // depth image
    for (int i = 0; i < m_depth_image.rows; i++)
    {
        for (int j = 0; j < m_depth_image.cols; j++)
        {
            m_depth_image.at<uint16_t>(i, j) *= 32;
        }
    }
    double depth_min_value, depth_max_value;
    cv::minMaxLoc(m_depth_image, &depth_min_value, &depth_max_value);
    cv::cvtColor(m_depth_image, m_depth_image, CV_GRAY2BGRA);
    m_depth_image.convertTo(m_depth_image, m_color_image.type(), 255.0 / (depth_max_value - depth_min_value), -depth_min_value * 255.0 / (depth_max_value - depth_min_value));

    // world image
    cvtColor(m_world_image, m_world_image, CV_GRAY2BGRA);
    m_world_image.convertTo(m_world_image, m_color_image.type());

    // invuvmap image
    if (m_invuvmap_set)
    {
        for (int i = 0; i < m_invuvmap_image.rows; i++)
        {
            for (int j = 0; j < m_invuvmap_image.cols; j++)
            {
                m_invuvmap_image.at<uint16_t>(i, j) *= 32;
            }
        }
        double invuvmap_min_value, invuvmap_max_value;
        cv::minMaxLoc(m_invuvmap_image, &invuvmap_min_value, &invuvmap_max_value);
        cv::cvtColor(m_invuvmap_image, m_invuvmap_image, CV_GRAY2BGRA);
        m_invuvmap_image.convertTo(m_invuvmap_image, m_color_image.type(), 255.0 / (invuvmap_max_value - invuvmap_min_value), -invuvmap_min_value * 255.0 / (invuvmap_max_value - invuvmap_min_value));
    }

    // adding text messages
    cv::putText(m_color_image, "COLOR", cv::Point(5, 20), CV_FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(255, 0, 0));
    cv::putText(m_depth_image, "DEPTH", cv::Point(5, 20), CV_FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(0, 255, 0));
    cv::putText(m_world_image, "WORLD", cv::Point(5, 20), CV_FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(0, 0, 255));
}

void projection_gui::draw_points(image_type image, float x, float y)
{
    switch (image)
    {
        case image_type::color:
            cv::circle(m_color_image, cv::Point2f(x, y), 3, CV_RGB(255, 0, 0), -1);
            break;
        case image_type::depth:
            cv::circle(m_depth_image, cv::Point2f(x, y), 3, CV_RGB(0, 255, 0), -1);
            break;
        case image_type::world:
            cv::circle(m_world_image, cv::Point2f(x, y), 3, CV_RGB(0, 0, 255), -1);
            break;
        default: break;
    }
}

bool projection_gui::show_window()
{
    static uint key_pressed = 0;
    create_window();
    cv::imshow(m_main_window_name, m_window_image);
    cv::setMouseCallback(m_main_window_name, mouse_callback_wrapper, this);

    /* processing keyboard events */
    switch(key_pressed)
    {
        case 27: // 52 equals ESC on a keyboard
            cv::destroyAllWindows();
            return false;
        case 52:// 52 equals 4 on a keyboard
            if (!m_uvmap_queried)
            {
                m_uvmap_queried = true;
            }
            else
            {
                m_uvmap_queried = false;
            }
            break;
        case 53: // 53 equals 5 on a keyboard
            if (!m_invuvmap_queried)
            {
                m_invuvmap_queried = true;
            }
            else
            {
                m_invuvmap_queried = false;
            }
            break;
        case 120: // 120 equals X on a keyboard
            m_no_drawing = true;
            m_focused_image = image_type::any;
            m_points_vector.clear();
            break;
        default: break;
    }
    if (m_uvmap_set)
    {
        cv::imshow(m_uvmap_window_name, m_uvmap_image);
    }
    else
    {
        cv::destroyWindow(m_uvmap_window_name);
    }
    if (m_invuvmap_set)
    {
        cv::imshow(m_invuvmap_window_name, m_invuvmap_image);
    }
    else
    {
        cv::destroyWindow(m_invuvmap_window_name);
    }
    /* waiting for key events and finishing drawing routine */
    key_pressed = cv::waitKey(1) & 255;
    /* checking if window is closed with 'X' */
    if (!cvGetWindowHandle(m_main_window_name.c_str()))
    {
        key_pressed = 27;
    }
    if (m_uvmap_queried && m_uvmap_set)
    {
        if (!cvGetWindowHandle(m_uvmap_window_name.c_str()))
        {
            cv::destroyWindow(m_uvmap_window_name);
            m_uvmap_queried = false;
        }
    }
    if (m_invuvmap_queried && m_invuvmap_set)
    {
        if (!cvGetWindowHandle(m_invuvmap_window_name.c_str()))
        {
            cv::destroyWindow(m_invuvmap_window_name);
            m_invuvmap_queried = false;
        }
    }
    /* cleaning the flags */
    m_uvmap_set = false;
    m_invuvmap_set = false;
    return true;
}

image_type projection_gui::image_with_drawn_points()
{
    return m_focused_image;
}

std::vector<rs::core::pointI32> projection_gui::get_points()
{
    return m_points_vector;
}

bool projection_gui::is_uvmap_queried()
{
    return m_uvmap_queried;
}

bool projection_gui::is_invuvmap_queried()
{
    return m_invuvmap_queried;
}


/* private methods */
void projection_gui::create_window()
{
    int margin = 10;
    int window_height = m_depth_height + std::max(m_text_image.rows, m_color_height) + 2 * margin;
    int window_width  = std::max(m_text_image.cols, m_depth_width) + std::max(m_depth_width, m_color_width) + 2 * margin;
    std::vector<cv::Mat> mat_vector;
    mat_vector.push_back(m_text_image);
    mat_vector.push_back(m_color_image);
    mat_vector.push_back(m_depth_image);
    mat_vector.push_back(m_world_image);

    const int nRowsCols = 2, vsize = mat_vector.size();
    cv::Mat whole_image = cv::Mat::zeros(window_height, window_width, CV_8UC4);

    int x_max = std::max(m_depth_width, m_text_image.cols); // text message width = 640
    int y_max = std::max(m_text_image.rows, m_color_height); // text message height = 480
    for (int i = 0, k = 0; i < nRowsCols; i++)
    {
        int y = margin + i * y_max;
        for (int j = 0; j < nRowsCols && k < vsize; j++, k++)
        {
            int x = margin + j * x_max;
            cv::Rect roi(x, y, mat_vector[k].cols, mat_vector[k].rows); // make a rectangle of current Mat size
            mat_vector[k].copyTo(whole_image(roi));
        }
    }
    whole_image.copyTo(m_window_image);
}

void projection_gui::mouse_callback_wrapper(int event, int x, int y, int flags, void* userdata)
{
    projection_gui* self = reinterpret_cast<projection_gui*>(userdata);
    self->mouse_callback(event, x, y, flags, 0);
}

void projection_gui::mouse_callback(int event, int x, int y, int flags, void* userdata)
{
    const int margin = 10;
    int x_max = std::max(m_depth_width, m_text_image.cols); // text message width = 640
    int y_max = std::max(m_text_image.rows, m_color_height); // text message height = 480
    if (event == cv::EVENT_LBUTTONDOWN)
    {
        m_points_vector.clear();
        m_focused_image = image_type::any;
        bool x_y_valid = false;
        if (!(x <= (margin + m_text_image.cols) && y <= (margin + m_text_image.rows))) // if-handling text image behaviour: text message width = 640 | height = 480
        {
            m_no_drawing = false;
            m_drawing_started = true;
            m_drawing_finished = false;
        }
        // depth
        if ((x > margin && x <= (margin + m_depth_width)) && (y > (margin + y_max) && y <= (margin + y_max + m_depth_height)))
        {
            x -= margin;
            y -= (margin + y_max);
            m_focused_image = image_type::depth;
            x_y_valid = true;
        }
        // color
        if ((x > (margin + x_max) && x <= (x_max + margin + m_color_width)) && (y > margin && y <= (margin + m_color_height)))
        {
            x -= (margin + x_max);
            y -= margin;
            m_focused_image = image_type::color;
            x_y_valid = true;
        }
        // world
        if ((x > (margin + x_max) && x <= (x_max + margin + m_depth_width)) && (y > (margin + y_max) && y <= (margin + y_max + m_depth_height)))
        {
            x -= (margin + x_max);
            y -= (margin + y_max);
            m_focused_image = image_type::world;
            x_y_valid = true;
        }
        if (x_y_valid)
            m_points_vector.push_back({x, y});
    }
    else if (event == cv::EVENT_MOUSEMOVE)
    {
        if (m_drawing)
        {
            bool x_y_valid = false;
            //depth
            if ((m_focused_image == image_type::depth) && (x > margin && x <= (margin + m_depth_width)) && (y > (margin + y_max) && y <= (margin + y_max + m_depth_height)))
            {
                x -= margin;
                y -= (margin + y_max);
                x_y_valid = true;
            }
            //color
            if ((m_focused_image == image_type::color) && ((x > (margin + x_max) && x <= (x_max + margin + m_color_width)) && (y > margin && y <= (margin + m_color_height))))
            {
                x -= (margin + x_max);
                y -= margin;
                x_y_valid = true;
            }
            //world
            if ((m_focused_image == image_type::world) && (x > (margin + x_max) && x <= (x_max + margin + m_depth_width)) && (y > (margin + y_max) && y <= (margin + y_max + m_depth_height)))
            {
                x -= (margin + x_max);
                y -= (margin + y_max);
                x_y_valid = true;
            }
            if (x_y_valid)
                m_points_vector.push_back({x, y});
        }
    }
    else if (event == cv::EVENT_LBUTTONUP)
    {
        m_drawing_started = false;
        m_drawing_finished = true;
    }
    m_drawing = (m_drawing_started && !m_drawing_finished);
}
