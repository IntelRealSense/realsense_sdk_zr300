// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once

/* standard library */
#include <vector>
#include <map>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <condition_variable>

/* realsense sdk */
#include "rs_core.h"
#include "types.h"

//opengl api
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

/**
 * @enum image_type
 * @brief image_type specifies projection related types that are used in rendering
 */
enum class image_type : int32_t
{
    depth = 0,
    color = 1,
    world = 2, /**< real world image */
    uvmap = 3,
    invuvmap = 4,

    max /**< max enum value */
};

/**
 * @class projection_viewer
 * @brief Image renderer based on GLFW and OpenGL.
 *
 * The renderer is used by projection tool to render images and points
 * synthesized by projection_interface instance.
 */
class projection_viewer
{
    projection_viewer() = delete;
public:
    /** @brief projection_viewer
     *
     * Viewer constructor.
     * @param[in] color             Color resolution.
     * @param[in] depth             Depth resolution.
     * @param[in] on_close_callback User-defined on close event callback.
     */
    projection_viewer(rs::core::sizeI32 color, rs::core::sizeI32 depth, std::function<void()> on_close_callback);

    /** @brief show_stream
     *
     * Show color, depth or world stream on main window.
     * Color image may be scaled if the resolution is more than VGA.
     * @param[in] type      Image type to specify window part on which to render the data.
     * @param[in] image     Image to render.
     */
    void show_stream(image_type type, rs::core::image_interface* image);

    /** @brief show_window
     *
     * Show popup window with color image mapped to depth or
     * depth image mapped to color.
     * Windows and images remain unscaled.
     * @param[in] image     Image to render.
     */
    void show_window(rs::core::image_interface* image);

    /** @brief image_with_drawn_points
     *
     * Get image on which the points were originally drawn by user.
     * @return:const image_type Stream type of the corresponding image.
     */
    const image_type image_with_drawn_points();

    /** @brief get_points
     *
     * Get vector of drawn points
     * @return:const std::vector<rs::core::pointF32> Vector of drawn points.
     */
    const std::vector<rs::core::pointF32> get_points() const;

    /** @brief draw_points
     *
     * Draw points from points vector on main window.
     * @param[in] type      Image to draw on.
     * @param[in] points    Points vector to draw.
     */
    void draw_points(image_type type, std::vector<rs::core::pointF32> points);

    /** @brief update
     *
     * Use the method each time when finishing render loop.
     * It is obligatory to use this method.
     * Update main window.
     * Render text message and stream name on main window.
     */
    void update();

    /** @brief get_current_max_depth_distance
     *
     * Get current maximal value of the depth distance stored as internal variable.
     * Used to change the size of the depth range interval when working with uvmap or invuvmap.
     * The value is changed via keyboard keys.
     * Possible maximal depth value is 10 meters.
     * @return: const float Currently maximal value of depth distance.
     */
    const float get_current_max_depth_distance() const;

    /** @brief is_uvmap_queried
     *
     * Check if uvmap image was requested by user.
     * @return: true        Queried.
     * @return: false       Not queried.
     */
    const bool is_uvmap_queried() const;

    /** @brief is_invuvmap_queried
     *
     * Check if inversed uvmap image was requested by user.
     * @return: true        Queried.
     * @return: false       Not queried.
     */
    const bool is_invuvmap_queried() const;

    /** @brief is_color_to_depth_queried
     *
     * Check if color image mapped to depth was requested by user.
     * @return: true        Queried.
     * @return: false       Not queried.
     */
    const bool is_color_to_depth_queried() const;

    /** @brief is_depth_to_color_queried
     *
     * Check if depth image mapped to color was requested by user.
     * @return: true        Queried.
     * @return: false       Not queried.
     */
    const bool is_depth_to_color_queried() const;

    /**
     * @brief process_user_events
     *
     * Process user events in the main thread as required by GLFW.
     * User events include mouse events (move, click), keyboard button events (click).
     */
    void process_user_events();

    /**
     * @brief terminate
     *
     * Close all windows, terminate GLFW library.
     */
    void terminate();

private:
    /** @brief mouse_click_callback
     *
     * Assigned to main window.
     * Handles user left mouse button click event.
     * Used for points drawing.
     */
    void mouse_click_callback(GLFWwindow*, int, int, int);

    /** @brief mouse_move_callback
     *
     * Assigned to main window.
     * Handles user left mouse button move event.
     * Used for points drawing.
     */
    void mouse_move_callback(GLFWwindow*, double, double);

    /** @brief key_callback
     *
     * Assigned to main and popup windows.
     * Handles keyboard button click events.
     * Used for application - user interactions via keyboard.
     */
    void key_callback(GLFWwindow*, int, int, int, int);

    /** @brief draw_texture
     *
     * Draw 2D texture by the means of OpenGL.
     * @param[in] width     Texture width.
     * @param[in] height    Texture height.
     * @param[in] gl_type   Pixel type (e.g. GL_UNSIGNED_BYTE)
     * @param[in] gl_format Pixel format (e.g. GL_RGBA)
     * @param[in] data      User data to create texture from.
     */
    void draw_texture(int width, int height, const int gl_type, const int gl_format, const void* data) const;


    bool m_continue_rendering; /**< flag to check the user intent to stop the rendering */

    std::function<void()> m_on_close_callback; /**< callback that is called on rendering stop, defined by the user */

    GLFWwindow* m_window; /**< main window */
    int m_window_width, m_window_height; /**< main window width and height */
    const int m_help_width, m_help_height; /**< help message width and height */

    std::map<rs::core::stream_type, GLFWwindow*> m_popup_windows; /**< popup windows collection */
    std::map<rs::core::stream_type, rs::core::sizeI32> m_image_resolutions; /**< image resolutions collection */
    float m_color_scale; /**< color stream scale */

    std::vector<rs::core::pointF32> m_points_vector; /**< vector of user drawn points */
    image_type m_focused_image; /**< user-focused image corresponding to image_type */

    bool m_drawing_started = false, m_drawing_finished = false, m_drawing = false; /**< drawing flags for points drawing */
    bool m_uvmap_queried = false, m_invuvmap_queried = false; /**< uvmap/invuvmap rendering flags */
    bool m_c2d_queried = false, m_d2c_queried = false; /**< color_mapped_to_depth/depth_mapped_to_color rendering flags */

    float m_curr_max_depth_distance; /**< current maximal depth distance */

    std::mutex m_render_mutex; /**< rendering mutex to synchronize method calls in async streaming */
    std::condition_variable m_rendering_cv; /**< cv to synchronize frames callback with termination */
};
