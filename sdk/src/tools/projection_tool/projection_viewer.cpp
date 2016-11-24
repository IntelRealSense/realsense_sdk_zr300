// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.


#include <iostream>
#include <algorithm>

#include "projection_viewer.h"

using namespace rs::core;
using namespace rs::utils;

namespace
{
#ifndef WIN32
// disable GCC -Wconversion warning in 3rdparty source
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
    /* text rendering using stb */
    #include "3rdparty/stb_easy_font.h"

    static void draw_text(int x, int y, const char * text)
    {
        char buffer[150000]; // buffer for characters populated in stb function
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 16, buffer);
        glDrawArrays(GL_QUADS, 0, 4*stb_easy_font_print((float)x, (float)(y-7), const_cast<char*>(text), nullptr, buffer, sizeof(buffer)));
        glDisableClientState(GL_VERTEX_ARRAY);
    }
#ifndef WIN32
#pragma GCC diagnostic pop
#endif

    /* default values */
    static const int POINT_SIZE = 4;
    static const float MAX_DEPTH_DISTANCE = 10.f; // in meters; min_depth_distance == 0.f
    static const float DISTANCE_STEP = 0.5f; // in meters
    static const int HELP_MESSAGE_WIDTH = 640, HELP_MESSAGE_HEIGHT = 480;

    static const stream_type convert(image_type type)
    {
        switch(type)
        {
        case image_type::depth: return stream_type::depth;
        case image_type::color: return stream_type::color;
        case image_type::world: return stream_type::depth;
        case image_type::uvmap: return stream_type::color;
        case image_type::invuvmap: return stream_type::depth;
        default: return stream_type::max;
        }
    }
}

projection_viewer::projection_viewer(rs::core::sizeI32 color, rs::core::sizeI32 depth, std::function<void()> on_close_callback) :
    m_help_width(HELP_MESSAGE_WIDTH),
    m_help_height(HELP_MESSAGE_HEIGHT),
    m_curr_max_depth_distance(2*DISTANCE_STEP),
    m_focused_image(image_type::max),
    m_continue_rendering(true),
    m_color_scale(1.f),
    m_on_close_callback(on_close_callback)
{
    if (color.width > m_help_width) // for color resolution bigger than VGA
    {
        // preserve the same scaling for both width and height
        m_color_scale = float(color.width) / float(m_help_width);
    }
    m_image_resolutions[stream_type::color] = { int32_t(float(color.width) / m_color_scale), int32_t(float(color.height) / m_color_scale) };
    m_image_resolutions[stream_type::depth] = { depth.width, depth.height };

    if(m_window)
    {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }
    if(m_popup_windows.begin() != m_popup_windows.end())
    {
        for (auto window : m_popup_windows)
        {
            glfwDestroyWindow(window.second);
        }
        m_popup_windows.clear();
        glfwTerminate();
    }

    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, false);
    m_window_width = std::max(m_help_width, m_image_resolutions.at(stream_type::depth).width)
            + std::max(m_image_resolutions.at(stream_type::color).width, m_image_resolutions.at(stream_type::depth).width);
    m_window_height = std::max(m_help_height, m_image_resolutions.at(stream_type::color).height)
            + m_image_resolutions.at(stream_type::depth).height;

    m_window = glfwCreateWindow(m_window_width, m_window_height, "Projection Tool", NULL, NULL);
    glfwMakeContextCurrent(m_window);
    glClear(GL_COLOR_BUFFER_BIT);
    glfwSwapBuffers(m_window);

    glfwSetErrorCallback([](int error, const char* description)
    {
        std::cerr << "\nGLFW Error code: " << error << "\nGLFW Error desc: " << description << std::endl;
    });

    std::map<stream_type, std::string> titles;
    titles[stream_type::depth] = "Depth Image Mapped To Color";
    titles[stream_type::color] = "Color Image Mapped To Depth";
    std::vector<stream_type> streams = { stream_type::depth, stream_type::color };
    for (auto stream = streams.begin(); stream != streams.end(); stream++)
    {
        sizeI32 resolution = {};
        if (*stream == stream_type::depth)
        {
            // using unscaled resolution
            resolution = { color.width, color.height }; // color image mapped to depth
        }
        if (*stream == stream_type::color)
        {
            resolution = { depth.width, depth.height }; // depth image mapped to color
        }
        glfwInit();
        glfwWindowHint(GLFW_VISIBLE, false);
        glfwWindowHint(GLFW_RESIZABLE, false);
        if (resolution.width != 0)
        {
            m_popup_windows[*stream] = glfwCreateWindow(resolution.width, resolution.height, titles.at(*stream).c_str(), NULL, NULL);
        }
    }
    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, this);
    auto key_events_callback = [] (GLFWwindow* w, int key, int scancode, int action, int mods)
    {
        static_cast<projection_viewer*>(glfwGetWindowUserPointer(w))->key_callback(w, key, scancode, action, mods);
    };
    auto focus_callback = [] (GLFWwindow* w, int focused)
    {
        if (focused == true)
        {
            glfwMakeContextCurrent(w);
        }
    };

    glfwSetKeyCallback(m_window, key_events_callback);
    glfwSetWindowFocusCallback(m_window, focus_callback);
    glfwSetMouseButtonCallback(m_window, [] (GLFWwindow* w, int button, int action, int mods)
    {
        static_cast<projection_viewer*>(glfwGetWindowUserPointer(w))->mouse_click_callback(w, button, action, mods);
    });
    glfwSetCursorPosCallback(m_window, [] (GLFWwindow* w, double x, double y)
    {
       static_cast<projection_viewer*>(glfwGetWindowUserPointer(w))->mouse_move_callback(w, x, y);
    });
    glfwSetWindowCloseCallback(m_window, [] (GLFWwindow* w)
    {
        static_cast<projection_viewer*>(glfwGetWindowUserPointer(w))->m_continue_rendering = false;
        static_cast<projection_viewer*>(glfwGetWindowUserPointer(w))->m_on_close_callback();
    });

    for (auto window : m_popup_windows)
    {
        glfwMakeContextCurrent(window.second);
        glfwSetWindowUserPointer(window.second, this);
        glfwSetKeyCallback(window.second, key_events_callback);
        glfwSetWindowFocusCallback(window.second, focus_callback);
        glfwSetWindowCloseCallback(window.second, [] (GLFWwindow* w)
        {
            auto popup_windows_alias = static_cast<projection_viewer*>(glfwGetWindowUserPointer(w))->m_popup_windows;
            auto window_pair = std::find_if(popup_windows_alias.begin(), popup_windows_alias.end(),
                                            [&w] (const std::pair<stream_type, GLFWwindow*> &pair)
                                            {
                                                return pair.second == w;
                                            });
            if (window_pair != popup_windows_alias.end())
            {
                switch(window_pair->first)
                {
                case stream_type::color:
                    static_cast<projection_viewer*>(glfwGetWindowUserPointer(w))->m_c2d_queried = false;
                    break;
                case stream_type::depth:
                    static_cast<projection_viewer*>(glfwGetWindowUserPointer(w))->m_d2c_queried = false;
                    break;
                default: break;
                }
            }
            glfwMakeContextCurrent(w);
            glClear(GL_COLOR_BUFFER_BIT);
            glfwSwapBuffers(w);
            glfwHideWindow(w);
        });
    }
    glFlush();
    glfwMakeContextCurrent(m_window);
    glClear(GL_COLOR_BUFFER_BIT);
    glfwSwapBuffers(m_window);
    glfwPollEvents();
}

void projection_viewer::mouse_click_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            m_drawing_started = true;
            m_drawing_finished = false;

            m_points_vector.clear();
            m_focused_image = image_type::max;

            bool x_y_valid = false;
            double x, y;
            glfwGetCursorPos(window, &x, &y);

            if (!(x <= m_help_width && y <= m_help_height)) // if-handling text image behaviour: text message width = 640 | height = 480
            {
                m_drawing_started = true;
                m_drawing_finished = false;
            }

            const int diff_height = m_window_height - m_image_resolutions.at(stream_type::depth).height;
            //depth
            if (x <= m_image_resolutions.at(stream_type::depth).width
                && y > diff_height
                && y <= (diff_height + m_image_resolutions.at(stream_type::depth).height))
            {
                // calculate points coordinates relative to a stream
                y -= diff_height;
                m_focused_image = image_type::depth; // select user-focused image
                x_y_valid = true;
            }
            //color
            else if ((x > m_help_width
                     && x <= (m_help_width + m_image_resolutions.at(stream_type::color).width))
                     && y > (diff_height - m_image_resolutions.at(stream_type::color).height)
                     && y <= diff_height)
            {
                // calculate points coordinates relative to a stream
                x -= m_help_width;
                y -= (diff_height - m_image_resolutions.at(stream_type::color).height);
                m_focused_image = image_type::color; // select user-focused image
                x_y_valid = true;

                // apply scaling
                x *= m_color_scale;
                y *= m_color_scale;
            }
            //world
            else if (x > m_image_resolutions.at(stream_type::depth).width
                     && x <= (m_image_resolutions.at(stream_type::depth).width + m_image_resolutions.at(stream_type::depth).width)
                     && (y > diff_height && y <= (diff_height + m_image_resolutions.at(stream_type::depth).height)))
            {
                // calculate points coordinates relative to a stream
                x -= m_image_resolutions.at(stream_type::depth).width;
                y -= diff_height;
                m_focused_image = image_type::world; // select user-focused image
                x_y_valid = true;
            }

            if (x_y_valid)
            {
                m_points_vector.push_back({float(x), float(y)});
            }
        }
        if (action == GLFW_RELEASE) // stop drawing if mouse button released
        {
            m_drawing_started = false;
            m_drawing_finished = true;
        }
        m_drawing = (m_drawing_started && !m_drawing_finished);
    }
}

void projection_viewer::mouse_move_callback(GLFWwindow* window, double x, double y)
{
    if (m_drawing) // if mouse button is pressed and not released
    {
        bool x_y_valid = false;

        const int diff_height = m_window_height - m_image_resolutions.at(stream_type::depth).height;
        //depth
        if (m_focused_image == image_type::depth
                && x <= m_image_resolutions.at(stream_type::depth).width
                && y > diff_height
                && y <= (diff_height + m_image_resolutions.at(stream_type::depth).height))
        {
            // calculate points coordinates relative to a stream
            y -= diff_height;
            x_y_valid = true;
        }
        //color
        else if (m_focused_image == image_type::color
                 && x > m_help_width
                 && x <= (m_help_width + m_image_resolutions.at(stream_type::color).width)
                 && y > (diff_height - m_image_resolutions.at(stream_type::color).height)
                 && y <= diff_height)
        {
            // calculate points coordinates relative to a stream
            x -= m_help_width;
            y -= (diff_height - m_image_resolutions.at(stream_type::color).height);
            x_y_valid = true;

            // apply scaling
            x *= m_color_scale;
            y *= m_color_scale;
        }
        //world
        else if (m_focused_image == image_type::world
                 && x > m_image_resolutions.at(stream_type::depth).width
                 && x <= (m_image_resolutions.at(stream_type::depth).width + m_image_resolutions.at(stream_type::depth).width)
                 && y > diff_height
                 && y <= (diff_height + m_image_resolutions.at(stream_type::depth).height))
        {
            // calculate points coordinates relative to a stream
            x -= m_image_resolutions.at(stream_type::depth).width;
            y -= diff_height;
            x_y_valid = true;
        }

        if (x_y_valid)
        {
            m_points_vector.push_back({float(x), float(y)});
        }
    }
}

void projection_viewer::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        switch(key)
        {
        case GLFW_KEY_ESCAPE:
        {
            m_continue_rendering = false;
            m_on_close_callback();
            break;
        }
        case GLFW_KEY_X:
            m_focused_image = image_type::max;
            m_points_vector.clear();
            break;
        case GLFW_KEY_1:
            m_uvmap_queried = !m_uvmap_queried;
            break;
        case GLFW_KEY_2:
            m_invuvmap_queried = !m_invuvmap_queried;
            break;
        case GLFW_KEY_3:
        {
            m_c2d_queried = !m_c2d_queried;
            if (!m_c2d_queried)
            {
                glfwMakeContextCurrent(m_popup_windows.at(stream_type::color));
                glfwSwapBuffers(m_popup_windows.at(stream_type::color));
                glfwHideWindow(m_popup_windows.at(stream_type::color));
            }
            break;
        }
        case GLFW_KEY_4:
        {
            m_d2c_queried = !m_d2c_queried;
            if (!m_d2c_queried)
            {
                glfwMakeContextCurrent(m_popup_windows.at(stream_type::depth));
                glfwSwapBuffers(m_popup_windows.at(stream_type::depth));
                glfwHideWindow(m_popup_windows.at(stream_type::depth));
            }
            break;
        }
        case GLFW_KEY_RIGHT:
        {
            m_curr_max_depth_distance = (m_curr_max_depth_distance >= MAX_DEPTH_DISTANCE) ? MAX_DEPTH_DISTANCE : (m_curr_max_depth_distance + DISTANCE_STEP);
            break;
        }
        case GLFW_KEY_LEFT:
        {
            m_curr_max_depth_distance = (m_curr_max_depth_distance <= 0.f) ? 0.f : (m_curr_max_depth_distance - DISTANCE_STEP);
            break;
        }
        case GLFW_KEY_Z:
        {
            m_curr_max_depth_distance = 2*DISTANCE_STEP;
            break;
        }
        default: break;
        }
    }
}

void projection_viewer::show_stream(image_type type, rs::core::image_interface* image)
{
    std::lock_guard<std::recursive_mutex> lock(m_render_mutex);
    auto stream = image->query_stream_type();
    if (m_image_resolutions.find(stream) == m_image_resolutions.end())
    {
        auto info = image->query_info();
        m_image_resolutions[stream] = { info.width, info.height };
    }

    int gl_format = GL_RGB;
    const image_interface* converted_image = nullptr;
    const image_interface* image_to_show = image;
    switch(image->query_info().format) // change pixel format upon sdk pixel_format
    {
    case pixel_format::rgb8:
        gl_format = GL_RGB;
        break;
    case pixel_format::bgr8:
        gl_format = GL_BGR_EXT;
        break;
    case pixel_format::rgba8:
        gl_format = GL_RGBA;
        break;
    case pixel_format::bgra8:
        gl_format = GL_BGRA_EXT;
        break;
    case pixel_format::z16:
        if(image->convert_to(pixel_format::rgba8, &converted_image) != status::status_no_error) return;
        image_to_show = converted_image;
        gl_format = GL_RGBA;
        break;
    default:
        throw "format is not supported";
    }
    auto converted_image_releaser = get_unique_ptr_with_releaser(converted_image);

    bool apply_scaling = false;
    int position_x = 0, position_y = 0;
    switch(type)
    {
    case image_type::depth:
        break;
    case image_type::color:
        position_x = m_help_width;
        position_y = m_image_resolutions.at(stream_type::depth).height;
        if (m_color_scale != 1.f) apply_scaling = true;
        break;
    case image_type::world:
        position_x = m_image_resolutions.at(stream_type::depth).width;
        break;
    default: break;
    }

    auto info = image->query_info();
    int width = info.width;
    int height = info.height;

    // drawing
    glfwMakeContextCurrent(m_window);

    if (apply_scaling)
    {
        glViewport(position_x, position_y, int(float(width) / m_color_scale), int(float(height) / m_color_scale));
    }
    else
    {
        glViewport(position_x, position_y, width, height);
    }
    draw_texture(width, height, GL_UNSIGNED_BYTE, gl_format, image_to_show->query_data());
}

void projection_viewer::show_window(image_interface* image)
{
    std::lock_guard<std::recursive_mutex> lock(m_render_mutex);
    if (!image) return;
    int gl_format = GL_RGB;
    auto info = image->query_info();
    const image_interface* converted_image = nullptr;
    const image_interface* image_to_show = image;
    switch(info.format) // change pixel format upon sdk pixel_format
    {
    case pixel_format::rgb8:
        gl_format = GL_RGB;
        break;
    case pixel_format::rgba8:
        gl_format = GL_RGBA;
        break;
    case pixel_format::bgr8:
        gl_format = GL_BGR_EXT;
        break;
    case pixel_format::bgra8:
        gl_format = GL_BGRA_EXT;
        break;
    case pixel_format::z16:
        if(image->convert_to(pixel_format::rgba8, &converted_image) != status::status_no_error) return;
        image_to_show = converted_image;
        gl_format = GL_RGBA;
        break;
    default:
        throw "format is not supported";
    }
    auto converted_image_releaser = get_unique_ptr_with_releaser(converted_image);

    auto stream = image->query_stream_type();
    bool show_window = false;
    if (stream == rs::core::stream_type::color)
        show_window = m_c2d_queried;
    if (stream == rs::core::stream_type::depth)
        show_window = m_d2c_queried;
    auto width = info.width;
    auto height = info.height;

    // drawing
    glfwMakeContextCurrent(m_popup_windows.at(stream));

    glViewport(0, 0, width, height);
    draw_texture(width, height, GL_UNSIGNED_BYTE, gl_format, image_to_show->query_data());
    glfwSwapBuffers(m_popup_windows.at(stream));
    glClear(GL_COLOR_BUFFER_BIT);
    if (!glfwGetWindowAttrib(m_popup_windows.at(stream), GLFW_VISIBLE) && show_window)
    {
        glfwShowWindow(m_popup_windows.at(stream));
    }
}

void projection_viewer::draw_texture(int width, int height, const int gl_type, const int gl_format, const void* data) const
{
    glLoadIdentity();
    glMatrixMode(GL_MATRIX_MODE);
    glPushMatrix();
    glOrtho(0, width, height, 0, -1, +1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, gl_format, gl_type, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(0, 0);
    glTexCoord2f(0, 1); glVertex2f(0, float(height));
    glTexCoord2f(1, 1); glVertex2f(float(width), float(height));
    glTexCoord2f(1, 0); glVertex2f(float(width), 0);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    glPopMatrix();
}

void projection_viewer::draw_points(image_type type, std::vector<pointF32> points)
{
    std::lock_guard<std::recursive_mutex> lock(m_render_mutex);
    std::vector<float> rgb; // color components
    bool valid_points = true;
    float point_size = POINT_SIZE;
    int window_x = 0, window_y = 0;
    switch(type)
    {
    case image_type::depth: // depth stream - main window
        rgb = { 0.7f, 0.f, 0.5f }; // purple
        break;
    case image_type::color: // color stream - main window
        rgb = { 1.f, 0.f, 0.f }; // red
        window_x = m_help_width;
        window_y = m_image_resolutions.at(stream_type::depth).height;
        for (int i = 0, size = int(points.size()); i < size; i++)
        {
            points[i].x /= m_color_scale;
            points[i].y /= m_color_scale;
        }
        break;
    case image_type::world: // world stream - main window
        rgb = { 1.f, 0.7f, 0.f }; // yellow
        window_x = m_image_resolutions.at(stream_type::depth).width;
        break;
    case image_type::uvmap: // uvmap
        type = image_type::color;
        rgb = { 0.f, 1.f, 0.f }; // green
        point_size /= 2;
        window_x = m_help_width;
        window_y = m_image_resolutions.at(stream_type::depth).height;
        for (int i = 0, size = int(points.size()); i < size; i++)
        {
            points[i].x /= m_color_scale;
            points[i].y /= m_color_scale;
        }
        break;
    case image_type::invuvmap: // invuvmap
        type = image_type::depth;
        rgb = { 1.f, 0.7f, 0.f }; // yellow
        point_size /= 2;
        break;
    default:
        valid_points = false;
        break;
    }
    if (valid_points)
    {
        const int window_width = m_image_resolutions.at(convert(type)).width;
        const int window_height = m_image_resolutions.at(convert(type)).height;

        // drawing
        glfwMakeContextCurrent(m_window);
        glViewport(window_x, window_y, window_width, window_height);

        glLoadIdentity();
        glMatrixMode(GL_MATRIX_MODE);
        glPushMatrix();
        glOrtho(0, window_width, window_height, 0, -1, +1);

        glPointSize(point_size);
        glColor3f(rgb[0], rgb[1], rgb[2]);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, points.data());
        glDrawArrays(GL_POINTS, 0, GLsizei(points.size()));
        glDisableClientState(GL_VERTEX_ARRAY);

        glPopMatrix();
        glColor3f(1.f, 1.f, 1.f);
    }
}

void projection_viewer::update()
{
    std::lock_guard<std::recursive_mutex> lock(m_render_mutex);
    glfwMakeContextCurrent(m_window);

    glViewport(0, m_image_resolutions.at(stream_type::depth).height, m_help_width, m_help_height);
    const int scaled_ortho_w = 320, scaled_ortho_h = 250;
    glOrtho(0, scaled_ortho_w, scaled_ortho_h, 0, -1, +1);
    glPixelZoom(1, -1);

    glColor3f(1.f,1.f,1.f);
    glRecti(0, 0, m_help_width, m_help_height);

    // help message
    std::ostringstream ss;
    ss << "SHOW/HIDE basic projection calculations:\n"
       << "  Press 1: show/hide points from UVMap\n"
       << "  Press 2: show/hide points from InvUVMap\n"
       << "  Press 3: show/hide Color Image Mapped to Depth\n"
       << "  Press 4: show/hide Depth Image Mapped to Color\n"
       << "\nDEPTH INTERVAL: 0 - " << MAX_DEPTH_DISTANCE << " meters\n"
       << "  Current depth range: 0 - " << m_curr_max_depth_distance << " meters\n"
       << "    To modify depth range use arrow keys ( <- and -> )\n"
       << "    To reset to default range press Z\n"
       << "\nDRAWING:\n"
       << "  To draw points hold down LEFT MOUSE BUTTON\n"
       << "    Mapped points are also shown\n"
       << "  To clear images press X\n"
       << "\nCONSOLE:\n"
       << "  To show command line HELP\n    run the tool with -help option\n";
    glColor3f(0.f, 0.f, 1.f);
    const int x_offset = 10, y_offset = 15;
    draw_text(x_offset, y_offset, ss.str().c_str());
    ss.clear(); ss.str("");


    // stream descriptions
    // depth
    glViewport(0, 0,
               m_image_resolutions.at(stream_type::depth).width, m_image_resolutions.at(stream_type::depth).height);
    ss << "DEPTH";
    glColor3f(0.7f, 0.0f, 0.5f);
    draw_text(x_offset, y_offset, ss.str().c_str());
    ss.clear(); ss.str("");

    // color
    glViewport(m_help_width, m_image_resolutions.at(stream_type::depth).height,
               m_image_resolutions.at(stream_type::color).width, m_image_resolutions.at(stream_type::color).height);
    if (m_color_scale != 1.f) ss << "SCALED COLOR";
    else ss << "COLOR";
    glColor3f(1.f, 0.f, 0.f);
    draw_text(x_offset, y_offset, ss.str().c_str());
    ss.clear(); ss.str("");

    // world
    glViewport(m_image_resolutions.at(stream_type::depth).width, 0,
               m_image_resolutions.at(stream_type::depth).width, m_image_resolutions.at(stream_type::depth).height);
    ss << "WORLD";
    glColor3f(1.f, 0.7f, 0.f);
    draw_text(x_offset, y_offset, ss.str().c_str());
    ss.clear(); ss.str("");

    glColor3f(1.f, 1.f, 1.f);
    glfwSwapBuffers(m_window);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!m_continue_rendering && m_on_close_callback)
    {
        for (auto window : m_popup_windows)
        {
            glfwDestroyWindow(window.second);
        }
        glfwDestroyWindow(m_window);
        glfwTerminate();
        m_on_close_callback();
    }
}

const std::vector<pointF32> projection_viewer::get_points() const
{
    return m_points_vector;
}

const image_type projection_viewer::image_with_drawn_points()
{
    if (m_points_vector.empty())
    {
        m_focused_image = image_type::max;
    }
    return m_focused_image;
}


const bool projection_viewer::is_uvmap_queried() const
{
    return m_uvmap_queried;
}

const bool projection_viewer::is_invuvmap_queried() const
{
    return m_invuvmap_queried;
}

const bool projection_viewer::is_color_to_depth_queried() const
{
    return m_c2d_queried;
}

const bool projection_viewer::is_depth_to_color_queried() const
{
    return m_d2c_queried;
}

const float projection_viewer::get_current_max_depth_distance() const
{
    return m_curr_max_depth_distance;
}

void projection_viewer::process_user_events()
{
    std::lock_guard<std::recursive_mutex> lock(m_render_mutex);
    glfwPollEvents();
}
