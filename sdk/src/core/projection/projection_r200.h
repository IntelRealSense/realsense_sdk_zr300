// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
int memcpy_s(void *, size_t , const void *, size_t );

#include <mutex>
#define __STDC_WANT_LIB_EXT1__ 1
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include "projection.h"

namespace rs
{
    namespace core
    {

        projection* create_projection_ds4(bool isPlatformCameraProjection);

        // Memory allocation
#define ALIGN 32

        void *aligned_malloc(int size)
        {
            uint8_t *mem = (uint8_t*)malloc(size+ALIGN+sizeof(void*));
            void **ptr = (void**)((uintptr_t)(mem+ALIGN+sizeof(void*)) & ~(ALIGN-1));
            ptr[-1] = mem;
            return ptr;
        }

        void aligned_free(void *ptr)
        {
            free(((void**)ptr)[-1]);
        }


        //TODO: Fix the function. Potentiial issue inside.
        int memcpy_s(void *dest, size_t numberOfElements, const void *src, size_t count)
        {
            if (!count)
                return -1;

            if (src == NULL)
            {
                return -1;
            }
            if (count <= numberOfElements)
            {
                return -1;
            }

            memcpy(dest, src, count);
            return 0;
        }


        struct stream_transform
        {
            float translation[3];   /* The translation in mm of the camera coordinate system origin to the world coordinate system origin. The world coordinate system coincides with the depth camera coordinate system. */
            float rotation[3][3];   /* The rotation of the camera coordinate system with respect to the world coordinate system. The world coordinate system coincides with the depth camera coordinate system. */
        };

        struct stream_calibration
        {
            pointF32 focalLength;    /* The sensor focal length in pixels along the x and y axes. The parameters vary with the stream resolution setting. */
            pointF32 principalPoint; /*  The sensor principal point in pixels along the x and y axes. The parameters vary with the stream resolution setting. */
            float                      radialDistortion[3];     /*  The radial distortion coefficients, as described by camera model equations. */
            float                      tangentialDistortion[2]; /* The tangential distortion coefficients, as described by camera model equations. */
            // DeviceModel model; /* Defines the distortion model of the device - different device models may use different distortion models */
        };


        //TODO: Is it correct?
        //extern "C" PXCSessionService::DLLExportTable ExportTableProjectionDS4;

        struct r200_projection_float_array
        {
            float marker; // should be 12345.f
            float colorWidth;
            float colorHeight;
            float depthWidth;
            float depthHeight;
            float isColorRectified;
            float isMirrored;
            float reserved;
            // If isColorRectified=true, the following fields are enough to set in structures below
            //     colorCalib.focalLength.x
            //     colorCalib.focalLength.y
            //     colorCalib.principalPoint.x
            //     colorCalib.principalPoint.y
            //     depthCalib.focalLength.x
            //     depthCalib.focalLength.y
            //     depthCalib.principalPoint.x
            //     depthCalib.principalPoint.y
            //     depthTransform.translation[0]
            //     depthTransform.translation[1]
            //     depthTransform.translation[2]

            stream_calibration colorCalib;
            stream_calibration depthCalib;
            stream_transform   colorTransform;
            stream_transform   depthTransform;
        };

        enum initialize_status
        {
            NOT_INITIALIZED = 0,
            DEPTH_INITIALIZED = 1,
            COLOR_INITIALIZED = 2,
            BOTH_INITIALIZED = 3
        };

        inline initialize_status operator|(initialize_status lhs, initialize_status rhs)
        {
            return static_cast<initialize_status>(static_cast<int>(lhs) | static_cast<int>(rhs));
        }

        class ds4_projection : public projection
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
            virtual status map_color_to_depth(image_interface *d_image, int32_t npoints, pointF32 *posc, pointF32 *posd);
            virtual status query_uvmap(image_interface *depth, pointF32 *uvmap);
            virtual status query_invuvmap(image_interface *depth, pointF32 *inv_uvmap);
            virtual status query_vertices(image_interface *depth, point3dF32 *vertices);
            virtual image_interface* create_color_image_mapped_to_depth(image_interface *depth, image_interface *color);
            virtual image_interface* create_depth_image_mapped_to_color(image_interface *depth, image_interface *color);

        private:
            void unset();
            status init(bool isMirrored);
            status init_from_serialized_buffer(uint8_t *data, int dataSize);


            //PXCSession* m_session;
            //ProjectionWithSaturation m_projectionWithSaturation;

            bool                      m_isPlatformCameraProjection;
            initialize_status         m_initialize_status;

            // below is minimum set of parameters for projection initialization
            sizeI32 m_depthSize; // Stored Depth camera size
            sizeI32 m_colorSize; // Stored Color camera size
            bool                      m_isColorRectified;
            stream_calibration        m_depthCalib;
            stream_transform          m_depthTransform;
            stream_calibration        m_colorCalib;
            stream_transform          m_colorTransform;

            sizeI32 m_colorSizeRectified; // always identical to m_colorSize?
            sizeI32 m_colorSizeUnrectified; // always identical to m_colorSize?
            stream_calibration        m_colorCalibRectified;
            stream_calibration        m_colorCalibUnrectified;
            stream_transform          m_colorTransformRectified;
            stream_transform          m_colorTransformUnrectified;


            float m_cameraD[4];     // Internal Depth camera parameters
            float m_cameraC[4];     // Internal Color camera parameters
            float m_translation[3]; // Translation vector from Depth to Color camera

            // Used if unrectified color
            float m_rotation[9];    // Rotation matrix from Depth to Color camera
            float m_distorsionC[5]; // Color camera distortion coefficients
            float m_invdistC[5];    // Color camera inverse distortion coefficients
            float m_invrotC[9];     // Rotation matrix from Color to Depth camera
            float m_invtransC[3];   // Translation vector from Color to Depth camera

            // internal buffers
            uint8_t                    *m_projectionSpec; // Projection spec buffer used in QueryUVMap and QueryVertices
            int                        m_projectionSpecSize;// Projection spec buffer size
            pointF32 *m_buffer;         // Projection internal buffer
            int32_t                    m_bufferSize;     // Projection internal buffer size
            std::recursive_mutex       cs_buffer;        // Projection internal buffer protection mutex
        };

    }
}
