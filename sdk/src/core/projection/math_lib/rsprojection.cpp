// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "rs_math.h"

/* ///////////////////////////////////////////////////////////////////////////////////////
//  Name:                      rsProjectWorldToImage_32f_C1R
//
//  Purpose:                   Transforms vector coefficients according to the given data
//
//  Returns:
//    STATUS_NO_ERROR              Indicates no error.
//    STATUS_HANDLE_INVALID         Indicates an error when one of the pointers is NULL.
//    STATUS_DATA_NOT_INITIALIZED            Indicates an error when length is negative, or equal to zero.
//
//  Parameters:
//    pSrc                     Pointer to source data
//    pDst                     Pointer to destination data
//    length                   Number of point
//    cameraSrc                Pointer to source camera coefficients
//    invDistortion            Pointer to inverse distortion coefficients
//    rotation                 Pointer to rotation coefficients
//    translation              Pointer to translation coefficients
//    distortion               Pointer to destination RGB data
//    cameraDst                Pointer to destination camera coefficients
//
*/

//Added
status REFCALL rs3DArrayProjection_32f (const float *pSrc, float *pDst, int length, float cameraSrc[4],
                                        float invDistortionSrc[5], float rotation[9], float translation[3], float distortionDst[5], float cameraDst[4])
{

    status sts = status_no_error;
    signed int n = 0;
    double invFx, invFy;
    const float* src = (float*) pSrc;
    float* dst = (float*) pDst;
    int dstStep = 3;
    if( cameraDst ) dstStep = 2;
    double u, v, fDist, zPlane;

    if (pSrc == 0 || pDst == 0) return status_handle_invalid;
    if (length <= 0) return status_data_not_initialized;

    if( cameraSrc )
    {
        invFx = (double)(1. / cameraSrc[0]);
        invFy = (double)(1. / cameraSrc[2]);
    }

    for( ; n < length; n++, src += 3, dst += dstStep )
    {
        dst[0] = src[0];
        dst[1] = src[1];
        zPlane = src[2];
        if( cameraSrc )
        {
            dst[0] = u = ( dst[0] - cameraSrc[1] ) * invFx;
            dst[1] = v = ( dst[1] - cameraSrc[3] ) * invFy;

            if( invDistortionSrc )
            {
                double r2  = u * u + v * v;
                double r4  = r2 * r2;
                fDist = 1.f + invDistortionSrc[0] * r2 + invDistortionSrc[1] * r4 + invDistortionSrc[4] * r2 * r4;
                if( invDistortionSrc[2] != 0 || invDistortionSrc[3] != 0 )
                {
                    r4  = 2.f * u * v;
                    dst[0] = u * fDist + invDistortionSrc[2] * r4 + invDistortionSrc[3] * (r2 + 2.f * u * u);
                    dst[1] = v * fDist + invDistortionSrc[3] * r4 + invDistortionSrc[2] * (r2 + 2.f * v * v);
                }
                else
                {
                    dst[0] = u * fDist;
                    dst[1] = v * fDist;
                }
            }

            dst[0] *= zPlane;
            dst[1] *= zPlane;
        }

        if( rotation )
        {
            double tmp0 = rotation[0] * dst[0] + rotation[1] * dst[1] + rotation[2] * zPlane;
            double tmp1 = rotation[3] * dst[0] + rotation[4] * dst[1] + rotation[5] * zPlane;
            zPlane      = rotation[6] * dst[0] + rotation[7] * dst[1] + rotation[8] * zPlane;
            dst[0] = tmp0;
            dst[1] = tmp1;
        }

        if( translation )
        {
            dst[0] += translation[0];
            dst[1] += translation[1];
            zPlane += translation[2];
        }

        if( cameraDst )
        {
            if ( ABS(zPlane) <= MINABS_32F )
            {
                dst[0] = dst[1] = 0.f;
                //sts = STATUS_HANDLE_INVALID;
                continue;
            }

            u = v = 1.f / zPlane;
            u *= dst[0];
            v *= dst[1];

            if( distortionDst )
            {
                double r2  = u * u + v * v;
                double r4  = r2 * r2;
                fDist = 1.f + distortionDst[0] * r2 + distortionDst[1] * r4 + distortionDst[4] * r2 * r4;
                if( distortionDst[2] != 0 )
                {
                    r4  = 2.f * u * v;
                    u = u * fDist + distortionDst[2] * r4 + distortionDst[3] * (r2 + 2.f * u * u);
                    v = v * fDist + distortionDst[3] * r4 + distortionDst[2] * (r2 + 2.f * v * v);
                }
                else
                {
                    u *= fDist;
                    v *= fDist;
                }
            }

            dst[0] = u * cameraDst[0] + cameraDst[1];
            dst[1] = v * cameraDst[2] + cameraDst[3];
        }
        else
        {
            dst[2] = zPlane;
        }
    }

    return sts;
}

//Added
status REFCALL rsProjectionGetSize_32f ( rs::core::sizeI32 roiSize, int *pSpecSize )
{
    if(pSpecSize == 0) return status_handle_invalid;
    if (roiSize.width <= 0 || roiSize.height <= 0) return status_data_not_initialized;

    (*pSpecSize) = sizeof(float) * 16;
    (*pSpecSize) +=  roiSize.width * roiSize.height * sizeof(float) * 2;

    return status_no_error;
}

static int memcmp (unsigned char* ptr1, unsigned char* ptr2, int length)
{
    int i = 0;
    for ( ; i < length; ++i)
        if (ptr1[i] != ptr2[i]) return 1;
    return 0;
}

static void memcpy (unsigned char* dst, unsigned char* src, int length)
{
    int i = 0;
    for ( ; i < length; ++i) dst[i] = src[i];
    return;
}


//Added
status REFCALL rsProjectionInit_32f ( rs::core::sizeI32 roiSize, float cameraSrc[4], float invDistortion[5], ProjectionSpec_32f *pSpec )
{
    unsigned char* pBuffer = (unsigned char*)pSpec;
    rs::core::sizeI32 contextRoiSize = ((rs::core::sizeI32*)pBuffer)[0];
    int updatePreset  = (contextRoiSize.width != roiSize.width || contextRoiSize.height != roiSize.height) ? 1 : 0;

    if(cameraSrc == 0 || pSpec == 0) return status_handle_invalid;
    if (roiSize.width <= 0 || roiSize.height <= 0) return status_data_not_initialized;

    pBuffer += sizeof(roiSize);
    updatePreset |= memcmp( pBuffer, (unsigned char*)cameraSrc, sizeof(float) * 4 );
    if( invDistortion )
    {
        pBuffer += sizeof(float) * 4;
        updatePreset |= memcmp( pBuffer, (unsigned char*)invDistortion, sizeof(float) * 5 );
    }

    if( updatePreset )
    {
        pBuffer = (unsigned char*)pSpec;
        ((rs::core::sizeI32*)pBuffer)->width  = roiSize.width;
        ((rs::core::sizeI32*)pBuffer)->height = roiSize.height;
        pBuffer += sizeof(roiSize);
        memcpy(pBuffer, (unsigned char*)cameraSrc, 4 * sizeof(float));
        pBuffer += sizeof(float) * 4;
        if( invDistortion )
        {
            memcpy(pBuffer, (unsigned char*)invDistortion, 5 * sizeof(float));
            pBuffer += sizeof(float) * 5;
        }

        pBuffer = (unsigned char*)pSpec + sizeof(float) * 16;
        pointF32 *rowUV = (pointF32*)pBuffer;
        double invFx = (double)(1. / cameraSrc[0]);
        double invFy = (double)(1. / cameraSrc[2]);
        double u, v;
        int x, y;
        for ( y = 0; y < roiSize.height; ++y)
        {
            for ( x = 0; x < roiSize.width; ++x, rowUV++)
            {
                u = (( x - cameraSrc[1] ) * invFx);
                v = (( y - cameraSrc[3] ) * invFy);
                rowUV->x = (float)u;
                rowUV->y = (float)v;
                if( invDistortion )
                {
                    double r2  = u * u + v * v;
                    double r4  = r2 * r2;
                    double fDist = 1.f + (invDistortion[0] + (invDistortion[1] + invDistortion[4] * r2) * r2) * r2;
                    if( invDistortion[2] != 0 || invDistortion[3] != 0 )
                    {
                        r4  = 2.f * u * v;
                        rowUV->x = (float)(u * fDist + invDistortion[2] * r4 + invDistortion[3] * (r2 + 2.f * u * u));
                        rowUV->y = (float)(v * fDist + invDistortion[3] * r4 + invDistortion[2] * (r2 + 2.f * v * v));
                    }
                    else
                    {
                        rowUV->x = (float)(u * fDist);
                        rowUV->y = (float)(v * fDist);
                    }
                }
            }
        }
    }

    return status_no_error;
}

//Added
status REFCALL rsProjection_16u32f_C1CXR ( const unsigned short *pSrc, rs::core::sizeI32 roiSize, int srcStep, float *pDst, int dstStep,
        float rotation[9], float translation[3], float distortionDst[5], float cameraDst[4], const ProjectionSpec_32f *pSpec)
{
    if(pSrc == 0 || pDst == 0 || pSpec == 0) return status_handle_invalid;
    if (roiSize.width <= 0 || roiSize.height <= 0) return status_data_not_initialized;

    rs::core::sizeI32 contextRoiSize = ((rs::core::sizeI32*)pSpec)[0];
    if( roiSize.width != contextRoiSize.width || roiSize.height != contextRoiSize.height ) return status_param_unsupported ;
    status sts = status_no_error;

    double u, v, zPlane;
    int dstPix = 3;
    if( cameraDst ) dstPix = 2;

    unsigned char* pBuffer   = (unsigned char*)pSpec + sizeof(float) * 16;
    pointF32 *rowUV = (pointF32*)pBuffer;

    for (int y = 0; y < roiSize.height; ++y)
    {
        float* dst = (float*)((unsigned char*)pDst + y * dstStep);
        unsigned short* srcValue = (unsigned short*)pSrc;
        for (int x = 0; x < roiSize.width; ++x, rowUV++, dst += dstPix)
        {
            if ( srcValue[x] == 0 )
            {
                if( cameraDst )
                {
                    dst[0] = -1.f;
                    dst[1] = -1.f;
                }
                else
                {
                    dst[0] = 0.f;
                    dst[1] = 0.f;
                    dst[2] = 0.f;
                }
                continue;
            }

            zPlane = (float)srcValue[x];
            dst[0] = rowUV->x * zPlane;
            dst[1] = rowUV->y * zPlane;

            if( rotation )
            {
                double tmp0 = rotation[0] * dst[0] + rotation[1] * dst[1] + rotation[2] * zPlane;
                double tmp1 = rotation[3] * dst[0] + rotation[4] * dst[1] + rotation[5] * zPlane;
                zPlane      = rotation[6] * dst[0] + rotation[7] * dst[1] + rotation[8] * zPlane;
                dst[0] = tmp0;
                dst[1] = tmp1;
            }

            if( translation )
            {
                dst[0] += translation[0];
                dst[1] += translation[1];
                zPlane += translation[2];
            }

            if( cameraDst )
            {
                if ( ABS(zPlane) <= MINABS_32F )
                {
                    dst[0] = dst[1] = 0.f;
                    sts = status_handle_invalid;
                    continue;
                }

                u = v = 1.f / zPlane;
                u *= dst[0];
                v *= dst[1];

                if( distortionDst )
                {
                    double r2  = u * u + v * v;
                    double r4  = r2 * r2;
                    double fDist = 1.f + distortionDst[0] * r2 + distortionDst[1] * r4 + distortionDst[4] * r2 * r4;
                    if( distortionDst[2] != 0 )
                    {
                        r4  = 2.f * u * v;
                        u = u * fDist + distortionDst[2] * r4 + distortionDst[3] * (r2 + 2.f * u * u);
                        v = v * fDist + distortionDst[3] * r4 + distortionDst[2] * (r2 + 2.f * v * v);
                    }
                    else
                    {
                        u *= fDist;
                        v *= fDist;
                    }
                }

                dst[0] = u * cameraDst[0] + cameraDst[1];
                dst[1] = v * cameraDst[2] + cameraDst[3];
            }
            else
            {
                dst[2] = zPlane;
            }
        }
        pSrc  = (unsigned short*)((unsigned char*)pSrc + srcStep);
    }

    return sts;
}
