// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <sstream>
#include <memory>
#include "projection_r200.h"
//Preventing pragma no vector from breaking debug build
#pragma warning (disable : 4068)

#include "custom_image.h"
#include "src/utilities/image/image_utils.h"
#include "librealsense/rs.h"


#ifdef USE_IPP
#include "ipp.h"
#include "ipprs.h"
#else
#include "rs_math.h"
#endif

using namespace rs::core;
using namespace rs::utils;

//Right dependencies
#define PXC_COLOR_FORMAT_UVMAP                  ((image::PixelFormat)(0x40000002)) // UVMAP attached to Depth image

#undef  CHECK
#define CHECK(_EXPR_, _STATUS_)                     \
    if (!(_EXPR_)) {                                    \
        /*assert(! #_EXPR_);*/                          \
        /*LOG_ERROR("failed on " #_EXPR_); */        \
        return _STATUS_;                                \
    }

#define CHECK_STS(_FUNC, _RET)                      \
{                                                       \
    Status _sts = _FUNC;                             \
    if (_sts < 0)                               \
    {                                                   \
        /*LOG_ERROR("Error " << StatusToCharString(_sts) << " on " #_FUNC);*/ \
        return _RET;                                    \
    }                                                   \
}


ds4_projection::ds4_projection( bool platformCameraProjection) :
    m_buffer(nullptr),
    m_initialize_status(NOT_INITIALIZED),
    m_isPlatformCameraProjection(platformCameraProjection),
    m_projectionSpec(nullptr),
    m_projectionSpecSize(0)
{
    // Initialization of IPP and all internal buffers
#ifdef USE_IPP
    auto Status = ippInit();
    if (Status != STATUS_NO_ERROR)
    {
        //TODO: Print warning
        //LOG_WARN_CFORMAT("ippInit failed: %d", Status);
    }
#endif
    reset();
}

ds4_projection::~ds4_projection()
{
    reset();
}

void ds4_projection::reset()
{
    // Unset of projection module:
    // Lock mutex -> free memory -> unlock mutes
    std::lock_guard<std::recursive_mutex> auto_lock(cs_buffer);
    memset(m_distorsionC, 0, sizeof(m_distorsionC));
    if (m_projectionSpec) aligned_free(m_projectionSpec);
    m_projectionSpec = nullptr;
    m_projectionSpecSize = 0;
    if (m_buffer) aligned_free(m_buffer);
    m_buffer = nullptr;
    m_bufferSize = 0;
}

int distorsion_ds_lms(float* Kc, float* invdistc, float* distc)
{
    status sts;
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

    double *A = (double*)aligned_malloc(2 * 5 * cnt*2 * sizeof(float)); APitch = sizeof(float);
    double *b = (double*)aligned_malloc(sizeof(double)*cnt*2);
    if (!A || !b)
    {
        if (A) aligned_free(A);
        if (b) aligned_free(b);
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
    sts = rsQRDecomp_m_64f(A, APitch, sizeof(double), pBuffer, pDecomp, APitch, sizeof(double), 5, cnt);
    if (sts)
    {
        if (A) aligned_free(A);
        if (b) aligned_free(b);
        return -1;
    }

    // Solve overdetermined equation system
    sts = rsQRBackSubst_mva_64f(pDecomp, APitch, sizeof(double), pBuffer, b, cnt * sizeof(double), sizeof(double),
                                dst, 5 * sizeof(double), sizeof(double), 5, cnt, 1);
#pragma warning( default: 4996 )
    if (A) aligned_free(A);
    if (b) aligned_free(b);
    if (sts) return -1;

    // Copy overdetermined equation system solution to output buffer
    for (i = 0; i < 5; i++) distc[i] = (float)dst[i];

    return 0;
}

// Find inverse projection matrix
int projection_ds_lms12(float* r, float* t, float* ir, float* it)
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

    double *A = (double*)aligned_malloc(2 * 12 * cnt*2 * sizeof(float)); APitch = sizeof(float);
    memset( A, 0, APitch * cnt * 2 );
    double *b = (double*)aligned_malloc(sizeof(double)*cnt*2);
    if (!A || !b)
    {
        if (A) aligned_free(A);
        if (b) aligned_free(b);
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
    sts = rsQRDecomp_m_64f(A, APitch, sizeof(double), pBuffer, pDecomp, APitch, sizeof(double), 12, cnt);
    if (sts)
    {
        if (A) aligned_free(A);
        if (b) aligned_free(b);
        return -1;
    }

    // Solve overdetermined equation system
    sts = rsQRBackSubst_mva_64f(pDecomp, APitch, sizeof(double), pBuffer, b, cnt * sizeof(double), sizeof(double),
                                dst, 12 * sizeof(double), sizeof(double), 12, cnt, 1);
#pragma warning( default: 4996 )
    if (A) aligned_free(A);
    if (b) aligned_free(b);
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

status ds4_projection::init_from_float_array(r200_projection_float_array *data)
{
    // Copy Calibratioon data to internal buffers
    m_colorSize.width = static_cast<int>(data->colorWidth);
    m_colorSize.height = static_cast<int>(data->colorHeight);
    m_depthSize.width = static_cast<int>(data->depthWidth);
    m_depthSize.height = static_cast<int>(data->depthHeight);
    m_isColorRectified = static_cast<int>(data->isColorRectified) ? true : false;
    bool isMirrored = static_cast<int>(data->isMirrored) ? true : false;
    m_colorCalib = data->colorCalib;
    m_depthCalib = data->depthCalib;
    m_colorTransform = data->colorTransform;
    m_depthTransform = data->depthTransform;

    //m_depthCalib.model = m_isPlatformCameraProjection ? Capture::DEVICE_MODEL_R200_ENHANCED : Capture::DEVICE_MODEL_R200;
    //m_colorCalib.model = m_depthCalib.model;
    m_colorSizeRectified = m_colorSize;
    m_colorSizeUnrectified = m_colorSize;
    m_colorCalibRectified = m_colorCalib;
    m_colorCalibUnrectified = m_colorCalib;
    m_colorTransformRectified = m_colorTransform;
    m_colorTransformUnrectified = m_colorTransform;

    return init(isMirrored);
}

status ds4_projection::init(bool isMirrored)
{
    m_initialize_status = NOT_INITIALIZED;

    // Check correct camera initialization for Depth and Color cameras separately
    if (m_depthSize.width && m_depthSize.height)
        m_initialize_status = m_initialize_status | DEPTH_INITIALIZED;

    if ((!m_isColorRectified || m_colorSizeRectified.width) &&
            (!m_isColorRectified || m_colorSizeRectified.height) &&
            (m_isColorRectified || m_colorSizeUnrectified.width) &&
            (m_isColorRectified || m_colorSizeUnrectified.height) &&
            m_colorSize.width &&
            m_colorSize.height
       )
    {
        m_initialize_status = m_initialize_status | COLOR_INITIALIZED;
    }

    if (!m_initialize_status)
        return status_handle_invalid;

    // Set coordinate system for future axis direction flipping
    bool flipY = false ;

    // Extract internal Depth camera parameters from DS4 calibration params
    m_cameraD[0] = m_depthCalib.focalLength.x;
    m_cameraD[1] = m_depthCalib.principalPoint.x;
    m_cameraD[2] = m_depthCalib.focalLength.y;
    m_cameraD[3] = m_depthCalib.principalPoint.y;

    // Extract translation vector from DS4 calibration params
    m_translation[0] = m_depthTransform.translation[0];
    m_translation[1] = m_depthTransform.translation[1];
    m_translation[2] = m_depthTransform.translation[2];

    if (flipY)
        m_translation[1] = -m_translation[1];

    // Update internal Depth camera parameters in mirror mode case
    if (isMirrored)
    {
        m_cameraD[0] = -m_cameraD[0];
        m_cameraD[1] = (float)m_depthSize.width - 1.f - m_cameraD[1];
    }

    if (flipY)
        m_cameraD[2] = -m_cameraD[2];

    // IPP Projection initialization
    int projectionSpecSize;

    rs::core::sizeI32 depthSize = {m_depthSize.width, m_depthSize.height};
    rsProjectionGetSize_32f(depthSize, &projectionSpecSize );
    if( m_projectionSpecSize < projectionSpecSize )
    {
        m_projectionSpec = (uint8_t *)aligned_malloc(sizeof(uint8_t) * projectionSpecSize);
        m_projectionSpecSize = projectionSpecSize;
        memset(m_projectionSpec, 0, projectionSpecSize);
    }
    rsProjectionInit_32f( depthSize, m_cameraD, NULL, (ProjectionSpec_32f*)m_projectionSpec );

    // Extract internal Color camera parameters from DS4 calibration params
    m_cameraC[0] = m_colorCalib.focalLength.x;
    m_cameraC[1] = m_colorCalib.principalPoint.x;
    m_cameraC[2] = m_colorCalib.focalLength.y;
    m_cameraC[3] = m_colorCalib.principalPoint.y;

    if (m_isColorRectified)
    {
        // Update internal Color camera parameters in mirror mode case and rectified Color
        if (isMirrored)
        {
            m_cameraC[0] = -m_cameraC[0];
            m_cameraC[1] = (float)m_colorSizeRectified.width - 1.f - m_cameraC[1];
        }
        if (flipY) m_cameraC[2] = -m_cameraC[2];
    }
    else
    {
        // Update internal Color camera parameters in mirror mode case and nonrectified Color
        if (isMirrored)
        {
            m_cameraC[0] = -m_cameraC[0];
            m_cameraC[1] = (float)m_colorSizeUnrectified.width - 1.f - m_cameraC[1];
        }
        //Workaround for compiler incorrect optimization
#pragma novector
        memcpy_s(m_rotation, sizeof(m_rotation), m_depthTransform.rotation, 9 * sizeof(float));
        if (flipY)
        {
            m_rotation[3] = -m_rotation[1];
            m_rotation[4] = -m_rotation[4];
            m_rotation[5] = -m_rotation[7];
        }
        projection_ds_lms12(m_rotation, m_translation, m_invrotC, m_invtransC);
        //Workaround for compiler incorrect optimization
#pragma novector
        // Color distorsion coefficiens calculation
        float distortion[5] =
        {
            m_colorCalib.radialDistortion[0],
            m_colorCalib.radialDistortion[1],
            m_colorCalib.tangentialDistortion[0],
            m_colorCalib.tangentialDistortion[1],
            m_colorCalib.radialDistortion[2]
        };
        if (memcmp(m_distorsionC, distortion, 5 * sizeof(float)))
        {
            memcpy_s(m_distorsionC, sizeof(m_distorsionC), distortion, 5 * sizeof(float));
            float camera[4] =
            {
                m_colorCalib.focalLength.x * 2.f / m_colorSizeUnrectified.width,
                m_colorCalib.principalPoint.x * 2.f / m_colorSizeUnrectified.width - 1.f,
                m_colorCalib.focalLength.y * 2.f / m_colorSizeUnrectified.height,
                m_colorCalib.principalPoint.y * 2.f / m_colorSizeUnrectified.height - 1.f
            };
            distorsion_ds_lms(camera, m_distorsionC, m_invdistC);
        }
    }

    // m_projectionWithSaturation.SetResolution(m_colorSize.width, m_colorSize.height, m_depthSize.width, m_depthSize.height);

    return status_no_error;
};

status  ds4_projection::project_depth_to_camera(int32_t npoints, point3dF32 *pos_uvz, point3dF32 *pos3d)
{
    CHECK(npoints, status_handle_invalid);
    CHECK((npoints >= 0), status_param_unsupported);
    CHECK(pos_uvz, status_handle_invalid);
    CHECK(pos3d, status_handle_invalid);
    if (!(m_initialize_status & DEPTH_INITIALIZED)) return status_data_unavailable;

    // Projection of an array of 3D points from Depth image coordinates to World
    rs3DArrayProjection_32f((const float*)pos_uvz, (float*)pos3d, npoints, m_cameraD, NULL, NULL, NULL, NULL, NULL);

    return status_no_error;
}

status  ds4_projection::project_camera_to_depth(int32_t npoints, point3dF32 *pos3d, pointF32 *pos_uv)
{
    CHECK((npoints >= 0), status_param_unsupported);
    CHECK(pos3d, status_handle_invalid);
    CHECK(pos_uv, status_handle_invalid);
    if (!(m_initialize_status & DEPTH_INITIALIZED)) return status_data_unavailable;

    // Projection of an array of 3D points from World to Depth image coordinates
    auto result = rs3DArrayProjection_32f((const float*)pos3d, (float*)pos_uv, npoints, nullptr, nullptr, nullptr, nullptr, nullptr, m_cameraD);
    if(result != status_no_error)
    {
        return status_param_unsupported;
    }

    return status_no_error;
};


status  ds4_projection::project_color_to_camera(int32_t npoints, point3dF32 *pos_ijz, point3dF32 *pos3d)
{
    CHECK(npoints, status_handle_invalid);
    CHECK((npoints >= 0), status_param_unsupported);
    CHECK(pos_ijz, status_handle_invalid);
    CHECK(pos3d, status_handle_invalid);
    if (!(m_initialize_status & COLOR_INITIALIZED)) return status_data_unavailable;

    // Projection of an array of 3D points from Color image coordinates to World
    float translationC[3] = { -m_translation[0], -m_translation[1], -m_translation[2] };
    if ( m_isColorRectified )
    {
        rs3DArrayProjection_32f((const float*)pos_ijz, (float*)pos3d, npoints, m_cameraC, NULL, NULL, translationC, NULL, NULL);
    }
    else
    {
        rs3DArrayProjection_32f( (const float*)pos_ijz, (float*)pos3d, npoints, m_cameraC, m_invdistC, m_invrotC, m_invtransC, NULL, NULL );
    }

    return status_no_error;
};


status  ds4_projection::project_camera_to_color(int32_t npoints, point3dF32 *pos3d, pointF32 *pos_ij)
{
    CHECK((npoints >= 0), status_param_unsupported);
    CHECK(pos3d, status_handle_invalid);
    CHECK(pos_ij, status_handle_invalid);
    if (!(m_initialize_status & COLOR_INITIALIZED)) return status_data_unavailable;

    // Projection of an array of 3D points from World to Color image coordinates
    if ( m_isColorRectified )
    {
        auto result = rs3DArrayProjection_32f((const float*)pos3d, (float*)pos_ij, npoints, nullptr, nullptr, nullptr, m_translation, nullptr, m_cameraC);
        if (result != status_no_error)
        {
            return status_param_unsupported;
        }
    }
    else
    {
        // If Color image is not rectified, we should assume rotation and distorsion of color image
        auto result = rs3DArrayProjection_32f((const float*)pos3d, (float*)pos_ij, npoints, nullptr, nullptr, m_rotation, m_translation, m_distorsionC, m_cameraC);
        if (result != status_no_error)
        {
            return status_param_unsupported;
        }
    }

    return status_no_error;
};


//////////////////////////////////////////////////////////////////////////////////////////////////////
// Query Map/Vertices
status  ds4_projection::query_uvmap(image_interface *depth, pointF32 *uvmap)
{
    CHECK(depth, status_handle_invalid);
    CHECK(uvmap, status_handle_invalid);
    if (m_initialize_status != BOTH_INITIALIZED) return status_data_unavailable;
    status sts;

    image_info info = depth->query_info();
    const void * data;

    //acquire access to depth as ds4 does not have uvmap built in
    data = depth->query_data();

    if ( !data )
    {
        return status_data_not_initialized;
    }

    pointF32* dst_plane;
    int dst_pitches;

    dst_plane = (pointF32*)uvmap;
    dst_pitches = info.width * 8;
    sts = status_no_error;

    rs::core::sizeI32 m_depthSize = { info.width, info.height };
    float invW = 1.f / (float)m_colorSize.width;
    float invH = 1.f / (float)m_colorSize.height;
    float cameraC[4] = { m_cameraC[0] * invW, m_cameraC[1] * invW, m_cameraC[2] * invH, m_cameraC[3] * invH };
    // Call projection function for fast UVMap calculataion from Depth data
    if ( m_isColorRectified )
    {
        if ( status_param_unsupported  == rsProjection_16u32f_C1CXR( (const unsigned short*)data, m_depthSize, depth->query_info().pitch, (float*)dst_plane, dst_pitches,
                NULL, m_translation, NULL, cameraC, (const ProjectionSpec_32f*)m_projectionSpec ) )
        {
            return status_feature_unsupported;
        }
    }
    else
    {
        // If Color image is not rectified, we should assume rotation and distorsion of color image
        if ( status_param_unsupported  == rsProjection_16u32f_C1CXR( (const unsigned short*)data, m_depthSize, depth->query_info().pitch, (float*)dst_plane, dst_pitches,
                m_rotation, m_translation, m_distorsionC, cameraC, (const ProjectionSpec_32f*)m_projectionSpec ) )
        {
            return status_feature_unsupported;
        }
    }

    rsUVmapFilter_32f_C2IR( (float*)dst_plane, dst_pitches, m_depthSize, NULL, 0, 0 );

    return sts;
};


status  ds4_projection::query_invuvmap(image_interface *depth, pointF32 *inv_uvmap)
{
    CHECK(inv_uvmap, status_handle_invalid);
    CHECK(depth, status_handle_invalid);
    if (m_initialize_status != BOTH_INITIALIZED) return status_data_unavailable;

    // UVMap getting here
    pointF32* src_plane;
    int src_pitches;

    pointF32* uvmap = new pointF32[depth->query_info().width*depth->query_info().height];
    status sts = query_uvmap(depth, uvmap);
    src_plane = uvmap;
    src_pitches = depth->query_info().width * 8;

    // Find inverse UVMap
    image_info info = depth->query_info();
    rs::core::sizeI32 dsize = {info.width, info.height};
    rs::core::sizeI32 csize = {m_colorSize.width, m_colorSize.height};
    rect uvMapRoi = {0, 0, info.width, info.height};
    // Call  projection function for fast Inverse UVMap calculataion from UVMap data
    if( status_no_error != rsUVmapInvertor_32f_C2R( (float*)src_plane, src_pitches, dsize, uvMapRoi, (float*)inv_uvmap, csize.width * sizeof(pointF32), csize, 1 ) )
        sts = status_feature_unsupported;

    delete[] uvmap;
    return sts;
};


status  ds4_projection::query_vertices(image_interface *depth, point3dF32 *vertices)
{
    CHECK(depth, status_handle_invalid);
    CHECK(vertices, status_handle_invalid);
    if (!(m_initialize_status & DEPTH_INITIALIZED)) return status_data_unavailable;

    image_info info = depth->query_info();
    const void * data;

    //acquire access to depth as ds4 does not have uvmap built in
    data = depth->query_data();
    if ( !data ) return status_data_unavailable;

    rs::core::sizeI32 m_depthSize = { info.width, info.height };
    // Call projection function for fast Vertices calculataion from Depth data
    rsProjection_16u32f_C1CXR( (const unsigned short*)data, m_depthSize, depth->query_info().pitch, (float*)vertices, m_depthSize.width * sizeof(point3dF32),
                               NULL, NULL, NULL, NULL, (const ProjectionSpec_32f*)m_projectionSpec );


    return status_no_error;
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Map
status ds4_projection::map_depth_to_color(int32_t npoints, point3dF32 *pos_uvz, pointF32  *pos_ij)
{
    CHECK((npoints >= 0), status_param_unsupported);
    CHECK(pos_uvz, status_handle_invalid);
    CHECK(pos_ij, status_handle_invalid);
    if (m_initialize_status != BOTH_INITIALIZED) return status_data_unavailable;

    status sts = status_no_error;
    // Call 3D array projection function from mapping Depth to Color images
    if (m_isColorRectified)
    {
        auto result = rs3DArrayProjection_32f((const float*)pos_uvz, (float*)pos_ij, npoints, m_cameraD, nullptr, nullptr, m_translation, nullptr, m_cameraC);
        if (result != status_no_error)
        {
            sts = status_param_unsupported;
        }
    }
    else
    {
        // If Color image is not rectified, we should assume rotation and distorsion of color image
        auto result = rs3DArrayProjection_32f((const float*)pos_uvz, (float*)pos_ij, npoints, m_cameraD, nullptr, m_rotation, m_translation, m_distorsionC, m_cameraC);
        if (result != status_no_error)
        {
            sts = status_param_unsupported;
        }
    }

    return sts;
};


#define x64_ALIGNMENT(x) (((x)+0x3f)&0xffffffc0)
struct depthUVPoint
{
    float u, v;
    int16_t i, j;
};
status  ds4_projection::map_color_to_depth(image_interface *depth, int32_t npoints, pointF32 *pos_ij, pointF32 *pos_uv)
{
    CHECK(npoints, status_handle_invalid);
    CHECK(depth, status_handle_invalid);
    CHECK((npoints>= 0), status_param_unsupported);
    CHECK(pos_ij, status_handle_invalid);
    CHECK(pos_uv, status_handle_invalid);

    status sts;

    if (m_initialize_status != BOTH_INITIALIZED) return status_data_unavailable;

    image_info dinfo = depth->query_info();

    // Lock auxiliary projection buffer mutex guard
    std::lock_guard<std::recursive_mutex> auto_lock(cs_buffer);
    // Aauxiliary projection buffer reallocation
    int32_t depthPoints = x64_ALIGNMENT( dinfo.width * dinfo.height * sizeof(depthUVPoint) );
    if( depthPoints > m_bufferSize )
    {
        if( m_buffer ) aligned_free( m_buffer );
        m_buffer = 0;
        m_buffer = (pointF32*)aligned_malloc( sizeof(pointF32) * depthPoints );
        if(!m_buffer)
        {
            return status_feature_unsupported;
        }
        m_bufferSize = depthPoints;
    }

    // UVMap getting here
    pointF32* uvmap = new pointF32[dinfo.width*dinfo.height];
    query_uvmap(depth, uvmap);
    uint8_t *pUV = (uint8_t*)uvmap;
    int32_t uvStep = dinfo.width * 8;
    sts = status_no_error;

    // Find inverse UVMap

    pointF32 *uvTest;
    depthUVPoint *duvPoint = (depthUVPoint*)m_buffer;
    int dpoints = 0;

    // Filtering UVMap value according Depth value
    for( int j = 0; j < dinfo.height; j ++ )
    {
        for ( int i = 0; i < dinfo.width; i++ )
        {
            uvTest = ((pointF32*)pUV) + i;
            if( uvTest->x >= 0.f && uvTest->x < 1.f && uvTest->y >= 0.f && uvTest->y < 1.f )
            {
                duvPoint[dpoints].u = uvTest->x;
                duvPoint[dpoints].v = uvTest->y;
                duvPoint[dpoints].i = (int16_t)i;
                duvPoint[dpoints].j = (int16_t)j;
                dpoints ++;
            }
        }
        pUV += uvStep;
    }

    float cwidth_ = 1.f / m_colorSize.width;
    float cheight_ = 1.f / m_colorSize.height;
    float oR, oC, prodX, prodY;
    double minD, r;
    double minDMax = 2. * ( (double)cwidth_*cwidth_ + (double)cheight_*cheight_ ); // 2 pixels error available for depth scaled image

    // Find nearest point from input color array to projected from Depth to Color image points
    for(int n = 0; n < npoints; n++)
    {
        pointF32 tmpPosc = pos_ij[n];

        tmpPosc.x *= cwidth_;
        tmpPosc.y *= cheight_;
        minD = minDMax;
        oR = oC = -1.0f;	// Default values, assuming not found
        for(int d = 0; d < dpoints; d++)
        {
            prodX = tmpPosc.x - duvPoint[d].u;
            prodY = tmpPosc.y - duvPoint[d].v;
            r = (double)prodX * prodX + (double)prodY * prodY;

            // Find and store the nearest point from UVMap
            if (r < minD)
            {
                minD = r;
                oC = duvPoint[d].i;
                oR = duvPoint[d].j;
            }
        }

        if (oR == -1.0f )
        {
            pos_uv[n].x = pos_uv[n].y = -1.0f;
            sts = status_value_out_of_range;
        }
        else
        {
            pos_uv[n].x = oC;
            pos_uv[n].y = oR;
        }
    }

    delete[] uvmap;
    return sts;
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Create images
image_interface *ds4_projection::create_color_image_mapped_to_depth(image_interface *depth, image_interface *color)
{
    CHECK(depth, 0);
    CHECK(color, 0);
    //TODO: return error for Linux
    if (false)
    {
        return nullptr;
    }
    uint8_t defaultColor[4] = {0, 0, 0, 0};

    image_info dInfo = depth->query_info();
    image_info cInfo = color->query_info();
    image_info c2dInfo;

    memset( &c2dInfo, 0, sizeof(c2dInfo) );
    c2dInfo.width  = dInfo.width;
    c2dInfo.height = dInfo.height;
    c2dInfo.format = cInfo.format;
    c2dInfo.pitch = dInfo.width * 3; //TODO: Calculate automatically

    // Check color image format and try to create a new image with the same format
    // Ovrewise creates PIXEL_FORMAT_RGB24 and acquire Color image with convertion to PIXEL_FORMAT_RGB24


    //TODO: Fix format convertation
    smart_ptr<const image_interface> converted_image;
    color->convert_to(pixel_format::bgr8, converted_image);

    //TODO:: Fix memory allocation
    custom_image c2d (&c2dInfo,
                      nullptr,
                      stream_type::color,
                      image_interface::flag::any,
                      0,
                      nullptr, nullptr);

    uint8_t * c2dDat = reinterpret_cast<uint8_t *> (const_cast<void*> (c2d.query_data()));
    if ( !c2dDat ) return 0;

    // UVMap getting here
    pointF32* uvmap = new pointF32[dInfo.width*dInfo.height];
    query_uvmap(depth, uvmap);

    int32_t uvStep    = dInfo.width * 8;
    uint8_t *pUV       = (uint8_t*)uvmap;

    int32_t colorStep = color->query_info().pitch;
    uint8_t *pColor    = reinterpret_cast<uint8_t *> (const_cast<void*> (color->query_data()));

    int32_t c2dStep   = c2d.query_info().pitch;
    uint8_t *pC2d      = reinterpret_cast<uint8_t *> (const_cast<void*> (c2d.query_data()));

    pointF32 *uvTest;
    uint8_t *tmpColor;
    int32_t x, cWidth  = cInfo.width;
    int32_t y, cHeight = cInfo.height;
    rs::core::sizeI32 roi = {dInfo.width, dInfo.height};

    if( c2dInfo.format == pixel_format::bgr8 )
    {
        // Remap according UVMap date and PIXEL_FORMAT_RGB24 color image format
        int32_t xi, colorComponents = 3;
        // Set default color on image

        memset(c2dDat, 0, c2d.query_info().pitch*roi.height*3*sizeof(uint8_t));
        for( y = 0; y < dInfo.height; y ++ )
        {
            for ( x = 0, xi = 0; x < dInfo.width; x++, xi += colorComponents )
            {
                uvTest = ((pointF32*)pUV) + x;
                if( uvTest->x >= 0.f && uvTest->x < 1.f && uvTest->y >= 0.f && uvTest->y < 1.f )
                {
                    tmpColor = &pColor[(int)(uvTest->y * cHeight) * colorStep + colorComponents * (int)(uvTest->x * cWidth)];
                    pC2d[xi]   = tmpColor[0];
                    pC2d[xi+1] = tmpColor[1];
                    pC2d[xi+2] = tmpColor[2];
                }
            }
            pUV  += uvStep;
            pC2d += c2dStep;
        }
    }
    else if( c2dInfo.format == pixel_format::bgra8  )     // c2dInfo.format == image::PIXEL_FORMAT_RGB32
    {
        // Remap according UVMap date and PIXEL_FORMAT_RGB32 color image format
        int32_t xi, colorComponents = 4;
        // Set default color on image

        memset(c2dDat, 0, c2d.query_info().pitch*roi.height*4*sizeof(uint8_t));
        for( y = 0; y < dInfo.height; y ++ )
        {
            for ( x = 0, xi = 0; x < dInfo.width; x++, xi += colorComponents )
            {
                uvTest = ((pointF32*)pUV) + x;
                if( uvTest->x >= 0.f && uvTest->x < 1.f && uvTest->y >= 0.f && uvTest->y < 1.f )
                {
                    tmpColor = &pColor[(int)(uvTest->y * cHeight) * colorStep + colorComponents * (int)(uvTest->x * cWidth)];
                    pC2d[xi]   = tmpColor[0];
                    pC2d[xi+1] = tmpColor[1];
                    pC2d[xi+2] = tmpColor[2];
                    pC2d[xi+3] = tmpColor[3];
                }
            }
            pUV  += uvStep;
            pC2d += c2dStep;
        }
    }
    else     // c2dInfo.format == image::PIXEL_FORMAT_Y8
    {
        // Remap according UVMap date and PIXEL_FORMAT_Y8 color image format
        // Set default color on image
        memset(c2dDat, 0, c2d.query_info().pitch*roi.height*1*sizeof(uint8_t));
        for( y = 0; y < dInfo.height; y ++ )
        {
            for ( x = 0; x < dInfo.width; x++ )
            {
                uvTest = ((pointF32*)pUV) + x;
                if( uvTest->x >= 0.f && uvTest->x < 1.f && uvTest->y >= 0.f && uvTest->y < 1.f )
                    pC2d[x] = pColor[(int)(uvTest->y * cHeight) * colorStep + (int)(uvTest->x * cWidth)];
            }
            pUV  += uvStep;
            pC2d += c2dStep;
        }
    }

    delete[] uvmap;

    return nullptr; //&c2d;

}


image_interface* ds4_projection::create_depth_image_mapped_to_color(image_interface *depth, image_interface *color)
{
    CHECK(depth, 0);
    CHECK(color, 0);
    //TODO: return error for Linux
    if (false)
    {
        return nullptr;
    }
    unsigned short defaultDepth = 0;

    image_info dInfo = depth->query_info();
    image_info cInfo = color->query_info();
    image_info d2cInfo;

    memset( &d2cInfo, 0, sizeof(d2cInfo) );
    d2cInfo.width  = cInfo.width;
    d2cInfo.height = cInfo.height;
    d2cInfo.format =  dInfo.format;
    d2cInfo.pitch  =  cInfo.width * 2; //TODO: Calculate automatically

    //TODO:: Fix memory allocation
    custom_image d2c (&d2cInfo,
                      nullptr,
                      stream_type::depth,
                      image_interface::flag::any,
                      0,
                      nullptr,
                      nullptr);

    uint16_t * d2cDat = reinterpret_cast<uint16_t *> (const_cast<void*> (d2c.query_data()));
    if ( !d2cDat ) return 0;


    uint16_t * depthDat = reinterpret_cast<uint16_t *> (const_cast<void*> (depth->query_data()));

    // UVMap getting here
    pointF32* uvmap = new pointF32[dInfo.width*dInfo.height];
    query_uvmap(depth, uvmap);

    // Find inverse UVMap
    // Lock auxiliary projection buffer mutex guard
    std::lock_guard<std::recursive_mutex> auto_lock(cs_buffer);
    // Aauxiliary projection buffer reallocation
    int32_t invuvmapPoints = x64_ALIGNMENT( cInfo.width * cInfo.height * sizeof(pointF32) );
    if( invuvmapPoints > m_bufferSize )
    {
        if( m_buffer ) aligned_free( m_buffer );
        m_buffer = 0;
        m_buffer = (pointF32*)aligned_malloc( sizeof(pointF32) * invuvmapPoints );

        if(!m_buffer)
        {
            return 0;
        }
        m_bufferSize = invuvmapPoints;
    }

    // Inverse UVMap getting here
    rs::core::sizeI32 dsize = {dInfo.width, dInfo.height};
    rs::core::sizeI32 csize = {cInfo.width, cInfo.height};
    rect uvMapRoi = {0, 0, dInfo.width, dInfo.height};
    rsUVmapInvertor_32f_C2R( (float*)(uint8_t*)uvmap, dInfo.width * 8, dsize, uvMapRoi, (float*)m_buffer, cInfo.width * sizeof(pointF32), csize, 0 );
    // Remap Depth data according Inverse UVMap
    rsRemap_16u_C1R( (unsigned short*)depthDat, dsize, depth->query_info().pitch,
                     (float*)m_buffer, cInfo.width * sizeof(pointF32),
                     (unsigned short*)d2cDat, csize, d2c.query_info().pitch, 0, defaultDepth );

    delete[] uvmap;

    return nullptr; //&d2c;

}



extern "C" {
    stream_calibration convert_intrinsics(rs_intrinsics* intrin)
    {
        stream_calibration calib = {};
        //calib.model = Capture::DEVICE_MODEL_R200;
        // libRealsense returns these arguments denominated by 2 for case of intrinsics[THIRD_QRES]
        // TODO check what is it
        calib.focalLength.x = intrin->fx;
        calib.focalLength.y = intrin->fy;
        calib.principalPoint.x = intrin->ppx;
        calib.principalPoint.y = intrin->ppy;

        calib.radialDistortion[0] = intrin->coeffs[0];
        calib.radialDistortion[1] = intrin->coeffs[1];
        calib.tangentialDistortion[0] = intrin->coeffs[2];
        calib.tangentialDistortion[1] = intrin->coeffs[3];
        calib.radialDistortion[2] = intrin->coeffs[4];
        return calib;
    }

    extern void* rs_projection_create_instance_from_intrinsics_extrinsics(rs_intrinsics *colorIntrinsics, rs_intrinsics *depthIntrinsics, rs_extrinsics *extrinsics)
    {
        CHECK(colorIntrinsics, nullptr);
        CHECK(depthIntrinsics, nullptr);
        CHECK(extrinsics, nullptr);

        ds4_projection* proj = new ds4_projection(false);
        r200_projection_float_array calib = {};
        calib.marker = 12345.f;
        calib.colorWidth = (float)colorIntrinsics->width;
        calib.colorHeight = (float)colorIntrinsics->height;
        calib.depthWidth = (float)depthIntrinsics->width;
        calib.depthHeight = (float)depthIntrinsics->height;
        calib.isColorRectified = 1; // for case of request for the RS_STREAM_COLOR
        calib.isMirrored = 0;
        calib.reserved = 0;
        calib.colorCalib = convert_intrinsics(colorIntrinsics);
        calib.depthCalib = convert_intrinsics(depthIntrinsics);


        memcpy(calib.depthTransform.translation, extrinsics->translation, 3 * sizeof(float));
        memcpy(calib.depthTransform.rotation, extrinsics->rotation, 9 * sizeof(float));
        // for some reason translation in DS4 Projection was expressed in millimeters (not in meters)
        calib.depthTransform.translation[0] *= 1000;
        calib.depthTransform.translation[1] *= 1000;
        calib.depthTransform.translation[2] *= 1000;
        proj->init_from_float_array(&calib);
        return proj;
    }
}
