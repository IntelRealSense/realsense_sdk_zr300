// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once

/* standard library */
#include <set>
#include <vector>
#include <map>
#include "stdint.h"

/* intel realsense */
#include "rs/core/types.h"

/* opencv */
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

/**
 * @brief enum image_type
 * Specifies image types that are used in tool
 */
enum class image_type : int32_t
{
    any         = 0,            /**< used for invalid or unknown image type */
    text        = 1,            /**< used to specify text image type */
    depth       = 2,            /**< used to specify depth image type */
    color       = 3,            /**< used to specify color image type */
    world       = 4,            /**< used to specify world image type */
    uvmap       = 5,            /**< used to specify uvmap image type */
    invuvmap    = 6,            /**< used to specify inversed uvmap image type */
    color2depth = 7,            /**< used to specify color image mapped to depth type */
    depth2color = 8,            /**< used to specify depth image mapped to color type */
};

class projection_gui
{
public:
    projection_gui() = delete;
    /**
     * @brief Gui class constructor. Default constructor is not available
     * @param[in] d_width       Width of the depth image in pixels
     * @param[in] d_height      Height of the depth image in pixels
     * @param[in] c_width       Width of the color image in pixels
     * @param[in] c_height      Height of the color image in pixels
     */
    projection_gui(const int d_width, const int d_height, const int c_width, const int c_height);

    /**
     * @brief Create cv::Mat from the raw buffer of the specified type
     * @param[in] raw_data      Data returned from the camera
     * @param[in] image         Image type
     * @param[in] type          OpenCV matrix type
     * @return 0                Return on success
     * @return -1               Return if cv::Mat was not created
     */
    int create_image(const void* raw_data, const image_type image, const int type);

    /**
     * @brief Convert created images to the OpenCV matrix type CV_8UC4
     * Converts all cv::Mat objects created to the type CV_8UC4 to be able to create a whole(merged) image - window
     * that is presented to the screen
     */
    void convert_to_visualized_images();

    /**
     * @brief Draw point(circle for better sight) passed as coordinates on cv::Mat
     * @param[in] image         Image type
     * @param[in] x             X coordinate relative to image
     * @param[in] y             Y coordinate relative to image
     */
    void draw_points(image_type image, float x, float y);

    /**
     * @brief Show window of merged images
     * @return true             Streaming continues
     * @return false            Streaming is stopped
     */
    bool show_window();

    /**
     * @brief Get image on which the points were originally drawn by user
     * @return                  Return image type of the image with user-drawn points
     */
    image_type image_with_drawn_points();

    /**
     * @brief Get vector of drawn points
     * @return                  Return vector of drawn points
     */
    std::vector<rs::core::pointI32> get_points();

    /**
     * @brief is_uvmap_queried
     * Checks if uvmap image was requested by user
     * @return true             Image was requested
     * @return false            Image was not requested
     */
    bool is_uvmap_queried();

    /**
     * @brief is_invuvmap_queried
     * Checks if inversed uvmap image was requested by user
     * @return true             Image was requested
     * @return false            Image was not requested
     */
    bool is_invuvmap_queried();

    /**
     * @brief is_color_to_depth_queried
     * Checks if color image mapped to depth was requested by user
     * @return true             Image was requested
     * @return false            Image was not requested
     */
    bool is_color_to_depth_queried();

    /**
     * @brief is_depth_to_color_queried
     * Checks if depth image mapped to color was requested by user
     * @return true             Image was requested
     * @return false            Image was not requested
     */
    bool is_depth_to_color_queried();

private:
    void create_window();
    /* opencv specific methods */
    static void mouse_callback_wrapper(int event, int x, int y, int flags, void* userdata);
    void mouse_callback(int event, int x, int y, int flags, void* userdata);


    /* internal flags */
    const int m_depth_width, m_depth_height, m_color_width, m_color_height; /**< raw data resolutions taken from rs::device values */
    bool m_drawing = false, m_no_drawing = true, m_drawing_started = false, m_drawing_finished = false; /**< drawing specific flags */
    bool m_uvmap_queried = false, m_invuvmap_queried = false;
    bool m_uvmap_set = false, m_invuvmap_set = false;
    bool m_color2depth_queried = false, m_depth2color_queried = false;
    bool m_color2depth_set = false, m_depth2color_set = false;

    std::vector<rs::core::pointI32> m_points_vector;    /**< vector of drawn on image points */
    image_type m_focused_image = image_type::any;       /**< image focused by user specifier */

    /* images */
    cv::Mat m_text_image;                   /**< customly generated text message */
    cv::Mat m_depth_image;                  /**< depth image from rs::device raw data */
    cv::Mat m_color_image;                  /**< color image from rs::device raw data */
    cv::Mat m_world_image;                  /**< world pseudo-3D image from projection::query_vertices */
    cv::Mat m_uvmap_image;                  /**< uvmap from projection::query_uvmap */
    cv::Mat m_invuvmap_image;               /**< inversed uvmap from projection::query_invuvmap */
    cv::Mat m_color_mapped_to_depth_image;  /**< color custom_image from projection::create_color_image_mapped_to_depth */
    cv::Mat m_depth_mapped_to_color_image;  /**< depth custom_image from projection::create_depth_image_mapped_to_color */
    cv::Mat m_window_image;                 /**< window shown to user */

    /* window names */
    const std::string m_main_window_name = "Projection Tool";
    const std::string m_uvmap_window_name = "UVMap Image";
    const std::string m_invuvmap_window_name = "InversedUVMap Image";
    const std::string m_color2depth_name = "Color Image Mapped To Depth";
    const std::string m_depth2color_name = "Depth Image Mapped to Color";
};
