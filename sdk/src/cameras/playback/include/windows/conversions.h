// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <map>
#include <librealsense/rs.hpp>
#include "windows/file_types_windows.h"
#include "include/file_types.h"
#include "projection_types.h"
#include "status.h"
#include "image/image_utils.h"
#include "rs/utils/log_utils.h"

using namespace rs::core;

namespace rs
{
    namespace PXC
    {
        namespace Conversions
        {

            static uint64_t rssdk2lrs_timestamp(uint64_t time)
            {
                return time * 0.0001;
            }

            static status convert(StreamType source, rs_stream &target)
            {
                target = (rs_stream)-1;
                switch(source)
                {
                    case StreamType::STREAM_TYPE_COLOR: target = rs_stream::RS_STREAM_COLOR; break;
                    case StreamType::STREAM_TYPE_DEPTH: target = rs_stream::RS_STREAM_DEPTH; break;
                    case StreamType::STREAM_TYPE_IR:    target = rs_stream::RS_STREAM_INFRARED; break;
                    case StreamType::STREAM_TYPE_LEFT:  target = rs_stream::RS_STREAM_INFRARED; break;
                    case StreamType::STREAM_TYPE_RIGHT: target = rs_stream::RS_STREAM_INFRARED2; break;
                    default: return status_item_unavailable; break;
                }
                return status_no_error;
            }

            static status convert(compression_type source, file_types::compression_type &target)
            {
                target = (file_types::compression_type)-1;
                switch(source)
                {
                    case COMPRESSION_NONE: target = file_types::compression_type::none; break;
                    case COMPRESSION_H264: target = file_types::compression_type::h264; break;
                    case COMPRESSION_LZO: target = file_types::compression_type::lzo; break;
                    default: return status_item_unavailable; break;
                }
                return status_no_error;
            }

            static status convert(Rotation source, rs::core::rotation &target)
            {
                target = (rs::core::rotation)-1;
                switch(source)
                {
                    case Rotation::ROTATION_0_DEGREE: target = rs::core::rotation::rotation_0_degree; break;
                    case Rotation::ROTATION_90_DEGREE: target = rs::core::rotation::rotation_90_degree; break;
                    case Rotation::ROTATION_180_DEGREE: target = rs::core::rotation::rotation_180_degree; break;
                    case Rotation::ROTATION_270_DEGREE: target = rs::core::rotation::rotation_270_degree; break;
                    default: return status_item_unavailable; break;
                }
                return status_no_error;
            }

            static status convert(PixelFormat source, rs_format &target)
            {
                target = (rs_format)-1;
                switch(source)
                {
                    case 	 PixelFormat::PIXEL_FORMAT_ANY:        target = rs_format::RS_FORMAT_ANY; break;
                    case 	 PixelFormat::PIXEL_FORMAT_DEPTH:      target = rs_format::RS_FORMAT_Z16; break;
                    case     PixelFormat::PIXEL_FORMAT_DEPTH_F32:  target = rs_format::RS_FORMAT_XYZ32F; break;
                    case 	 PixelFormat::PIXEL_FORMAT_YUY2:       target = rs_format::RS_FORMAT_YUYV; break;
                    case 	 PixelFormat::PIXEL_FORMAT_RGB24:      target = rs_format::RS_FORMAT_RGB8; break;
                    case 	 PixelFormat::PIXEL_FORMAT_RGB32:      target = rs_format::RS_FORMAT_RGBA8; break;
                    case 	 PixelFormat::PIXEL_FORMAT_Y8:         target = rs_format::RS_FORMAT_Y8; break;
                    case 	 PixelFormat::PIXEL_FORMAT_Y16:        target = rs_format::RS_FORMAT_Y16; break;
                    case 	 PixelFormat::PIXEL_FORMAT_DEPTH_RAW:  target = rs_format::RS_FORMAT_RAW10; break;
                    default:                                       return status_item_unavailable; break;
                }
                return status_no_error;
            }

            static status convert(const CoordinateSystem &source, file_types::coordinate_system &target)
            {
                target = (file_types::coordinate_system)-1;
                switch(source)
                {
                    case CoordinateSystem::COORDINATE_SYSTEM_REAR_DEFAULT:  target = file_types::coordinate_system::rear_default; break;
                    case CoordinateSystem::COORDINATE_SYSTEM_REAR_OPENCV:   target = file_types::coordinate_system::rear_opencv; break;
                    case CoordinateSystem::COORDINATE_SYSTEM_FRONT_DEFAULT: target = file_types::coordinate_system::front_default; break;
                    default:                                                target = (file_types::coordinate_system)source; break;
                }
                return status_no_error;
            }

            static status convert(const CMDiskFormat::Header &source, file_types::file_header &target)
            {
                memset(&target, 0, sizeof(target));
                file_types::coordinate_system cs;
                if(convert(source.coordinateSystem, cs) != status_no_error)
                    return status_item_unavailable;
                target.id = source.id;
                target.version = source.version;
                target.coordinate_system = cs;
                target.first_frame_offset = source.firstFrameOffset;
                target.nstreams = source.nstreams;
                return status_no_error;
            }

            static status convert(const CMDiskFormat::StreamInfo &source, file_types::stream_info &target)
            {
                memset(&target, 0, sizeof(target));
                rs_stream stream;
                file_types::compression_type ctype;
                if(convert(source.stype, stream) != status_no_error || convert(source.ctype, ctype) != status_no_error)
                    return status_item_unavailable;
                target.stream = stream;
                target.nframes = source.nframes;
                target.ctype = ctype;
                return status_no_error;
            }

            static status convert(const CMDiskFormat::DeviceInfoDisk &source, core::file_types::device_info &target)
            {
                memset(&target, 0, sizeof(target));
                rs::core::rotation rotaion;
                if(convert(source.rotation, rotaion) != status_no_error)
                    return status_item_unavailable;
                target.rotation = rotaion;
                auto nameSize = sizeof(target.name) / sizeof(target.name[0]);
                for(size_t i = 0; i < nameSize; i++)
                    target.name[i] = source.name[i];
                auto serialSize = sizeof(target.serial) / sizeof(target.serial[0]);
                for(size_t i = 0; i < serialSize; i++)
                    target.serial[i] = source.serial[i];
                std::stringstream ss;
                ss << source.firmware[0] << "." << source.firmware[1] << "." <<
                   source.firmware[2] << "." << source.firmware[3];
                memcpy(&target.firmware, ss.str().c_str(), ss.str().size());
                return status_no_error;
            }

            static status convert(const ImageInfo &source, file_types::frame_info &target)
            {
                memset(&target, 0, sizeof(target));
                rs_format format;
                if(convert(source.format, format) != status_no_error)
                    return status_item_unavailable;
                target.width = source.width;
                target.height = source.height;
                target.stride = (source.width);
                target.bpp = rs::core::image_utils::get_pixel_size((rs::format)format);
                target.format = format;
                return status_no_error;
            }

            static status convert(const CMDiskFormat::StreamProfileDisk &source, file_types::stream_profile &target)
            {
                memset(&target, 0, sizeof(target));
                file_types::frame_info frame_info;
                if(convert(source.imageInfo, frame_info) != status_no_error)
                    return status_item_unavailable;
                if(source.frameRate[0] != source.frameRate[1])
                {
                    LOG_ERROR("min != max fps is not supported, setting to min");
                }
                target.frame_rate = source.frameRate[0];
                target.info = frame_info;

                return status_no_error;
                //rv.options = profile.options;
            }

            static status convert(const CMDiskFormat::StreamProfileSetDisk &source, std::map<rs_stream, file_types::stream_info> &target)
            {
                file_types::stream_profile sp;
                if(source.color.imageInfo.format)
                {
                    if(convert(source.color, sp) != status_no_error) return status_item_unavailable;
                    target[rs_stream::RS_STREAM_COLOR].profile = sp;
                }
                if(source.depth.imageInfo.format)
                {
                    if(convert(source.depth, sp) != status_no_error) return status_item_unavailable;
                    target[rs_stream::RS_STREAM_DEPTH].profile = sp;
                }
                if(source.ir.imageInfo.format)
                {
                    if(convert(source.ir, sp) != status_no_error) return status_item_unavailable;
                    target[rs_stream::RS_STREAM_INFRARED].profile = sp;
                }
                if(source.left.imageInfo.format)
                {
                    if(convert(source.left, sp) != status_no_error) return status_item_unavailable;
                    target[rs_stream::RS_STREAM_INFRARED].profile = sp;
                }
                if(source.right.imageInfo.format)
                {
                    if(convert(source.right, sp) != status_no_error) return status_item_unavailable;
                    target[rs_stream::RS_STREAM_INFRARED2].profile = sp;
                }
                return status_no_error;
            }

            static status convert(const CMDiskFormat::FrameMetaData &source, file_types::frame_info &target)
            {
                memset(&target, 0, sizeof(target));
                rs_stream stream;
                if(convert(source.streamType, stream) != status_no_error)
                    return status_item_unavailable;
                target.stream = stream;
                target.time_stamp = rssdk2lrs_timestamp(source.timeStamp);
                return status_no_error;
            }

            static rs::intrinsics get_intrinsics(StreamType stream, ds_projection::ProjectionData* projection)
            {
                rs::intrinsics rv;
                switch(stream)
                {
                    case StreamType::STREAM_TYPE_COLOR:
                    {
                        rv.fx = projection->thirdCameraParams.isRectified ?
                                projection->thirdCameraParams.calibIntrinsicsRectified.rfx : projection->thirdCameraParams.calibIntrinsicsNonRectified.fx;
                        rv.fy = projection->thirdCameraParams.isRectified ?
                                projection->thirdCameraParams.calibIntrinsicsRectified.rfy : projection->thirdCameraParams.calibIntrinsicsNonRectified.fy;
                        rv.ppx = projection->thirdCameraParams.isRectified ?
                                 projection->thirdCameraParams.calibIntrinsicsRectified.rpx : projection->thirdCameraParams.calibIntrinsicsNonRectified.px;
                        rv.ppy = projection->thirdCameraParams.isRectified ?
                                 projection->thirdCameraParams.calibIntrinsicsRectified.rpy : projection->thirdCameraParams.calibIntrinsicsNonRectified.py;
                        rv.width = projection->thirdCameraParams.width;
                        rv.height = projection->thirdCameraParams.height;
                        break;
                    }
                    case StreamType::STREAM_TYPE_DEPTH:
                    {
                        rv.fx = projection->zRectified ? projection->zIntrinRect.rfx : projection->zIntrinNonRect.fx;
                        rv.fy = projection->zRectified ? projection->zIntrinRect.rfy : projection->zIntrinNonRect.fy;
                        rv.ppx = projection->zRectified ? projection->zIntrinRect.rpx : projection->zIntrinNonRect.px;
                        rv.ppy = projection->zRectified ? projection->zIntrinRect.rpy : projection->zIntrinNonRect.py;
                        rv.width = projection->dWidth;
                        rv.height = projection->dHeight;
                        break;
                    }
                    case StreamType::STREAM_TYPE_LEFT:
                    {
                        rv.fx = projection->zRectified ? projection->lrIntrinRect.rfx : projection->lIntrinNonRect.fx;
                        rv.fy = projection->zRectified ? projection->lrIntrinRect.rfy : projection->lIntrinNonRect.fy;
                        rv.ppx = projection->zRectified ? projection->lrIntrinRect.rpx : projection->lIntrinNonRect.px;
                        rv.ppy = projection->zRectified ? projection->lrIntrinRect.rpy : projection->lIntrinNonRect.py;
                        rv.width = projection->dWidth;
                        rv.height = projection->dHeight;
                        break;
                    }
                    case StreamType::STREAM_TYPE_RIGHT:
                    {
                        rv.fx = projection->zRectified ? projection->lrIntrinRect.rfx : projection->rIntrinNonRect.fx;
                        rv.fy = projection->zRectified ? projection->lrIntrinRect.rfy : projection->rIntrinNonRect.fy;
                        rv.ppx = projection->zRectified ? projection->lrIntrinRect.rpx : projection->rIntrinNonRect.px;
                        rv.ppy = projection->zRectified ? projection->lrIntrinRect.rpy : projection->rIntrinNonRect.py;
                        rv.width = projection->dWidth;
                        rv.height = projection->dHeight;
                        break;
                    }
                    default: break;
                }
                return rv;
            }

            static rs::extrinsics get_extrinsics(StreamType stream, ds_projection::ProjectionData *projection)
            {
                rs::extrinsics rv;
                switch(stream)
                {
                    case StreamType::STREAM_TYPE_COLOR:
                    {
                        for(int i = 0; i < 3; ++i)
                        {
                            rv.translation[i] = projection->calibParams.T[0][i] * 0.001;
                        }
                        memcpy(rv.rotation, projection->calibParams.Rthird[0], sizeof(float) * 9);
                        break;
                    }
                    default: break;//TODO - add support in other streams
                }
                return rv;
            }
        }
    }
}
