// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.


/**
* \file librealsense_conversion_utils.h
* @brief Contains static conversion functions from the SDK types to librealsense types and vise versa.
*/

#pragma once
#include <cstring>
#include <librealsense/rs.hpp>
#include "rs/core/image_interface.h"
#include "rs/core/metadata_interface.h"

namespace rs
{
    namespace utils
    {
        /**
        * @brief Converts pixel format from the SDK type to librealsense type.
        *
        * @param[in] framework_pixel_format    SDK pixel format.
        * @return rs::format                   Librealsense pixel format type.
        */
        static rs::format convert_pixel_format(rs::core::pixel_format framework_pixel_format)
        {
            switch(framework_pixel_format)
            {
                case rs::core::pixel_format::any         : return rs::format::any;
                case rs::core::pixel_format::z16         : return rs::format::z16;
                case rs::core::pixel_format::disparity16 : return rs::format::disparity16;
                case rs::core::pixel_format::xyz32f      : return rs::format::xyz32f;
                case rs::core::pixel_format::yuyv        : return rs::format::yuyv;
                case rs::core::pixel_format::rgb8        : return rs::format::rgb8;
                case rs::core::pixel_format::bgr8        : return rs::format::bgr8;
                case rs::core::pixel_format::rgba8       : return rs::format::rgba8;
                case rs::core::pixel_format::bgra8       : return rs::format::bgra8;
                case rs::core::pixel_format::y8          : return rs::format::y8;
                case rs::core::pixel_format::y16         : return rs::format::y16;
                case rs::core::pixel_format::raw8        : return rs::format::raw8;
                case rs::core::pixel_format::raw10       : return rs::format::raw10;
                case rs::core::pixel_format::raw16       : return rs::format::raw16;
            }
            return static_cast<rs::format>(-1);
        }

        /**
        * @brief Converts pixel format from the librealsense type to the SDK type.
        *
        * @param[in] lsr_pixel_format       Librealsense pixel format.
        * @return rs::core::pixel_format    SDK pixel format type.
        */
        static rs::core::pixel_format convert_pixel_format(rs::format lsr_pixel_format)
        {
            switch(lsr_pixel_format)
            {
                case rs::format::any         : return rs::core::pixel_format::any;
                case rs::format::z16         : return rs::core::pixel_format::z16;
                case rs::format::disparity16 : return rs::core::pixel_format::disparity16;
                case rs::format::xyz32f      : return rs::core::pixel_format::xyz32f;
                case rs::format::yuyv        : return rs::core::pixel_format::yuyv;
                case rs::format::rgb8        : return rs::core::pixel_format::rgb8;
                case rs::format::bgr8        : return rs::core::pixel_format::bgr8;
                case rs::format::rgba8       : return rs::core::pixel_format::rgba8;
                case rs::format::bgra8       : return rs::core::pixel_format::bgra8;
                case rs::format::y8          : return rs::core::pixel_format::y8;
                case rs::format::y16         : return rs::core::pixel_format::y16;
                case rs::format::raw8        : return rs::core::pixel_format::raw8;
                case rs::format::raw10       : return rs::core::pixel_format::raw10;
                case rs::format::raw16       : return rs::core::pixel_format::raw16;
            }
            return static_cast<rs::core::pixel_format>(-1);
        }
    
        /**
        * @brief Converts stream type from the librealsense type to the SDK type.
        *
        * @param[in] lrs_stream            Librealsense stream type.
        * @return rs::core::stream_type    SDK stream type.
        */
        static rs::core::stream_type convert_stream_type(rs::stream lrs_stream)
        {
            switch(lrs_stream)
            {
                case rs::stream::depth                            : return rs::core::stream_type::depth;
                case rs::stream::color                            : return rs::core::stream_type::color;
                case rs::stream::infrared                         : return rs::core::stream_type::infrared;
                case rs::stream::infrared2                        : return rs::core::stream_type::infrared2;
                case rs::stream::fisheye                          : return rs::core::stream_type::fisheye;
                case rs::stream::rectified_color                  : return rs::core::stream_type::rectified_color;
            default:
                return static_cast<rs::core::stream_type>(-1);
            }
            return static_cast<rs::core::stream_type>(-1);
        }

        /**
        * @brief Converts stream type from the SDK type to the librealsense type.
        *
        * @param[in] framework_stream_type    SDK stream type.
        * @return rs::stream                  Librealsense stream type.
        */
        static rs::stream convert_stream_type(rs::core::stream_type framework_stream_type)
        {
            switch(framework_stream_type)
            {
                case rs::core::stream_type::depth                            : return rs::stream::depth;
                case rs::core::stream_type::color                            : return rs::stream::color;
                case rs::core::stream_type::infrared                         : return rs::stream::infrared;
                case rs::core::stream_type::infrared2                        : return rs::stream::infrared2;
                case rs::core::stream_type::fisheye                          : return rs::stream::fisheye;
                case rs::core::stream_type::rectified_color                  : return rs::stream::rectified_color;
                default                                            : return static_cast<rs::stream>(-1);
            }
            return static_cast<rs::stream>(-1);
        }

        /**
        * @brief Converts distortion type from the librealsense type to the SDK type.
        *
        * @param[in] lrs_distortion            Librealsense distortion type.
        * @return rs::core::distortion_type    SDK distortion type.
        */
        static rs::core::distortion_type convert_distortion(const rs::distortion lrs_distortion)
        {
            switch(lrs_distortion)
            {
                case rs::distortion::none                   : return rs::core::distortion_type::none;
                case rs::distortion::modified_brown_conrady : return rs::core::distortion_type::modified_brown_conrady ;
                case rs::distortion::inverse_brown_conrady  : return rs::core::distortion_type::inverse_brown_conrady;
                case rs::distortion::distortion_ftheta      : return rs::core::distortion_type::distortion_ftheta;
            }
            return static_cast<rs::core::distortion_type>(-1);
        }

        /**
        * @brief Converts motion device intrinsics from the librealsense type to the SDK type.
        *
        * @param[in] lrs_motion_device_intrinsic        Librealsense motion device intrinsics.
        * @return rs::core::motion_device_intrinsics    SDK motion device intrinsics.
        */
        static rs::core::motion_device_intrinsics convert_motion_device_intrinsics(rs_motion_device_intrinsic lrs_motion_device_intrinsic)
        {
            rs::core::motion_device_intrinsics framework_motion_device_intrinsics = {};
            std::memcpy(framework_motion_device_intrinsics.bias_variances, lrs_motion_device_intrinsic.bias_variances, sizeof(framework_motion_device_intrinsics.bias_variances));
            std::memcpy(framework_motion_device_intrinsics.noise_variances, lrs_motion_device_intrinsic.noise_variances, sizeof(framework_motion_device_intrinsics.noise_variances));
            std::memcpy(framework_motion_device_intrinsics.data, lrs_motion_device_intrinsic.data, sizeof(framework_motion_device_intrinsics.data));
            return framework_motion_device_intrinsics;
        }

        /**
        * @brief Converts intrinsics from the librealsense type to the SDK type.
        *
        * @param[in] lrs_intrinsics      Librealsense intrinsics type.
        * @return rs::core::intrinsics   SDK intrinsics.
        */
        static rs::core::intrinsics convert_intrinsics(rs::intrinsics lrs_intrinsics)
        {
            rs::core::intrinsics framework_intrinsics =
            {
                lrs_intrinsics.width,
                lrs_intrinsics.height,
                lrs_intrinsics.ppx,
                lrs_intrinsics.ppy,
                lrs_intrinsics.fx,
                lrs_intrinsics.fy,
                convert_distortion(lrs_intrinsics.model()),
                {}
            };
            std::memcpy(framework_intrinsics.coeffs, lrs_intrinsics.coeffs, sizeof(framework_intrinsics.coeffs));

            return framework_intrinsics;
        }

        /**
        * @brief Converts extrinsics from the librealsense type to the SDK type.
        *
        * @param[in] lrs_extrinsics       Librealsense extrinsics.
        * @return rs::core::extrinsics    SDK extrinsics.
        */
        static rs::core::extrinsics convert_extrinsics(rs::extrinsics lrs_extrinsics)
        {
            rs::core::extrinsics framework_extrinsics = {};
            std::memcpy(framework_extrinsics.rotation, lrs_extrinsics.rotation, sizeof(framework_extrinsics.rotation));
            std::memcpy(framework_extrinsics.translation, lrs_extrinsics.translation, sizeof(framework_extrinsics.translation));
            return framework_extrinsics;
        }

        /**
        * @brief Converts motion from the librealsense type to the SDK type.
        *
        * @param[in] lrs_event             Librealsense event.
        * @return rs::core::motion_type    SDK motion type.
        */
        static rs::core::motion_type convert_motion_type(const rs::event lrs_event)
        {
            switch(lrs_event)
            {
                case rs::event::event_imu_accel      : return rs::core::motion_type::accel;
                case rs::event::event_imu_gyro       : return rs::core::motion_type::gyro;
                default                              : return static_cast<rs::core::motion_type>(0);
            }
            return static_cast<rs::core::motion_type>(0);
        }

        /**
        * @brief Converts motion from the SDK type to the librealsense type.
        *
        * @param[in] framework_motion    SDK motion type.
        * @return rs::event              Librealsense event.
        */
        static rs::event convert_motion_type(const rs::core::motion_type framework_motion)
        {
            switch(framework_motion)
            {
                case rs::core::motion_type::accel      : return rs::event::event_imu_accel;
                case rs::core::motion_type::gyro       : return rs::event::event_imu_gyro;
                default                      : return static_cast<rs::event>(0);
            }
            return static_cast<rs::event>(0);
        }

        /**
        * @brief Converts timestamp domain from the SDK type to the librealsense type.
        *
        * @param[in] framework_timestamp_domain    SDK timestamp domain type.
        * @return rs::timestamp_domain             Librealsense timestamp domain type.
        */
        static rs::timestamp_domain convert_timestamp_domain(const rs::core::timestamp_domain framework_timestamp_domain)
        {
            switch(framework_timestamp_domain)
            {
                case rs::core::timestamp_domain::camera : return rs::timestamp_domain::camera;
                case rs::core::timestamp_domain::microcontroller : return rs::timestamp_domain::microcontroller;
                default : break;
            }
            return static_cast<rs::timestamp_domain>(-1);
        }

        /**
        * @brief Converts timestamp domain from the librealsense type to the SDK type.
        *
        * @param[in] lrs_timestamp_domain       Librealsense timestamp domain type.
        * @return rs::core::timestamp_domain    SDK timestamp domain type.
        */
        static rs::core::timestamp_domain convert_timestamp_domain(const rs::timestamp_domain lrs_timestamp_domain)
        {
            switch(lrs_timestamp_domain)
            {
                case rs::timestamp_domain::camera :          return rs::core::timestamp_domain::camera;
                case rs::timestamp_domain::microcontroller : return rs::core::timestamp_domain::microcontroller;
            default : break;
            }
            return static_cast<rs::core::timestamp_domain>(-1);
        }

        /**
        * @brief Converts frame metadata from the librealsense type to the SDK type.
        *
        * @param[in] md                      Librealsense frame metadata type.
        * @return rs::core::metadata_type    SDK frame metadata type.
        */
        static rs::core::metadata_type convert(rs::frame_metadata md)
        {
            switch(md)
            {
                case rs::frame_metadata::actual_exposure:
                    return rs::core::metadata_type::actual_exposure;
                case rs::frame_metadata::actual_fps:
                    return rs::core::metadata_type::actual_fps;
                default:
                    return static_cast<rs::core::metadata_type>(-1);
            }
        }

        /**
        * @brief Converts frame metadata from the SDK type to the librealsense type.
        *
        * @param[in] md                 SDK frame metadata type.
        * @return rs::frame_metadata    Librealsense frame metadata type.
        */
        static rs::frame_metadata convert(rs::core::metadata_type md)
        {
            switch(md)
            {
                case rs::core::metadata_type::actual_exposure:
                    return rs::frame_metadata::actual_exposure;
                case rs::core::metadata_type::actual_fps:
                    return rs::frame_metadata::actual_fps;
                default:
                   return static_cast<rs::frame_metadata>(-1);
            }
        }
    }
}
