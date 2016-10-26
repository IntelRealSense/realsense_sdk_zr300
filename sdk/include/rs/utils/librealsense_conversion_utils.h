// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <cstring>
#include <librealsense/rs.hpp>

#include "rs/core/types.h"
#include "rs/core/metadata_interface.h"

namespace rs
{
    namespace utils
    {
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
                case rs::core::pixel_format::raw8       : return rs::format::raw8;
                case rs::core::pixel_format::raw10       : return rs::format::raw10;
                case rs::core::pixel_format::raw16       : return rs::format::raw16;
            }
            return static_cast<rs::format>(-1);
        }

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

        static rs::core::stream_type convert_stream_type(rs::stream lrs_stream)
        {
            switch(lrs_stream)
            {
                case rs::stream::depth                            : return rs::core::stream_type::depth;
                case rs::stream::color                            : return rs::core::stream_type::color;
                case rs::stream::infrared                         : return rs::core::stream_type::infrared;
                case rs::stream::infrared2                        : return rs::core::stream_type::infrared2;
                case rs::stream::fisheye                          : return rs::core::stream_type::fisheye;
                case rs::stream::points                           : return rs::core::stream_type::points;
                case rs::stream::rectified_color                  : return rs::core::stream_type::rectified_color;
                case rs::stream::color_aligned_to_depth           : return rs::core::stream_type::color_aligned_to_depth;
                case rs::stream::infrared2_aligned_to_depth       : return rs::core::stream_type::infrared2_aligned_to_depth;
                case rs::stream::depth_aligned_to_color           : return rs::core::stream_type::depth_aligned_to_color;
                case rs::stream::depth_aligned_to_rectified_color : return rs::core::stream_type::depth_aligned_to_rectified_color;
                case rs::stream::depth_aligned_to_infrared2       : return rs::core::stream_type::depth_aligned_to_infrared2;
            }
            return static_cast<rs::core::stream_type>(-1);
        }

        static rs::stream convert_stream_type(rs::core::stream_type framework_stream_type)
        {
            switch(framework_stream_type)
            {
                case rs::core::stream_type::depth                            : return rs::stream::depth;
                case rs::core::stream_type::color                            : return rs::stream::color;
                case rs::core::stream_type::infrared                         : return rs::stream::infrared;
                case rs::core::stream_type::infrared2                        : return rs::stream::infrared2;
                case rs::core::stream_type::fisheye                          : return rs::stream::fisheye;
                case rs::core::stream_type::points                           : return rs::stream::points;
                case rs::core::stream_type::rectified_color                  : return rs::stream::rectified_color;
                case rs::core::stream_type::color_aligned_to_depth           : return rs::stream::color_aligned_to_depth;
                case rs::core::stream_type::infrared2_aligned_to_depth       : return rs::stream::infrared2_aligned_to_depth;
                case rs::core::stream_type::depth_aligned_to_color           : return rs::stream::depth_aligned_to_color;
                case rs::core::stream_type::depth_aligned_to_rectified_color : return rs::stream::depth_aligned_to_rectified_color;
                case rs::core::stream_type::depth_aligned_to_infrared2       : return rs::stream::depth_aligned_to_infrared2;
                default                                            : return static_cast<rs::stream>(-1);
            }
            return static_cast<rs::stream>(-1);
        }

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

        static rs::core::motion_device_intrinsic convert_motion_device_intrinsic(rs_motion_device_intrinsic lrs_motion_device_intrinsic)
        {
            rs::core::motion_device_intrinsic framework_motion_device_intrinsic = {};
            std::memcpy(framework_motion_device_intrinsic.bias_variances, lrs_motion_device_intrinsic.bias_variances, sizeof(framework_motion_device_intrinsic.bias_variances));
            std::memcpy(framework_motion_device_intrinsic.noise_variances, lrs_motion_device_intrinsic.noise_variances, sizeof(framework_motion_device_intrinsic.noise_variances));
            std::memcpy(framework_motion_device_intrinsic.data, lrs_motion_device_intrinsic.data, sizeof(framework_motion_device_intrinsic.data));
            return framework_motion_device_intrinsic;
        }

        static rs::core::motion_intrinsics convert_motion_intrinsics(rs::motion_intrinsics lrs_motion_intrinsics)
        {
            rs::core::motion_intrinsics framework_motion_intrinsics =
            {
                convert_motion_device_intrinsic(lrs_motion_intrinsics.gyro),
                convert_motion_device_intrinsic(lrs_motion_intrinsics.acc)
            };

            return framework_motion_intrinsics ;
        }

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

        static rs::core::extrinsics convert_extrinsics(rs::extrinsics lrs_extrinsics)
        {
            rs::core::extrinsics framework_extrinsics = {};
            std::memcpy(framework_extrinsics.rotation, lrs_extrinsics.rotation, sizeof(framework_extrinsics.rotation));
            std::memcpy(framework_extrinsics.translation, lrs_extrinsics.translation, sizeof(framework_extrinsics.translation));
            return framework_extrinsics;
        }

        static rs::core::motion_type convert_motion_type(const rs::event lrs_event)
        {
            switch(lrs_event)
            {
                case rs::event::event_imu_accel      : return rs::core::motion_type::accel;
                case rs::event::event_imu_gyro       : return rs::core::motion_type::gyro;
            }
            return static_cast<rs::core::motion_type>(0);
        }

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

        static rs::core::metadata_type convert(rs::frame_metadata md)
        {
            switch(md)
            {
                case rs::frame_metadata::actual_exposure:
                    return rs::core::metadata_type::actual_exposure;
                default:
                    return static_cast<rs::core::metadata_type>(-1);
            }
        }

        static rs::frame_metadata convert(rs::core::metadata_type md)
        {
            switch(md)
            {
                case rs::core::metadata_type::actual_exposure:
                    return rs::frame_metadata::actual_exposure;
                default:
                   return static_cast<rs::frame_metadata>(-1);

            }
        }
    }
}
