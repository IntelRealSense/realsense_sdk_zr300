// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "image_conversion_util.h"
#include "rs/core/status.h"

#include "opencv/cv.h"
#include "opencv/cxcore.h"
#include <opencv2/imgproc/imgproc.hpp>

namespace rs
{
    namespace core
    {
        status image_conversion_util::is_conversion_valid(const image_info &src_info, const image_info &dst_info)
        {
            return get_cv_convert_enum(src_info.format, dst_info.format) == -1 ? status_param_unsupported : status_no_error;
        }

        status image_conversion_util::convert(const image_info &src_info, const uint8_t *src_data, const image_info &dst_info, uint8_t *dst_data)
        {
            auto is_valid_status = is_conversion_valid(src_info, dst_info);
            if(is_valid_status != status_no_error)
            {
                return is_valid_status;
            }

            try
            {
                pixel_format src_format;
                cv::Mat src_mat;
                cv::Mat temp = cv::Mat(src_info.height, src_info.width, rs_format_to_cv_pixel_type(src_info.format), const_cast<uint8_t*>(src_data));
                auto dst_mat = cv::Mat(dst_info.height, src_info.width, rs_format_to_cv_pixel_type(dst_info.format), const_cast<uint8_t*>(dst_data));
                switch(src_info.format)
                {
                    case rs::core::pixel_format::z16:
                    {
                        double min, max;
                        cv::minMaxIdx(temp, &min, &max);
                        max = max > 3000 ? 3000 : max;
                        cv::Mat color_map;
                        cv::convertScaleAbs(temp, color_map, 255 / max);
                        if(dst_info.format == pixel_format::rgb8)
                        {
                            cv::applyColorMap(color_map, dst_mat, cv::COLORMAP_HOT);
                            return status_no_error;
                        }
                        cv::applyColorMap(color_map, src_mat, cv::COLORMAP_HOT);
                        src_format = pixel_format::rgb8;
                        break;
                    }
                    case rs::core::pixel_format::y16:
                    {
                        double min, max;
                        cv::minMaxIdx(temp, &min, &max);
                        cv::convertScaleAbs(temp, src_mat, 255 / max);
                        src_format = src_info.format;
                        break;
                    }
                    default:
                    {
                        src_format = src_info.format;
                        src_mat = temp;
                    }
                }
                cv::cvtColor(src_mat, dst_mat, get_cv_convert_enum(src_format, dst_info.format));
                return status_no_error;
            }
            catch(const std::exception& ex)
            {
                //auto what = ex.what();
                //TODO: log exception
                return status_exec_aborted;
            }

            return status_no_error;
        }

        int image_conversion_util::rs_format_to_cv_pixel_type(rs::core::pixel_format format)
        {
            switch(format)
            {
                case rs::core::pixel_format::y8:
                case rs::core::pixel_format::raw8: return CV_8UC1;
                case rs::core::pixel_format::raw16:
                case rs::core::pixel_format::z16:
                case rs::core::pixel_format::y16: return CV_16UC1;
                case rs::core::pixel_format::bgr8:
                case rs::core::pixel_format::rgb8: return CV_8UC3;
                case rs::core::pixel_format::rgba8:
                case rs::core::pixel_format::bgra8: return CV_8UC4;
                case rs::core::pixel_format::yuyv: return CV_8UC2;
                default:
                    return -1;
            }
        }

        int image_conversion_util::get_cv_convert_enum(rs::core::pixel_format from, rs::core::pixel_format to)
        {
            switch(from)
            {
                case rs::core::pixel_format::raw8:
                case rs::core::pixel_format::y8:
                    switch(to)
                    {
                        case rs::core::pixel_format::bgr8: return CV_GRAY2BGR;
                        case rs::core::pixel_format::rgb8: return CV_GRAY2RGB;
                        case rs::core::pixel_format::rgba8: return CV_GRAY2RGBA;
                        case rs::core::pixel_format::bgra8: return CV_GRAY2BGRA;
                        default:
                            return -1;
                    }
                case rs::core::pixel_format::z16:
                case rs::core::pixel_format::y16:
                    switch(to)
                    {
                        case rs::core::pixel_format::bgr8: return CV_GRAY2BGR;
                        case rs::core::pixel_format::rgb8: return CV_GRAY2RGB;
                        case rs::core::pixel_format::rgba8: return CV_GRAY2RGBA;
                        case rs::core::pixel_format::bgra8: return CV_GRAY2BGRA;
                        default:
                            return -1;
                    }
                case rs::core::pixel_format::bgr8:
                    switch(to)
                    {
                        case rs::core::pixel_format::y8: return CV_BGR2GRAY;
                        case rs::core::pixel_format::rgb8: return CV_BGR2RGB;
                        case rs::core::pixel_format::rgba8: return CV_BGR2RGBA;
                        case rs::core::pixel_format::bgra8: return CV_BGR2BGRA;
                        default:
                            return -1;
                    }
                case rs::core::pixel_format::rgb8:
                    switch(to)
                    {
                        case rs::core::pixel_format::y8: return CV_RGB2GRAY;
                        case rs::core::pixel_format::bgr8: return CV_RGB2BGR;
                        case rs::core::pixel_format::rgba8: return CV_RGB2RGBA;
                        case rs::core::pixel_format::bgra8: return CV_RGB2BGRA;
                        default:
                            return -1;
                    }
                case rs::core::pixel_format::rgba8:
                    switch(to)
                    {
                        case rs::core::pixel_format::y8: return CV_RGBA2GRAY;
                        case rs::core::pixel_format::bgr8: return CV_RGBA2BGR;
                        case rs::core::pixel_format::rgb8: return CV_RGBA2RGB;
                        case rs::core::pixel_format::bgra8: return CV_RGBA2BGRA;
                        default:
                            return -1;
                    }
                case rs::core::pixel_format::bgra8:
                    switch(to)
                    {
                        case rs::core::pixel_format::y8: return CV_BGRA2GRAY;
                        case rs::core::pixel_format::bgr8: return CV_BGRA2BGR;
                        case rs::core::pixel_format::rgb8: return CV_BGRA2RGB;
                        case rs::core::pixel_format::rgba8: return CV_BGRA2RGBA;
                        default:
                            return -1;
                    }
                case rs::core::pixel_format::yuyv:
                    switch(to)
                    {
                        case rs::core::pixel_format::y8: return CV_YUV2GRAY_YUYV;
                        case rs::core::pixel_format::bgr8: return CV_YUV2BGR_YUYV;
                        case rs::core::pixel_format::rgb8: return CV_YUV2RGB_YUYV;
                        case rs::core::pixel_format::rgba8: return CV_YUV2RGBA_YUYV;
                        case rs::core::pixel_format::bgra8: return CV_YUV2BGRA_YUYV;
                        default:
                            return -1;
                    }
                default:
                    return -1;
            }
        }
    }
}
