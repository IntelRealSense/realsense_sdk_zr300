// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "stdint.h"

/**
 * \file types.h
 * @brief Describes common types.
 */

namespace rs
{
    namespace core
    {
        /**
        * @brief Constructs a unique id from a given set of characters.
        * @param X1        Char 1
        * @param X2        Char 2
        * @param X3        Char 3
        * @param X4        Char 4
        * @return int32_t  Unique id
        */
        inline int32_t CONSTRUCT_UID(char X1,char X2,char X3,char X4)
        {
            return (((unsigned int)(X4)<<24)+((unsigned int)(X3)<<16)+((unsigned int)(X2)<<8)+(unsigned int)(X1));
        }

        /**
        * @brief Image rotation options.
        */
        enum class rotation
        {
            rotation_0_degree   = 0x0,   /**< 0-degree rotation             */
            rotation_90_degree  = 90,    /**< 90-degree clockwise rotation  */
            rotation_180_degree = 180,   /**< 180-degree clockwise rotation */
            rotation_270_degree = 270,   /**< 270-degree clockwise rotation */
            rotation_invalid_value = -1
        };

        /**
        * @brief Device details.
        */
        struct device_info
        {
            char                name[224];    /**< Device name                                 */
            char                serial[32];   /**< Serial number                               */
            char                firmware[32]; /**< Firmware version                            */
            rs::core::rotation  rotation;     /**< How the camera device is physically mounted */
        };

        /**
        * @brief Size
        */
        struct sizeI32
        {
            int32_t width, height;
        };

        /**
        * @brief Sample flags
        */
        enum class sample_flags
        {
            none,     /**< No special flags                                                     */
            external  /**< Sample generated from external device (platform camera/external IMU) */
        };

        /**
        * @brief Stream type
        */
        enum class stream_type : int32_t
        {
            depth                            = 0,  /**< Native stream of depth data produced by the device                                                   */
            color                            = 1,  /**< Native stream of color data captured by the device                                                   */
            infrared                         = 2,  /**< Native stream of infrared data captured by the device                                                */
            infrared2                        = 3,  /**< Native stream of infrared data captured from a second viewpoint by the device                        */
            fisheye                          = 4,  /**< Native stream of color data captured by the fisheye camera                                           */

            rectified_color                  = 6,  /**< Synthetic stream containing undistorted color data with no extrinsic rotation from the depth stream  */
            max                                    /**< Maximum number of stream types - must be last                                                        */
        };

        /**
        * @brief Distortion type
        */
        enum class distortion_type : int32_t
        {
            none                   = 0, /**< Rectilinear images, no distortion compensation required                                                             */
            modified_brown_conrady = 1, /**< Equivalent to Brown-Conrady distortion, except that tangential distortion is applied to radially distorted points   */
            inverse_brown_conrady  = 2, /**< Equivalent to Brown-Conrady distortion, except undistorts image instead of distorting it                            */
            distortion_ftheta      = 3  /**< Distortion model for the fisheye                                                                                    */
        };

        /**
        * @brief Represents the motion sensor scale, bias and variances.
        */
        struct motion_device_intrinsics
        {
            float data[3][4];           /**< Scale_X        cross_axis        cross_axis      Bias_X  <br>
                                             cross_axis     Scale_Y           cross_axis      Bias_Y  <br>
                                             cross_axis     cross_axis        Scale_Z         Bias_Z       */
            float noise_variances[3];
            float bias_variances[3];
        };

        /**
        * @brief Stream intrinsic parameters.
        *
        * The intrinsics parameters describe the relationship between the 2D and 3D coordinate systems of the camera stream.
        * The image produced by the camera is slightly different, depending on the camera distortion model. However, the intrinsics
        * parameters are sufficient to describe the images produced from the different models, using different closed-form formula.
        * The parameters are used for projection operation - mapping points from 3D coordinate space to 2D pixel location in the image,
        * and deprojection operation - mapping 2D pixel, using its depth data, to a point in the 3D coordinate space.
        */
        struct intrinsics
        {
            int             width;     /**< Width of the image, in pixels                                                                    */
            int             height;    /**< Height of the image, in pixels                                                                   */
            float           ppx;       /**< Horizontal coordinate of the principal point of the image, as a pixel offset from the left edge  */
            float           ppy;       /**< Vertical coordinate of the principal point of the image, as a pixel offset from the top edge     */
            float           fx;        /**< Focal length of the image plane, as a multiple of pixel width                                    */
            float           fy;        /**< Focal length of the image plane, as a multiple of pixel height                                   */
            distortion_type model;     /**< Distortion model of the image                                                                    */
            float           coeffs[5]; /**< Distortion coefficients                                                                          */
        };

        /**
        * @brief Camera extrinsics parameters
        *
        * The extrinsics parameters describe the relationship between different 3D coordinate systems.
        * Camera streams are produced by imagers in different locations. The extrinsics parameters are used to transform 3D points
        * from one camera coordinate system to another camera coordinate system. The transformation is a standard affine transformation
        * using a 3x3 rotation matrix and a 3-component translation vector.
        */
        struct extrinsics
        {
            float rotation[9];    /**< Column-major 3x3 rotation matrix          */
            float translation[3]; /**< 3-element translation vector, in meters   */
        };

        /**
        * @brief pointI32
        */
        struct pointI32
        {
            int32_t x, y; /**< Represents a two-dimensional point */
        };

        /**
        * @brief pointF32
        */
        struct pointF32
        {
            float x, y; /**< Represents a two-dimensional point */
        };

        /**
        * @brief point3dF32
        */
        struct point3dF32
        {
            float x, y, z; /**< Represents a three-dimensional point */
        };

        /**
        * @brief Rectangle
        */
        struct rect
        {
            int x;
            int y;
            int width;
            int height;
        };

        /**
        * @brief rectF32
        */
        struct rectF32
        {
            float x;
            float y;
            float width;
            float height;
        };

        /**
        * @brief Motion types
        */
        enum class motion_type : int32_t
        {
            accel       = 1,    /**< Accelerometer */
            gyro        = 2,    /**< Gyroscope     */

            max,
        };
    }
}
