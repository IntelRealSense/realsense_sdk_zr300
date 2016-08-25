// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include <mutex>
#include <vector>

#include "rs/core/projection_interface.h"
#include "rs/utils/ref_count_base.h"
#include "math_projection_interface.h"

namespace rs
{
    namespace core
    {
        projection_interface* create_projection_ds4(bool isPlatformCameraProjection);

        struct stream_transform
        {
            float translation[3];   /* The translation in mm of the camera coordinate system origin to the world coordinate system origin. The world coordinate system coincides with the depth camera coordinate system. */
            float rotation[3][3];   /* The rotation of the camera coordinate system with respect to the world coordinate system. The world coordinate system coincides with the depth camera coordinate system. */
        };

        struct stream_calibration
        {
            pointF32 focal_length;    /* The sensor focal length in pixels along the x and y axes. The parameters vary with the stream resolution setting. */
            pointF32 principal_point; /*  The sensor principal point in pixels along the x and y axes. The parameters vary with the stream resolution setting. */
            float    radial_distortion[3];     /*  The radial distortion coefficients, as described by camera model equations. */
            float    tangential_distortion[2]; /* The tangential distortion coefficients, as described by camera model equations. */
        };

        struct r200_projection_float_array
        {
            float               marker;
            float               color_width;
            float               color_height;
            float               depth_width;
            float               depth_height;
            float               is_color_rectified;
            float               is_mirrored;
            float               reserved;
            stream_calibration  color_calib;
            stream_calibration  depth_calib;
            stream_transform    color_transform;
            stream_transform    depth_transform;
        };

        enum initialize_status
        {
            not_initialized = 0,
            depth_initialized = 1,
            color_initialized = 2,
            both_initialized = 3
        };

        inline initialize_status operator|(initialize_status lhs, initialize_status rhs)
        {
            return static_cast<initialize_status>(static_cast<int>(lhs) | static_cast<int>(rhs));
        }

        class ds4_projection : public rs::utils::release_self_base<projection_interface>
        {
        public:
            ds4_projection(bool platformCameraProjection);
            virtual ~ds4_projection();
            virtual void reset();

            status init_from_float_array(r200_projection_float_array *data);

            /* projection */
            virtual status project_depth_to_camera(int32_t npoints, point3dF32 *pos_uvz, point3dF32 *pos3d);
            virtual status project_camera_to_depth(int32_t npoints, point3dF32 *pos3d, pointF32 *pos_uv);
            virtual status project_color_to_camera(int32_t npoints, point3dF32 *pos_ijz, point3dF32 *pos3d);
            virtual status project_camera_to_color(int32_t npoints, point3dF32 *pos3d, pointF32 *pos_ij);
            virtual status map_depth_to_color(int32_t npoints, point3dF32 *pos_uvz, pointF32  *pos_ij);
            virtual status map_color_to_depth(image_interface *depth, int32_t npoints, pointF32 *pos_ij, pointF32 *pos_uv);
            virtual status query_uvmap(image_interface *depth, pointF32 *uvmap);
            virtual status query_invuvmap(image_interface *depth, pointF32 *inv_uvmap);
            virtual status query_vertices(image_interface *depth, point3dF32 *vertices);
            virtual image_interface* create_color_image_mapped_to_depth(image_interface *depth, image_interface *color);
            virtual image_interface* create_depth_image_mapped_to_color(image_interface *depth, image_interface *color);

        private:
            ds4_projection(const ds4_projection&) = delete;
            ds4_projection& operator=(const ds4_projection&) = delete;

            status init(bool isMirrored);
            int distorsion_ds_lms(float* Kc, float* invdistc, float* distc);
            int projection_ds_lms12(float* r, float* t, float* ir, float* it);

            math_projection m_math_projection;

            bool              m_is_platform_camera_projection;
            initialize_status m_initialize_status;

            // below is minimum set of parameters for projection initialization
            sizeI32            m_depth_size; // Stored Depth camera size
            sizeI32            m_color_size; // Stored Color camera size
            bool               m_is_color_rectified;
            stream_calibration m_depth_calib;
            stream_transform   m_depth_transform;
            stream_calibration m_color_calib;
            stream_transform   m_color_transform;

            sizeI32            m_color_size_rectified; // always identical to m_colorSize?
            sizeI32            m_color_size_unrectified; // always identical to m_colorSize?
            stream_calibration m_color_calib_rectified;
            stream_calibration m_color_calib_unrectified;
            stream_transform   m_color_transform_rectified;
            stream_transform   m_color_transform_unrectified;

            float m_camera_depth_params[4];     // Internal Depth camera parameters
            float m_camera_color_params[4];     // Internal Color camera parameters
            float m_translation[3]; // Translation vector from Depth to Color camera

            // Used if unrectified color
            float m_rotation[9];    // Rotation matrix from Depth to Color camera
            float m_distorsion_color_coeffs[5]; // Color camera distortion coefficients
            float m_invdist_color_coeffs[5];    // Color camera inverse distortion coefficients
            float m_invrot_color[9];     // Rotation matrix from Color to Depth camera
            float m_invtrans_color[3];   // Translation vector from Color to Depth camera

            // internal buffers
            uint8_t               *m_projection_spec; // Projection spec buffer used in QueryUVMap and QueryVertices
            int                   m_projection_spec_size;// Projection spec buffer size
            pointF32              *m_buffer;         // Projection internal buffer
            int32_t               m_buffer_size;     // Projection internal buffer size
            std::recursive_mutex  m_cs_buffer;        // Projection internal buffer protection mutex
            std::vector<pointI32> m_step_buffer;
            pointI32              *m_sparse_invuvmap;
        };

    }
}
