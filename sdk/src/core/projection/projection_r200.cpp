// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <sstream>
#include <memory>
#include <iostream>
#include <list>
#include "projection_r200.h"
#pragma warning (disable : 4068)

#include "src/utilities/image/librealsense_image_utils.h"
#include "librealsense/rs.h"
#include "projection/projection_utils.h"
#include "math_projection_interface.h"

using namespace rs::utils;

namespace rs
{
    namespace core
    {

        class data_releaser_impl : public image_interface::data_releaser_interface
        {
        public:
            data_releaser_impl(uint8_t* data) :data(data) {}
            void release() { delete [] data; }
        private:
            uint8_t* data;
        };

        ds4_projection::ds4_projection(bool platformCameraProjection) :
            m_buffer(nullptr),
            m_initialize_status(initialize_status::not_initialized),
            m_is_platform_camera_projection(platformCameraProjection),
            m_projection_spec(nullptr),
            m_projection_spec_size(0)
        {
            reset();
        }

        ds4_projection::~ds4_projection()
        {
            reset();
        }

        void ds4_projection::reset()
        {
            std::lock_guard<std::recursive_mutex> auto_lock(cs_buffer);
            memset(m_distorsion_color_coeffs, 0, sizeof(m_distorsion_color_coeffs));
            if (m_projection_spec) projection_utils::aligned_free(m_projection_spec);
            m_projection_spec = nullptr;
            m_projection_spec_size = 0;
            if (m_buffer) projection_utils::aligned_free(m_buffer);
            m_buffer = nullptr;
            m_buffer_size = 0;
        }

        status ds4_projection::init_from_float_array(r200_projection_float_array *data)
        {
            m_color_size.width = static_cast<int>(data->color_width);
            m_color_size.height = static_cast<int>(data->color_height);
            m_depth_size.width = static_cast<int>(data->depth_width);
            m_depth_size.height = static_cast<int>(data->depth_height);
            m_is_color_rectified = static_cast<int>(data->is_color_rectified) ? true : false;
            bool isMirrored = static_cast<int>(data->is_mirrored) ? true : false;
            m_color_calib = data->color_calib;
            m_depth_calib = data->depth_calib;
            m_color_transform = data->color_transform;
            m_depth_transform = data->depth_transform;

            m_color_size_rectified = m_color_size;
            m_color_size_unrectified = m_color_size;
            m_color_calib_rectified = m_color_calib;
            m_color_calib_unrectified = m_color_calib;
            m_color_transform_rectified = m_color_transform;
            m_color_transform_unrectified = m_color_transform;

            return init(isMirrored);
        }

        status ds4_projection::init(bool isMirrored)
        {
            m_initialize_status = initialize_status::not_initialized;

            if (m_depth_size.width && m_depth_size.height)
                m_initialize_status = m_initialize_status | initialize_status::depth_initialized;

            if ((!m_is_color_rectified || m_color_size_rectified.width) &&
                    (!m_is_color_rectified || m_color_size_rectified.height) &&
                    (m_is_color_rectified || m_color_size_unrectified.width) &&
                    (m_is_color_rectified || m_color_size_unrectified.height) &&
                    m_color_size.width &&
                    m_color_size.height)
            {
                m_initialize_status = m_initialize_status | initialize_status::color_initialized;
            }

            if (!m_initialize_status)
                return status::status_handle_invalid;

            m_camera_depth_params[0] = m_depth_calib.focal_length.x;
            m_camera_depth_params[1] = m_depth_calib.principal_point.x;
            m_camera_depth_params[2] = m_depth_calib.focal_length.y;
            m_camera_depth_params[3] = m_depth_calib.principal_point.y;

            m_translation[0] = m_depth_transform.translation[0];
            m_translation[1] = m_depth_transform.translation[1];
            m_translation[2] = m_depth_transform.translation[2];

            bool flipY = false;
            if (flipY)
                m_translation[1] = -m_translation[1];

            if (isMirrored)
            {
                m_camera_depth_params[0] = -m_camera_depth_params[0];
                m_camera_depth_params[1] = (float)m_depth_size.width - 1.f - m_camera_depth_params[1];
            }

            if (flipY)
                m_camera_depth_params[2] = -m_camera_depth_params[2];

            int projectionSpecSize;

            sizeI32 depthSize = {m_depth_size.width, m_depth_size.height};
            m_math_projection.rs_projection_get_size_32f(depthSize, &projectionSpecSize);
            if( m_projection_spec_size < projectionSpecSize)
            {
                m_projection_spec = (uint8_t*)projection_utils::aligned_malloc(sizeof(uint8_t) * projectionSpecSize);
                m_projection_spec_size = projectionSpecSize;
                memset(m_projection_spec, 0, projectionSpecSize);
            }
            m_math_projection.rs_projection_init_32f(depthSize, m_camera_depth_params, 0, (projection_spec_32f*)m_projection_spec);

            m_camera_color_params[0] = m_color_calib.focal_length.x;
            m_camera_color_params[1] = m_color_calib.principal_point.x;
            m_camera_color_params[2] = m_color_calib.focal_length.y;
            m_camera_color_params[3] = m_color_calib.principal_point.y;

            if (m_is_color_rectified)
            {
                if (isMirrored)
                {
                    m_camera_color_params[0] = -m_camera_color_params[0];
                    m_camera_color_params[1] = (float)m_color_size_rectified.width - 1.f - m_camera_color_params[1];
                }
                if (flipY) m_camera_color_params[2] = -m_camera_color_params[2];
            }
            else
            {
                if (isMirrored)
                {
                    m_camera_color_params[0] = -m_camera_color_params[0];
                    m_camera_color_params[1] = (float)m_color_size_unrectified.width - 1.f - m_camera_color_params[1];
                }
#pragma novector
                memcpy(m_rotation, m_depth_transform.rotation, 9 * sizeof(float));
                if (flipY)
                {
                    m_rotation[3] = -m_rotation[1];
                    m_rotation[4] = -m_rotation[4];
                    m_rotation[5] = -m_rotation[7];
                }
                projection_ds_lms12(m_rotation, m_translation, m_invrot_color, m_invtrans_color);
#pragma novector
                float distortion[5] =
                {
                    m_color_calib.radial_distortion[0],
                    m_color_calib.radial_distortion[1],
                    m_color_calib.tangential_distortion[0],
                    m_color_calib.tangential_distortion[1],
                    m_color_calib.radial_distortion[2]
                };
                if (memcmp(m_distorsion_color_coeffs, distortion, 5 * sizeof(float)))
                {
                    memcpy(m_distorsion_color_coeffs, distortion, 5 * sizeof(float));
                    float camera[4] =
                    {
                        m_color_calib.focal_length.x * 2.f / m_color_size_unrectified.width,
                        m_color_calib.principal_point.x * 2.f / m_color_size_unrectified.width - 1.f,
                        m_color_calib.focal_length.y * 2.f / m_color_size_unrectified.height,
                        m_color_calib.principal_point.y * 2.f / m_color_size_unrectified.height - 1.f
                    };
                    distorsion_ds_lms(camera, m_distorsion_color_coeffs, m_invdist_color_coeffs);
                }
            }
            return status::status_no_error;
        }

        status  ds4_projection::project_depth_to_camera(int32_t npoints, point3dF32 *pos_uvz, point3dF32 *pos3d)
        {
            if (npoints <= 0) return status::status_param_unsupported;
            if (!pos_uvz) return status::status_handle_invalid;
            if (!pos3d) return status::status_handle_invalid;
            if (!(m_initialize_status & initialize_status::depth_initialized)) return status::status_data_unavailable;
            m_math_projection.rs_3d_array_projection_32f((const float*)pos_uvz, (float*)pos3d, npoints, m_camera_depth_params, 0, 0, 0, 0, 0);
            return status::status_no_error;
        }

        status  ds4_projection::project_camera_to_depth(int32_t npoints, point3dF32 *pos3d, pointF32 *pos_uv)
        {
            if (npoints <= 0) return status::status_param_unsupported;
            if (!pos3d) return status::status_handle_invalid;
            if (!pos_uv) return status::status_handle_invalid;
            if (!(m_initialize_status & initialize_status::depth_initialized)) return status::status_data_unavailable;
            status result = m_math_projection.rs_3d_array_projection_32f((const float*)pos3d, (float*)pos_uv, npoints, nullptr, nullptr, nullptr, nullptr, nullptr, m_camera_depth_params);
            if(result != status::status_no_error)
            {
                return status::status_param_unsupported;
            }
            return status::status_no_error;
        }


        status  ds4_projection::project_color_to_camera(int32_t npoints, point3dF32 *pos_ijz, point3dF32 *pos3d)
        {
            if (npoints <= 0) return status::status_param_unsupported;
            if (!pos_ijz) return status::status_handle_invalid;
            if (!pos3d) return status::status_handle_invalid;
            if (!(m_initialize_status & initialize_status::color_initialized)) return status::status_data_unavailable;
            if (m_is_color_rectified)
            {
                float translationC[3] = {-m_translation[0], -m_translation[1], -m_translation[2]};
                m_math_projection.rs_3d_array_projection_32f((const float*)pos_ijz, (float*)pos3d, npoints, m_camera_color_params, 0, 0, translationC, 0, 0);
            }
            else
            {
                m_math_projection.rs_3d_array_projection_32f((const float*)pos_ijz, (float*)pos3d, npoints, m_camera_color_params, m_invdist_color_coeffs, m_invrot_color, m_invtrans_color, 0, 0);
            }
            return status::status_no_error;
        }


        status  ds4_projection::project_camera_to_color(int32_t npoints, point3dF32 *pos3d, pointF32 *pos_ij)
        {
            if (npoints <= 0) return status::status_param_unsupported;
            if (!pos3d) return status::status_handle_invalid;
            if (!pos_ij) return status::status_handle_invalid;
            if (!(m_initialize_status & initialize_status::color_initialized)) return status::status_data_unavailable;
            if (m_is_color_rectified)
            {
                status result = m_math_projection.rs_3d_array_projection_32f((const float*)pos3d, (float*)pos_ij, npoints, nullptr, nullptr, nullptr, m_translation, nullptr, m_camera_color_params);
                if (result != status::status_no_error)
                {
                    return status::status_param_unsupported;
                }
            }
            else
            {
                // if color image is not rectified, we should assume rotation and distorsion of color image
                status result = m_math_projection.rs_3d_array_projection_32f((const float*)pos3d, (float*)pos_ij, npoints, nullptr, nullptr, m_rotation, m_translation, m_distorsion_color_coeffs, m_camera_color_params);
                if (result != status::status_no_error)
                {
                    return status::status_param_unsupported;
                }
            }
            return status::status_no_error;
        }


        // Query Map/Vertices
        status  ds4_projection::query_uvmap(image_interface *depth, pointF32 *uvmap)
        {
            if (!depth) return status::status_handle_invalid;
            if (!uvmap) return status::status_handle_invalid;
            if (m_initialize_status != initialize_status::both_initialized) return status::status_data_unavailable;
            image_info info = depth->query_info();
            const void* data = depth->query_data();
            if (!data)
            {
                return status::status_data_not_initialized;
            }
            int dst_pitches = info.width * image_utils::get_pixel_size(rs::format::bgra8) * 2;
            sizeI32 depth_size = { info.width, info.height };
            float inv_width = 1.f / (float)m_color_size.width;
            float inv_height = 1.f / (float)m_color_size.height;
            float cameraC[4] = { m_camera_color_params[0] * inv_width, m_camera_color_params[1] * inv_width, m_camera_color_params[2] * inv_height, m_camera_color_params[3] * inv_height };
            if (m_is_color_rectified)
            {
                if (status::status_param_unsupported  == m_math_projection.rs_projection_16u32f_c1cxr((const unsigned short*)data, depth_size, depth->query_info().pitch, (float*)uvmap, dst_pitches,
                        0, m_translation, 0, cameraC, (const projection_spec_32f*)m_projection_spec))
                {
                    return status::status_feature_unsupported;
                }
            }
            else
            {
                // if color image is not rectified, we should assume rotation and distorsion of the image
                if (status::status_param_unsupported  == m_math_projection.rs_projection_16u32f_c1cxr((const unsigned short*)data, depth_size, depth->query_info().pitch, (float*)uvmap, dst_pitches,
                        m_rotation, m_translation, m_distorsion_color_coeffs, cameraC, (const projection_spec_32f*)m_projection_spec))
                {
                    return status::status_feature_unsupported;
                }
            }
            m_math_projection.rs_uvmap_filter_32f_c2ir((float*)uvmap, dst_pitches, depth_size, 0, 0, 0 );
            return status::status_no_error;
        }


        status  ds4_projection::query_invuvmap(image_interface *depth, pointF32 *inv_uvmap)
        {
            if (!inv_uvmap) return status::status_handle_invalid;
            if (!depth) return status::status_handle_invalid;
            if (m_initialize_status != initialize_status::both_initialized) return status::status_data_unavailable;
            std::vector<pointF32> uvmap(depth->query_info().width * depth->query_info().height);
            if (status::status_no_error > query_uvmap(depth, uvmap.data()))
                return status::status_data_unavailable;
            int src_pitches = depth->query_info().width * image_utils::get_pixel_size(rs::format::xyz32f) * 2;
            image_info info = depth->query_info();
            sizeI32 depth_size = { info.width, info.height };
            sizeI32 color_size = { m_color_size.width, m_color_size.height };
            rect uvMapRoi = { 0, 0, info.width, info.height };
            if(status::status_no_error != m_math_projection.rs_uvmap_invertor_32f_c2r((float*)uvmap.data(), src_pitches, depth_size, uvMapRoi, (float*)inv_uvmap, color_size.width * sizeof(pointF32), color_size, 1))
                return status::status_feature_unsupported;
            return status::status_no_error;
        }


        status  ds4_projection::query_vertices(image_interface *depth, point3dF32 *vertices)
        {
            if (!depth) return status::status_handle_invalid;
            if (!vertices) return status::status_handle_invalid;
            if (!(m_initialize_status & initialize_status::depth_initialized)) return status::status_data_unavailable;
            image_info info = depth->query_info();
            const void* data = depth->query_data();
            if (!data) return status::status_data_unavailable;
            sizeI32 depth_size = { info.width, info.height };
            m_math_projection.rs_projection_16u32f_c1cxr((const unsigned short*)data, depth_size, depth->query_info().pitch, (float*)vertices, depth_size.width * sizeof(point3dF32),
                    0, 0, 0, 0, (const projection_spec_32f*)m_projection_spec);
            return status::status_no_error;
        }


        // Map
        status ds4_projection::map_depth_to_color(int32_t npoints, point3dF32 *pos_uvz, pointF32  *pos_ij)
        {
            if (npoints <= 0) return status::status_param_unsupported;
            if (!pos_uvz) return status::status_handle_invalid;
            if (!pos_ij) return status::status_handle_invalid;
            if (m_initialize_status != initialize_status::both_initialized) return status::status_data_unavailable;
            if (m_is_color_rectified)
            {
                status result = m_math_projection.rs_3d_array_projection_32f((const float*)pos_uvz, (float*)pos_ij, npoints, m_camera_depth_params, nullptr, nullptr, m_translation, nullptr, m_camera_color_params);
                if (result != status::status_no_error)
                {
                    return status::status_param_unsupported;
                }
            }
            else
            {
                // if color image is not rectified, we should assume rotation and distorsion of the image
                status result = m_math_projection.rs_3d_array_projection_32f((const float*)pos_uvz, (float*)pos_ij, npoints, m_camera_depth_params, nullptr, m_rotation, m_translation, m_distorsion_color_coeffs, m_camera_color_params);
                if (result != status::status_no_error)
                {
                    return status::status_param_unsupported;
                }
            }
            return status::status_no_error;
        }

        struct depth_uv_point
        {
            float u, v;
            int16_t i, j;
        };
        status  ds4_projection::map_color_to_depth(image_interface *depth, int32_t npoints, pointF32 *pos_ij, pointF32 *pos_uv)
        {
            if (!depth) return status::status_handle_invalid;
            if (npoints <= 0) return status::status_param_unsupported;
            if (!pos_ij) return status::status_handle_invalid;
            if (!pos_uv) return status::status_handle_invalid;

            if (m_initialize_status != initialize_status::both_initialized) return status::status_data_unavailable;

            image_info depth_info = depth->query_info();

            std::lock_guard<std::recursive_mutex> auto_lock(cs_buffer);
            int32_t depthPoints = x64_ALIGNMENT(depth_info.width * depth_info.height * sizeof(depth_uv_point));
            if(depthPoints > m_buffer_size)
            {
                if(m_buffer) projection_utils::aligned_free(m_buffer);
                m_buffer = 0;
                m_buffer = (pointF32*)projection_utils::aligned_malloc(sizeof(pointF32) * depthPoints);
                if(!m_buffer)
                {
                    return status::status_feature_unsupported;
                }
                m_buffer_size = depthPoints;
            }
            std::vector<pointF32> uvmap(depth_info.width * depth_info.height);
            if (status::status_no_error > query_uvmap(depth, uvmap.data()))
                return status::status_data_unavailable;
            uint8_t* uvmap_data_ptr = (uint8_t*)uvmap.data();
            int32_t uvmap_step = depth_info.width * image_utils::get_pixel_size(rs::format::bgra8) * 2;
            pointF32* uv_point;
            depth_uv_point *depth_uvPoint = (depth_uv_point*)m_buffer;
            int depth_points_number = 0;

            for(int j = 0; j < depth_info.height; j++)
            {
                for (int i = 0; i < depth_info.width; i++)
                {
                    uv_point = ((pointF32*)uvmap_data_ptr) + i;
                    if(uv_point->x >= 0.f && uv_point->x < 1.f && uv_point->y >= 0.f && uv_point->y < 1.f)
                    {
                        depth_uvPoint[depth_points_number].u = uv_point->x;
                        depth_uvPoint[depth_points_number].v = uv_point->y;
                        depth_uvPoint[depth_points_number].i = (int16_t)i;
                        depth_uvPoint[depth_points_number].j = (int16_t)j;
                        depth_points_number++;
                    }
                }
                uvmap_data_ptr += uvmap_step;
            }

            float inv_color_width = 1.f / m_color_size.width;
            float inv_color_height = 1.f / m_color_size.height;
            float oR, oC, prod_x, prod_y;
            double minD, r;
            double minDMax = 2. * ((double)inv_color_width * inv_color_width + (double)inv_color_height * inv_color_height);

            status sts = status::status_no_error;
            for(int n = 0; n < npoints; n++)
            {
                pointF32 tmp_pos_color = pos_ij[n];
                tmp_pos_color.x *= inv_color_width;
                tmp_pos_color.y *= inv_color_height;
                minD = minDMax;
                oR = oC = -1.0f;	// default values
                for(int d = 0; d < depth_points_number; d++)
                {
                    prod_x = tmp_pos_color.x - depth_uvPoint[d].u;
                    prod_y = tmp_pos_color.y - depth_uvPoint[d].v;
                    r = (double)prod_x * prod_x + (double)prod_y * prod_y;

                    // find and store the nearest point to uvmap
                    if (r < minD)
                    {
                        minD = r;
                        oC = depth_uvPoint[d].i;
                        oR = depth_uvPoint[d].j;
                    }
                }
                if (oR == -1.0f)
                {
                    pos_uv[n].x = pos_uv[n].y = -1.0f;
                    sts = status::status_value_out_of_range;
                }
                else
                {
                    pos_uv[n].x = oC;
                    pos_uv[n].y = oR;
                }
            }
            return sts;
        }

        // Create images
        image_interface *ds4_projection::create_color_image_mapped_to_depth(image_interface *depth, image_interface *color)
        {
            if (!depth) return nullptr;
            if (!color) return nullptr;

            image_info depth_info = depth->query_info();
            image_info color_info = color->query_info();
            int32_t pitch = depth_info.width * image_utils::get_pixel_size(color_info.format);
            image_info color2depth_info = { depth_info.width, depth_info.height, color_info.format, pitch };

            uint8_t* color2depth_data = new uint8_t[color2depth_info.height * color2depth_info.pitch];
            memset(color2depth_data, 0, color2depth_info.height * color2depth_info.pitch);
            int32_t color2depth_step = color2depth_info.pitch;
            uint8_t* ptr_color2depth_data = color2depth_data;

            std::vector<pointF32> uvmap(depth_info.width * depth_info.height);
            if (status::status_no_error > query_uvmap(depth, uvmap.data()))
            {
                delete[] color2depth_data;
                return nullptr;
            }
            int32_t uvmap_step = depth_info.width * image_utils::get_pixel_size(rs::format::bgra8) * 2;
            uint8_t* ptr_uvmap = (uint8_t*)uvmap.data();
            pointF32* ptr_uvmap_32f;

            int32_t color_step = color_info.pitch;
            uint8_t* ptr_color = reinterpret_cast<uint8_t*>(const_cast<void*>(color->query_data()));

            int channels = 1;
            switch(color2depth_info.format)
            {
                case pixel_format::rgb8:
                case pixel_format::bgr8:
                    channels = image_utils::get_pixel_size(pixel_format::rgb8); break;
                case pixel_format::rgba8:
                case pixel_format::bgra8:
                    channels = image_utils::get_pixel_size(pixel_format::rgba8); break;
                case pixel_format::yuyv:
                case pixel_format::y16:
                    channels = image_utils::get_pixel_size(pixel_format::yuyv); break;
                default:
                    channels = 1;
            }

            for(int i = 0; i < depth_info.height; i++)
            {
                for (int j = 0, xi = 0; j < depth_info.width; j++, xi+= channels)
                {
                    ptr_uvmap_32f = ((pointF32*)ptr_uvmap) + j;
                    if(ptr_uvmap_32f->x >= 0.f && ptr_uvmap_32f->x < 1.f && ptr_uvmap_32f->y >= 0.f && ptr_uvmap_32f->y < 1.f)
                    {
                        uint8_t* ptr_color_tmp = &ptr_color[(int)(ptr_uvmap_32f->y * color_info.height) * color_step
                                                            + channels * (int)(ptr_uvmap_32f->x * color_info.width)];
                        for (int c = 0; c < channels; c++)
                        {
                            ptr_color2depth_data[xi+c] = ptr_color_tmp[c];
                        }
                    }
                }
                ptr_uvmap += uvmap_step;
                ptr_color2depth_data += color2depth_step;
            }

            smart_ptr<image_interface::data_releaser_interface> data_releaser(new data_releaser_impl(color2depth_data));

            return image_interface::create_instance_from_raw_data(&color2depth_info,
                                                                  color2depth_data,
                                                                  stream_type::color,
                                                                  image_interface::flag::any,
                                                                  0,
                                                                  0,
                                                                  nullptr,
                                                                  data_releaser);
        }


        image_interface* ds4_projection::create_depth_image_mapped_to_color(image_interface *depth, image_interface *color)
        {
            if (!depth) return nullptr;
            if (!color) return nullptr;

            uint16_t default_depth_value = 0;
            image_info depth_info = depth->query_info();
            image_info color_info = color->query_info();
            int32_t pitch = color_info.width * image_utils::get_pixel_size(rs::format::z16);
            image_info depth2color_info = { color_info.width, color_info.height, depth_info.format, pitch };

            uint8_t* depth2color_data = new uint8_t[depth2color_info.height * depth2color_info.pitch];
            memset(depth2color_data, 0, depth2color_info.height * depth2color_info.pitch);
            uint16_t* depth_data = reinterpret_cast<uint16_t*>(const_cast<void*>(depth->query_data()));

            std::vector<pointF32> uvmap(depth_info.width*depth_info.height);
            if (status::status_no_error > query_uvmap(depth, uvmap.data()))
            {
                delete[] depth2color_data;
                return nullptr;
            }
            std::lock_guard<std::recursive_mutex> auto_lock(cs_buffer);
            int32_t invuvmapPoints = x64_ALIGNMENT(color_info.width * color_info.height * sizeof(pointF32));
            if(invuvmapPoints > m_buffer_size)
            {
                if(m_buffer) projection_utils::aligned_free(m_buffer);
                m_buffer = 0;
                m_buffer = (pointF32*)projection_utils::aligned_malloc(sizeof(pointF32) * invuvmapPoints);

                if(!m_buffer)
                {
                    delete[] depth2color_data;
                    return 0;
                }
                m_buffer_size = invuvmapPoints;
            }
            sizeI32 depth_size = { depth_info.width, depth_info.height };
            sizeI32 color_size = { color_info.width, color_info.height };
            rect uvmap_roi = { 0, 0, depth_info.width, depth_info.height };
            m_math_projection.rs_uvmap_invertor_32f_c2r((float*)uvmap.data(), depth_info.width * image_utils::get_pixel_size(pixel_format::xyz32f) * 2,
                    depth_size, uvmap_roi, (float*)m_buffer, color_info.width * sizeof(pointF32), color_size, 0 );
            m_math_projection.rs_remap_16u_c1r((unsigned short*)depth_data, depth_size, depth_info.pitch,
                                               (float*)m_buffer, color_info.width * sizeof(pointF32), (uint16_t*)depth2color_data,
                                               color_size, depth2color_info.pitch, 0, default_depth_value);

            smart_ptr<image_interface::data_releaser_interface> data_releaser(new data_releaser_impl(depth2color_data));

            return image_interface::create_instance_from_raw_data(&depth2color_info,
                                                                  depth2color_data,
                                                                  stream_type::depth,
                                                                  image_interface::flag::any,
                                                                  0,
                                                                  0,
                                                                  nullptr,
                                                                  data_releaser);
        }


        // Helper Functions
        int ds4_projection::distorsion_ds_lms(float* Kc, float* invdistc, float* distc)
        {
            double dst[5];
            double x, y, r2, r4, r2c, xc, yc;
            double invKc0 = 1.f / Kc[0];
            double invKc2 = 1.f / Kc[2];
            double step = 0.1f;
            double rect = .7f;
            int i, cnt, APitch;

            // Find necessary amount of points for memory allocation
            cnt = 0;
            for (double v = -1.; v < 1.; v += step)
            {
                for (double u = -1.; u < 1.; u += step)
                {
                    if (u > -rect && u < rect && v > -rect && v < rect) continue;
                    cnt++;
                }
            }
            cnt *= 2;

            for (i = 0; i < 5; i++) distc[i] = 0.f;

            double *A = (double*)projection_utils::aligned_malloc(2 * 5 * cnt*2 * sizeof(float)); APitch = sizeof(float);
            double *b = (double*)projection_utils::aligned_malloc(sizeof(double)*cnt*2);
            if (!A || !b)
            {
                if (A) projection_utils::aligned_free(A);
                if (b) projection_utils::aligned_free(b);
                return -1;
            }

            // Based on spreaded points on an image, find coefficients for overdetermined equation system
            double *APtr = A;
            double *bPtr = b;
            for (double v = -1.; v < 1.; v += step)
            {
                y = (v - Kc[3]) * invKc2;
                for (double u = -1.; u < 1.; u += step)
                {
                    if (u > -rect && u < rect && v > -rect && v < rect) continue;
                    x = (u - Kc[1]) * invKc0;
                    r2 = x * x + y * y;
                    r4 = r2 * r2;
                    r2c = 1.f + invdistc[0] * r2 + invdistc[1] * r4 + invdistc[4] * r2 * r4;
                    xc = x * r2c + 2.f * invdistc[2] * x * y + invdistc[3] * (r2 + 2.f * x * x);
                    yc = y * r2c + 2.f * invdistc[3] * x * y + invdistc[2] * (r2 + 2.f * y * y);

                    // Coeffitients for U component
                    r2 = xc * xc + yc * yc;
                    r4 = r2 * r2;
                    APtr[0] = xc * r2;
                    APtr[1] = xc * r4;
                    APtr[2] = 2.f * xc * yc;
                    APtr[3] = r2 + 2.f * xc * xc;
                    APtr[4] = xc * r2 * r4;
                    APtr = (double*)((unsigned char*)APtr + APitch);
                    bPtr[0] = x - xc;
                    bPtr++;

                    // Coeffitients for V component
                    APtr[0] = yc * r2;
                    APtr[1] = yc * r4;
                    APtr[2] = r2 + 2.f * yc * yc;
                    APtr[3] = 2.f * xc * yc;
                    APtr[4] = yc * r2 * r4;
                    APtr = (double*)((unsigned char*)APtr + APitch);
                    bPtr[0] = y - yc;
                    bPtr++;
                }
            }

            // Provide QR decomposition for overdetermined equation system
            double *pDecomp = (double*)((unsigned char*)A + APitch * cnt);
            double *pBuffer = &b[cnt];
#pragma warning( disable: 4996 )
            status sts = m_math_projection.rs_qr_decomp_m_64f(A, APitch, sizeof(double), pBuffer, pDecomp, APitch, sizeof(double), 5, cnt);
            if (sts)
            {
                if (A) projection_utils::aligned_free(A);
                if (b) projection_utils::aligned_free(b);
                return -1;
            }

            // Solve overdetermined equation system
            sts = m_math_projection.rs_qr_back_subst_mva_64f(pDecomp, APitch, sizeof(double), pBuffer, b, cnt * sizeof(double), sizeof(double),
                    dst, 5 * sizeof(double), sizeof(double), 5, cnt, 1);
#pragma warning( default: 4996 )
            if (A) projection_utils::aligned_free(A);
            if (b) projection_utils::aligned_free(b);
            if (sts) return -1;

            // Copy overdetermined equation system solution to output buffer
            for (i = 0; i < 5; i++) distc[i] = (float)dst[i];

            return 0;
        }

        // Find inverse projection matrix
        int ds4_projection::projection_ds_lms12(float* r, float* t, float* ir, float* it)
        {
            status sts;
            double dst[12];
            double xc, yc, zc;
            double x, y, z;
            double step = 500.;
            double cube = 2000.;
            int i, APitch;
            int cnt = 0;

            // Find necessary amount of points for memory allocation
            for (x = -cube/2.f; x <= cube/2.f; x += step)
                for (y = -cube/2.f; y <= cube/2.f; y += step)
                    for (z = step; z <= cube; z += step)
                        cnt++;
            cnt *= 3;

            for (i = 0; i < 9; i++) ir[i] = 0.f;
            for (i = 0; i < 3; i++) it[i] = 0.f;

            double *A = (double*)projection_utils::aligned_malloc(2 * 12 * cnt*2 * sizeof(float)); APitch = sizeof(float);
            memset( A, 0, APitch * cnt * 2 );
            double *b = (double*)projection_utils::aligned_malloc(sizeof(double)*cnt*2);
            if (!A || !b)
            {
                if (A) projection_utils::aligned_free(A);
                if (b) projection_utils::aligned_free(b);
                return -1;
            }

            double *APtr = A;
            double *bPtr = b;
            for (x = -cube/2.; x <= cube/2.; x += step)
            {
                for (y = -cube/2.; y <= cube/2.; y += step)
                {
                    for (z = step; z <= cube; z += step)
                    {
                        xc = r[0] * x + r[1] * y + r[2] * z + t[0];
                        yc = r[3] * x + r[4] * y + r[5] * z + t[1];
                        zc = r[6] * x + r[7] * y + r[8] * z + t[2];

                        // Coeffitients for X component
                        APtr[0] = xc*xc;
                        APtr[1] = xc*yc;
                        APtr[2] = xc*zc;
                        APtr[3] = xc;
                        APtr = (double*)((unsigned char*)APtr + APitch);
                        bPtr[0] = xc*x;
                        bPtr++;

                        // Coeffitients for Y component
                        APtr[4] = yc*xc;
                        APtr[5] = yc*yc;
                        APtr[6] = yc*zc;
                        APtr[7] = yc;
                        APtr = (double*)((unsigned char*)APtr + APitch);
                        bPtr[0] = yc*y;
                        bPtr++;

                        // Coeffitients for Z component
                        APtr[8]  = zc*xc;
                        APtr[9]  = zc*yc;
                        APtr[10] = zc*zc;
                        APtr[11] = zc;
                        APtr = (double*)((unsigned char*)APtr + APitch);
                        bPtr[0] = zc*z;
                        bPtr++;
                    }
                }
            }

            // Provide QR decomposition for overdetermined equation system
            double *pDecomp = (double*)((unsigned char*)A + APitch * cnt);
            double *pBuffer = &b[cnt];
#pragma warning( disable: 4996 )
            sts = m_math_projection.rs_qr_decomp_m_64f(A, APitch, sizeof(double), pBuffer, pDecomp, APitch, sizeof(double), 12, cnt);
            if (sts)
            {
                if (A) projection_utils::aligned_free(A);
                if (b) projection_utils::aligned_free(b);
                return -1;
            }

            // Solve overdetermined equation system
            sts = m_math_projection.rs_qr_back_subst_mva_64f(pDecomp, APitch, sizeof(double), pBuffer, b, cnt * sizeof(double), sizeof(double),
                    dst, 12 * sizeof(double), sizeof(double), 12, cnt, 1);
#pragma warning( default: 4996 )
            if (A) projection_utils::aligned_free(A);
            if (b) projection_utils::aligned_free(b);
            if (sts) return -1;


            // Copy overdetermined equation system solution to output buffer
            ir[0] = (float)dst[0];
            ir[1] = (float)dst[1];
            ir[2] = (float)dst[2];
            it[0] = (float)dst[3];
            ir[3] = (float)dst[4];
            ir[4] = (float)dst[5];
            ir[5] = (float)dst[6];
            it[1] = (float)dst[7];
            ir[6] = (float)dst[8];
            ir[7] = (float)dst[9];
            ir[8] = (float)dst[10];
            it[2] = (float)dst[11];
            return 0;
        }



        extern "C" {
            stream_calibration convert_intrinsics(rs_intrinsics* intrin)
            {
                stream_calibration calib = {};
                calib.focal_length.x = intrin->fx;
                calib.focal_length.y = intrin->fy;
                calib.principal_point.x = intrin->ppx;
                calib.principal_point.y = intrin->ppy;

                calib.radial_distortion[0] = intrin->coeffs[0];
                calib.radial_distortion[1] = intrin->coeffs[1];
                calib.tangential_distortion[0] = intrin->coeffs[2];
                calib.tangential_distortion[1] = intrin->coeffs[3];
                calib.radial_distortion[2] = intrin->coeffs[4];
                return calib;
            }

            extern void* rs_projection_create_instance_from_intrinsics_extrinsics(rs_intrinsics *colorIntrinsics, rs_intrinsics *depthIntrinsics, rs_extrinsics *extrinsics)
            {
                if (!colorIntrinsics) return nullptr;
                if (!depthIntrinsics) return nullptr;
                if (!extrinsics) return nullptr;

                ds4_projection* proj = new ds4_projection(false);
                r200_projection_float_array calib = {};
                calib.marker = 12345.f;
                calib.color_width = (float)colorIntrinsics->width;
                calib.color_height = (float)colorIntrinsics->height;
                calib.depth_width = (float)depthIntrinsics->width;
                calib.depth_height = (float)depthIntrinsics->height;
                calib.is_color_rectified = 1; // for case of request for the RS_STREAM_COLOR
                calib.is_mirrored = 0;
                calib.reserved = 0;
                calib.color_calib = convert_intrinsics(colorIntrinsics);
                calib.depth_calib = convert_intrinsics(depthIntrinsics);

                memcpy(calib.depth_transform.translation, extrinsics->translation, 3 * sizeof(float));
                memcpy(calib.depth_transform.rotation, extrinsics->rotation, 9 * sizeof(float));
                // for some reason translation in DS4 Projection was expressed in millimeters (not in meters)
                calib.depth_transform.translation[0] *= 1000;
                calib.depth_transform.translation[1] *= 1000;
                calib.depth_transform.translation[2] *= 1000;
                proj->init_from_float_array(&calib);
                return proj;
            }
        }
    }
}
