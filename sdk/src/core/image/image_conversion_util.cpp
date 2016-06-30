// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "custom_image.h"
#include "image_conversion_util.h"
#include "status.h"

#include "opencv/cv.h"
#include "opencv/cxcore.h"

namespace rs
{
    namespace core
    {
        status image_conversion_util::is_conversion_valid(const image_info &src_info, const image_info &dst_info)
        {
            if(src_info.width == dst_info.width && src_info.height == dst_info.height &&  src_info.format != dst_info.format &&
                    ((src_info.format == pixel_format::rgb8 && dst_info.format == pixel_format::y8)    ||
                     (src_info.format == pixel_format::yuyv && dst_info.format == pixel_format::y8)    ||
                     (src_info.format == pixel_format::z16  && dst_info.format == pixel_format::rgb8) ||
                     (src_info.format == pixel_format::y16  && dst_info.format == pixel_format::rgb8)
                    )
              )
            {
                return status_no_error;
            }
            return status_param_unsupported;
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
                switch(src_info.format)
                {
                    case pixel_format::rgb8:
                        switch(dst_info.format)
                        {
                            case pixel_format::y8:
                            {
                                IplImage * src_image = cvCreateImageHeader(cvSize(src_info.width, src_info.height), IPL_DEPTH_8U, 3);
                                cvSetData(src_image, static_cast<void *>(const_cast<uint8_t *>(src_data)), src_image->widthStep);
                                IplImage * dst_image = cvCreateImageHeader(cvSize(dst_info.width, dst_info.height), IPL_DEPTH_8U, 1);
                                cvSetData(dst_image , static_cast<void *>(const_cast<uint8_t *>(dst_data)), dst_image ->widthStep);

                                cvCvtColor(src_image, dst_image, CV_RGB2GRAY);

                                cvReleaseImageHeader(&src_image);
                                cvReleaseImageHeader(&dst_image);

                                break;
                            }
                            default:
                                return status_feature_unsupported;
                        }
                        break;
                    case pixel_format::yuyv:
                        switch(dst_info.format)
                        {
                            case pixel_format::y8:
                            {
                                IplImage * src_image = cvCreateImageHeader(cvSize(src_info.width, src_info.height), IPL_DEPTH_8U, 2);
                                cvSetData(src_image, static_cast<void *>(const_cast<uint8_t *>(src_data)), src_info.pitch);
                                IplImage * dst_image = cvCreateImageHeader(cvSize(dst_info.width, dst_info.height), IPL_DEPTH_8U, 1);
                                cvSetData(dst_image , static_cast<void *>(const_cast<uint8_t *>(dst_data)), dst_image ->widthStep);

                                cvCvtColor(src_image, dst_image, CV_YUV2GRAY_YUYV);

                                cvReleaseImageHeader(&src_image);
                                cvReleaseImageHeader(&dst_image);
                                break;
                            }
                            default:
                                return status_feature_unsupported;
                        }
                        break;
                    case pixel_format::z16:
                    case pixel_format::y16:
                        switch(dst_info.format)
                        {
                            case pixel_format::rgb8:
                            {
                                /*IplImage * src_image = cvCreateImageHeader(cvSize(src_info.width, src_info.height), IPL_DEPTH_16U, 1);
                                cvSetData(src_image, static_cast<void *>(const_cast<uint8_t *>(src_data)), src_info.pitch);

                                //opencv cant convert 16 bit gray to rgb without down scaling it to 8 bit gray first
                                bool is_copying_data = true;
                                cv::Mat convert_matrix(src_image , is_copying_data); //true for copying the data
                                auto pixel_scale_ratio_from_16bit_to_8_bit = 1.0/256.0;
                                convert_matrix.convertTo(convert_matrix, CV_8U, pixel_scale_ratio_from_16bit_to_8_bit);

                                IplImage grayscale_8bit_image = convert_matrix;
                                IplImage * dst_image = cvCreateImageHeader(cvSize(dst_info.width, dst_info.height), IPL_DEPTH_8U, 3);
                                cvSetData(dst_image , static_cast<void *>(const_cast<uint8_t *>(dst_data)), dst_image ->widthStep);

                                cvCvtColor(&grayscale_8bit_image, dst_image, CV_GRAY2RGB);

                                cvReleaseImageHeader(&src_image);
                                cvReleaseImageHeader(&dst_image);*/

                                // native impl
                                uint16_t * upcasted_src_data = (uint16_t *)src_data;
                                for(int32_t i = 0; i < src_info.width * src_info.height; i++)
                                {
                                    uint8_t * current_dst_position = dst_data + (i * 3);

                                    uint8_t scaled_down_val = (uint8_t)((double)(*(upcasted_src_data + i)) * (1.0/256.0));
                                    const uint8_t R = 0, G = 1, B = 2;
                                    *(current_dst_position + R) = scaled_down_val;
                                    *(current_dst_position + G) = scaled_down_val;
                                    *(current_dst_position + B) = scaled_down_val;
                                }

                                break;
                            }
                            default:
                                return status_feature_unsupported;
                        }

                        break;
                    default:
                        return status_feature_unsupported;
                }
            }
            catch(const std::exception& ex)
            {
                //auto what = ex.what();
                //TODO: log exception
                return status_exec_aborted;
            }

            return status_no_error;
        }
    }
}
