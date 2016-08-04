// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <cstring>

#include "math_projection_interface.h"
#include "projection/projection_utils.h"

namespace rs
{
    namespace core
    {
        inline double abs(double a) { return (a < 0) ? (-a) : a; }

        static status r_own_iuvmap_invertor(const pointF32 *uvMap, int uvMapStep, sizeI32 uvMapSize, rect uvMapRoi,
                                            pointF32 *uvInv, int uvInvStep, sizeI32 uvInvSize, rect uvInvRoi, int uvInvUnitsIsRelative);


        math_projection::math_projection() {}

        //Added
        status REFCALL math_projection::rs_projection_init_32f(sizeI32 roiSize, float cameraSrc[4], float invDistortion[5], projection_spec_32f *pSpec)
        {
            unsigned char* pBuffer = (unsigned char*)pSpec;
            sizeI32 contextRoiSize = ((sizeI32*)pBuffer)[0];
            int updatePreset  = (contextRoiSize.width != roiSize.width || contextRoiSize.height != roiSize.height) ? 1 : 0;

            if(cameraSrc == 0 || pSpec == 0) return status::status_handle_invalid;
            if (roiSize.width <= 0 || roiSize.height <= 0) return status::status_data_not_initialized;

            pBuffer += sizeof(roiSize);
            updatePreset |= memcmp( pBuffer, (unsigned char*)cameraSrc, sizeof(float) * 4 );
            if(invDistortion)
            {
                pBuffer += sizeof(float) * 4;
                updatePreset |= memcmp( pBuffer, (unsigned char*)invDistortion, sizeof(float) * 5 );
            }

            if(updatePreset)
            {
                pBuffer = (unsigned char*)pSpec;
                ((sizeI32*)pBuffer)->width  = roiSize.width;
                ((sizeI32*)pBuffer)->height = roiSize.height;
                pBuffer += sizeof(roiSize);
                memcpy(pBuffer, (unsigned char*)cameraSrc, 4 * sizeof(float));
                pBuffer += sizeof(float) * 4;
                if(invDistortion)
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
                        if(invDistortion)
                        {
                            double r2  = u * u + v * v;
                            double r4  = r2 * r2;
                            double fDist = 1.f + (invDistortion[0] + (invDistortion[1] + invDistortion[4] * r2) * r2) * r2;
                            if(invDistortion[2] != 0 || invDistortion[3] != 0)
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
            return status::status_no_error;
        }

        //Added
        status REFCALL math_projection::rs_3d_array_projection_32f(const float *pSrc, float *pDst, int length, float cameraSrc[4],
                float invDistortionSrc[5], float rotation[9], float translation[3], float distortionDst[5], float cameraDst[4])
        {

            status sts = status::status_no_error;
            signed int n = 0;
            double invFx, invFy;
            const float* src = (float*) pSrc;
            float* dst = (float*) pDst;
            int dstStep = 3;
            if(cameraDst) dstStep = 2;
            double u, v, fDist, zPlane;

            if (pSrc == 0 || pDst == 0) return status::status_handle_invalid;
            if (length <= 0) return status::status_data_not_initialized;

            if(cameraSrc)
            {
                invFx = (double)(1. / cameraSrc[0]);
                invFy = (double)(1. / cameraSrc[2]);
            }

            for( ; n < length; n++, src += 3, dst += dstStep)
            {
                dst[0] = src[0];
                dst[1] = src[1];
                zPlane = src[2];
                if(cameraSrc)
                {
                    dst[0] = u = (dst[0] - cameraSrc[1]) * invFx;
                    dst[1] = v = (dst[1] - cameraSrc[3]) * invFy;

                    if(invDistortionSrc)
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

                if (rotation)
                {
                    double tmp0 = rotation[0] * dst[0] + rotation[1] * dst[1] + rotation[2] * zPlane;
                    double tmp1 = rotation[3] * dst[0] + rotation[4] * dst[1] + rotation[5] * zPlane;
                    zPlane      = rotation[6] * dst[0] + rotation[7] * dst[1] + rotation[8] * zPlane;
                    dst[0] = tmp0;
                    dst[1] = tmp1;
                }

                if(translation)
                {
                    dst[0] += translation[0];
                    dst[1] += translation[1];
                    zPlane += translation[2];
                }

                if(cameraDst)
                {
                    if ( abs(zPlane) <= projection_utils::MINABS_32F )
                    {
                        dst[0] = dst[1] = 0.f;
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
        status REFCALL math_projection::rs_projection_16u32f_c1cxr(const unsigned short *pSrc, sizeI32 roiSize, int srcStep, float *pDst, int dstStep,
                float rotation[9], float translation[3], float distortionDst[5], float cameraDst[4], const projection_spec_32f *pSpec)
        {
            if(pSrc == 0 || pDst == 0 || pSpec == 0) return status::status_handle_invalid;
            if (roiSize.width <= 0 || roiSize.height <= 0) return status::status_data_not_initialized;

            sizeI32 contextRoiSize = ((sizeI32*)pSpec)[0];
            if( roiSize.width != contextRoiSize.width || roiSize.height != contextRoiSize.height ) return status::status_param_unsupported ;
            status sts = status::status_no_error;

            double u, v, zPlane;
            int dstPix = 3;
            if(cameraDst) dstPix = 2;

            unsigned char* pBuffer = (unsigned char*)pSpec + sizeof(float) * 16;
            pointF32 *rowUV = (pointF32*)pBuffer;

            for (int y = 0; y < roiSize.height; ++y)
            {
                float* dst = (float*)((unsigned char*)pDst + y * dstStep);
                unsigned short* srcValue = (unsigned short*)pSrc;
                for (int x = 0; x < roiSize.width; ++x, rowUV++, dst += dstPix)
                {
                    if (srcValue[x] == 0)
                    {
                        if(cameraDst)
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

                    if(rotation)
                    {
                        double tmp0 = rotation[0] * dst[0] + rotation[1] * dst[1] + rotation[2] * zPlane;
                        double tmp1 = rotation[3] * dst[0] + rotation[4] * dst[1] + rotation[5] * zPlane;
                        zPlane      = rotation[6] * dst[0] + rotation[7] * dst[1] + rotation[8] * zPlane;
                        dst[0] = tmp0;
                        dst[1] = tmp1;
                    }

                    if(translation)
                    {
                        dst[0] += translation[0];
                        dst[1] += translation[1];
                        zPlane += translation[2];
                    }

                    if(cameraDst)
                    {
                        if ( abs(zPlane) <= projection_utils::MINABS_32F )
                        {
                            dst[0] = dst[1] = 0.f;
                            sts = status::status_handle_invalid;
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

        //Added
        status REFCALL math_projection::rs_projection_get_size_32f(sizeI32 roiSize, int *pSpecSize)
        {
            if(pSpecSize == 0) return status::status_handle_invalid;
            if (roiSize.width <= 0 || roiSize.height <= 0) return status::status_data_not_initialized;

            (*pSpecSize) = sizeof(float) * 16;
            (*pSpecSize) +=  roiSize.width * roiSize.height * sizeof(float) * 2;

            return status::status_no_error;
        }

        //Added
        status REFCALL math_projection::rs_remap_16u_c1r(const unsigned short* pSrc, sizeI32 srcSize, int srcStep, const float* pxyMap,
                int xyMapStep, unsigned short* pDst, sizeI32 dstRoiSize,
                int dstStep, int interpolation_type, unsigned short defaultValue)
        {
            int x, y, sx, sy;
            if (pSrc == 0 || pDst == 0 || pxyMap == 0) return status::status_handle_invalid;
            if (srcSize.width <= 0 || srcSize.height <= 0 || dstRoiSize.width <= 0 || dstRoiSize.height <= 0) return status::status_data_not_initialized;
            if(interpolation_type != 0) return status::status_data_not_initialized;

            for (y = 0; y < dstRoiSize.height; y++)
            {
                for (x = 0; x < dstRoiSize.width; x++)
                {
                    const float* xy = pxyMap + x * 2;
                    sx = (int)(xy[0] + (1.5)) - 1; /* this formula guarantees the same half-down rounding from -1 to infinity */
                    sy = (int)(xy[1] + (1.5)) - 1; /* this formula guarantees the same half-down rounding from -1 to infinity */
                    if( sx >= 0  && sy >= 0 && sx < srcSize.width  && sy < srcSize.height)
                    {
                        ((unsigned short*)pDst)[x] = ((unsigned short*)((unsigned char*)pSrc + sy * srcStep))[sx];
                    }
                    else
                    {
                        ((unsigned short*)pDst)[x] = defaultValue;
                    }
                }
                pxyMap  = (const float*)((unsigned char*)pxyMap + xyMapStep);
                pDst    = (unsigned short*)((unsigned char*)pDst + dstStep);
            }
            return status::status_no_error;
        }

        //Added
        status REFCALL math_projection::rs_uvmap_filter_32f_c2ir(float *pSrcDst, int srcDstStep, sizeI32 roiSize,
                const unsigned short *pDepth, int depthStep, unsigned short invalidDepth)
        {
            pointF32 *uvTest;
            int y, x;
            for( y = 0; y < roiSize.height; y ++ )
            {
                if (pDepth != NULL)
                {
                    for ( x = 0; x < roiSize.width; x++ )
                    {
                        uvTest = ((pointF32*)pSrcDst) + x;
                        if( ((unsigned short*)pDepth)[x] > 0 && ((unsigned short*)pDepth)[x] != invalidDepth
                                && uvTest->x >= 0.f && uvTest->x < 1.f && uvTest->y >= 0.f && uvTest->y < 1.f )
                        {
                            continue;
                        }
                        uvTest->x = uvTest->y = -1.f;
                    }
                    pDepth  = (const unsigned short*)((const unsigned char*)pDepth + depthStep);
                }
                else
                {
                    for ( x = 0; x < roiSize.width; x++ )
                    {
                        uvTest = ((pointF32*)pSrcDst) + x;
                        if( uvTest->x >= 0.f && uvTest->x < 1.f && uvTest->y >= 0.f && uvTest->y < 1.f )
                        {
                            continue;
                        }
                        uvTest->x = uvTest->y = -1.f;
                    }
                }
                pSrcDst = (float*)((unsigned char*)pSrcDst + srcDstStep);
            }
            return status::status_no_error;
        }

        //Added
        status REFCALL math_projection::rs_uvmap_invertor_32f_c2r(const float *pSrc, int srcStep, sizeI32 srcSize, rect srcRoi,
                float *pDst, int dstStep, sizeI32 dstSize, int unitsIsRelative)
        {
            rect uvInvRoi = {0, 0, dstSize.width, dstSize.height};
            int i, j;
            float *dst = pDst;

            for (i = 0; i < dstSize.height; ++i)
            {
                for (j = 0; j < dstSize.width * 2; ++j)
                {
                    dst[j] = -1.f;
                }
                dst = (float*)((unsigned char*)dst + dstStep);
            }
            return r_own_iuvmap_invertor((pointF32*)pSrc, srcStep, srcSize, srcRoi, (pointF32*)pDst, dstStep, dstSize, uvInvRoi, unitsIsRelative);
        }

        status REFCALL math_projection::rs_qr_decomp_m_64f(const double* pSrc, int srcStride1, int srcStride2, double* pBuffer,
                double* pDst, int dstStride1, int dstStride2, int width, int height)
        {
            double  sum, beta, norm;
            int  i, j, l;
            int  srcS1, dstS1;
            int  srcS2, dstS2;

            /* check array pointers */
            if( 0 == pSrc || 0 == pDst ) return status::status_handle_invalid;

            /* check buffer pointer */
            if( 0 == pBuffer ) return status::status_handle_invalid;

            /* check values of matrix sizes */
            if( 0 >= width  ) return status::status_param_unsupported;
            if( 0 >= height ) return status::status_param_unsupported;

            /* check: data stride should be dividen to the size of data type */
            if( 0 >= srcStride1 || 0 != srcStride1  % sizeof(double)) return status::status_param_unsupported;
            if( 0 >= srcStride2 || 0 != srcStride2  % sizeof(double)) return status::status_param_unsupported;

            if( 0 >= dstStride1 || 0 != dstStride1  % sizeof(double)) return status::status_param_unsupported;
            if( 0 >= dstStride2 || 0 != dstStride2  % sizeof(double)) return status::status_param_unsupported;

            /* Mathematical operation code */

            srcS1  = srcStride1 /sizeof(double);
            srcS2  = srcStride2 /sizeof(double);

            dstS1  = dstStride1 /sizeof(double);
            dstS2  = dstStride2 /sizeof(double);

            if( height < width) return status::status_param_unsupported;
            /* Copy array to buffer */
            for(j = 0; j < height; j++)
            {
                for(i = 0; i < width; i++)
                {
                    pDst[dstS1*j+i*dstS2] = pSrc[srcS1*j+i*srcS2];
                }
            }

            int size = (width == height) ? (width - 1):width;

            /* QR factorization */
            for(j = 0; j < size; j++)
            {
                norm = 0.0;
                for(i = j; i < height; i++)
                {
                    norm += pDst[dstS1*i+j*dstS2] * pDst[dstS1*i+j*dstS2];
                }

                if(fabs(norm) < projection_utils::EPS52)
                {
                    return status::status_handle_invalid;
                }

                norm = (double)sqrt(norm);
                norm = (pDst[dstS1*j+j*dstS2] > 0) ? norm:(-norm);
                norm += pDst[dstS1*j+j*dstS2];
                norm = (double)1.0 / norm;

                /* Culculate beta factor */
                sum = 1.0;
                pBuffer[j] = 1.0;

                for(i = j + 1; i < height; i++)
                {
                    pBuffer[i] = pDst[dstS1*i+j*dstS2] * norm;
                    sum += pBuffer[i] * pBuffer[i];
                }
                beta = (double)(-2.0) / sum;

                /*Multiply by Househokder matrix*/
                for(i = j; i < width; i++)
                {
                    sum = pDst[dstS1*j+i*dstS2];
                    for(l = j + 1; l < height; l++)
                    {
                        sum += pDst[dstS1*l+i*dstS2] * pBuffer[l];
                    }
                    sum *= beta;
                    for(l = j; l < height; l++)
                    {
                        pDst[dstS1*l+i*dstS2] += sum * pBuffer[l];
                    }
                }

                /* Store Householder vector */
                for(i=j+1; i<height; i++)
                {
                    pDst[dstS1*i+j*dstS2] = pBuffer[i];
                }
            }

            return status::status_no_error;
        }

        status REFCALL math_projection::rs_qr_back_subst_mva_64f(const double* pSrc1, int src1Stride1, int src1Stride2, double* pBuffer,
                const double* pSrc2, int src2Stride0, int src2Stride2, double* pDst,
                int dstStride0, int dstStride2, int width, int height, int count)
        {
            double  sum, beta, w;
            int  i, j, k, size;
            int  src1S1, src1S2;
            int  src2S2, dstS2;
            int  src2S0, dstS0;

            /* check array pointers */
            if( 0 == pSrc1 || 0 == pSrc2 || 0 == pDst ) return status::status_handle_invalid;

            /* check buffer pointer */
            if( 0 == pBuffer ) return status::status_handle_invalid;

            /* check values of matrix sizes */
            if( 0 >= width  ) return status::status_param_unsupported;
            if( 0 >= height ) return status::status_param_unsupported;

            /* check values of count */
            if( 0 >= count ) return status::status_param_unsupported;

            /* check: data stride should be dividen to the size of data type */
            if( 0 >= src1Stride1 || 0 != src1Stride1 % sizeof(double)) return status::status_param_unsupported;
            if( 0 >= src1Stride2 || 0 != src1Stride2 % sizeof(double)) return status::status_param_unsupported;
            if( 0 >= src2Stride2 || 0 != src2Stride2 % sizeof(double)) return status::status_param_unsupported;

            if( 0 >= dstStride0  || 0 != dstStride0  % sizeof(double)) return status::status_param_unsupported;
            if( 0 >= dstStride2  || 0 != dstStride2  % sizeof(double)) return status::status_param_unsupported;

            /* Mathematical operation code */

            src1S1 = src1Stride1/sizeof(double);
            src1S2 = src1Stride2/sizeof(double);

            src2S0 = src2Stride0/sizeof(double);
            dstS0  = dstStride0 /sizeof(double);

            src2S2 = src2Stride2/sizeof(double);
            dstS2  = dstStride2 /sizeof(double);

            for(k = 0; k < count; k++)
            {
                /* Copy vector to buffer */
                for(i=0; i<height; i++)
                {
                    pBuffer[i] = pSrc2[src2S0*k+i*src2S2];
                }
                size = (width == height) ? (width-1):width;

                for(j=0; j<size; j++)
                {
                    /* Culculate beta and w */
                    beta = 1.0;
                    w = pBuffer[j];
                    for(i = j + 1; i < height; i++)
                    {
                        beta += pSrc1[src1S1*i+j*src1S2] * pSrc1[src1S1*i+j*src1S2];
                        w += pBuffer[i] * pSrc1[src1S1*i+j*src1S2];
                    }

                    beta = (double)(-2.0) / beta;
                    w *= beta;

                    /*Multiply by Househokder matrix*/
                    pBuffer[j] += w;
                    for(i = j + 1; i < height; i++)
                    {
                        pBuffer[i] += pSrc1[src1S1*i+j*src1S2] * w;
                    }
                }
                pDst[dstS0*k+(width-1)*dstS2] = pBuffer[width-1] / pSrc1[src1S1*(width-1)+(width-1)*src1S2];
                for(j = width - 1; j > 0; j--)
                {
                    sum = 0.0;
                    for(i = j; i < width; i++)
                    {
                        sum += pSrc1[src1S1*(j-1)+i*src1S2] * pDst[dstS0*k+i*dstS2];
                    }
                    pDst[dstS0*k+(j-1)*dstS2] = (pBuffer[j-1] - sum) / pSrc1[src1S1*(j-1)+(j-1)*src1S2];
                }
            }

            return status::status_no_error;
        }

        double min_of_array(double *v, int n)
        {
            double minV = v[0];
            int k;
            for (k=1; k<n; k++)
            {
                if (v[k] < minV) minV=v[k];
            }
            return minV;
        }

        double max_of_array(double *v, int n)
        {
            double maxV = v[0];
            int k;
            for (k=1; k<n; k++)
            {
                if (v[k] > maxV) maxV=v[k];
            }
            return maxV;
        }

        status r_own_iuvmap_invertor(const pointF32 *uvMap, int uvMapStep, sizeI32 uvMapSize, rect uvMapRoi,
                                     pointF32 *uvInv, int uvInvStep, sizeI32 uvInvSize, rect uvInvRoi, int uvInvUnitsIsRelative)
        {

            typedef struct
            {
                double x;
                double y;
            } point_64f;


            point_64f pValidPix[4];
            int r, c, xmin,xmax,ymin,ymax, numPix;
            double a0, b0, c0, a1, b1, c1, d1, e1, d0, e0;
            double dx0, dy0, dx1, dy1, dx2, dy2, dx3, dy3, dx4, dy4;
            double *p0, *p1, *p2, *p3;
            double widthC = (double)uvInvSize.width;
            double heightC = (double)uvInvSize.height;
            int  xmin_uvInvRoi = uvInvRoi.x;
            int  ymin_uvInvRoi = uvInvRoi.y;
            int  xmax_uvInvRoi = uvInvRoi.x + uvInvRoi.width  - 1;
            int  ymax_uvInvRoi = uvInvRoi.y + uvInvRoi.height - 1;
            int iY, iX;

            double xNorming = uvInvUnitsIsRelative?(1. / (double)uvMapSize.width):1.f;
            double yNorming = uvInvUnitsIsRelative?(1. / (double)uvMapSize.height):1.f;

            unsigned char *uvMapPtr0 = (unsigned char*)uvMap + uvMapRoi.x * sizeof(pointF32) + uvMapRoi.y * uvMapStep;
            unsigned char *uvMapPtr1 = uvMapPtr0 + uvMapStep;
            unsigned char *pUVInv    = (unsigned char*)uvInv;
            pointF32 *uvPtr0, *uvPtr1, *uvInvPtr;
            // Since RGB has smaller FOV than depth, we can sweep through a subset of the depth pixels
            for ( r=0; r < uvMapRoi.height-1; r++ )
            {
                uvPtr0 = (pointF32*)uvMapPtr0;
                uvPtr1 = (pointF32*)uvMapPtr1;
                for ( c=0; c < uvMapRoi.width-1; c++ )
                {
                    // Get the number of valid pixels (around (c,r))
                    const double posC = (uvMapRoi.x + c + 0.5f) * xNorming;
                    const double posR = (uvMapRoi.y + r + 0.5f) * yNorming;

                    numPix = 0;
                    if( uvPtr0[c].x >= 0.f )
                    {
                        pValidPix[numPix].x = uvPtr0[c].x;
                        pValidPix[numPix].y = uvPtr0[c].y;
                        numPix ++;
                    }
                    if( uvPtr0[c+1].x >= 0.f )
                    {
                        pValidPix[numPix].x = uvPtr0[c+1].x;
                        pValidPix[numPix].y = uvPtr0[c+1].y;
                        numPix ++;
                    }
                    if (numPix == 0)
                        continue;

                    if( uvPtr1[c].x >= 0.f )
                    {
                        pValidPix[numPix].x = uvPtr1[c].x;
                        pValidPix[numPix].y = uvPtr1[c].y;
                        numPix ++;
                    }
                    if( uvPtr1[c+1].x >= 0.f )
                    {
                        pValidPix[numPix].x = uvPtr1[c+1].x;
                        pValidPix[numPix].y = uvPtr1[c+1].y;
                        numPix ++;
                    }

                    if (numPix < 3)
                        continue;

                    pValidPix[0].x *= widthC; pValidPix[0].y *= heightC;
                    pValidPix[1].x *= widthC; pValidPix[1].y *= heightC;
                    pValidPix[2].x *= widthC; pValidPix[2].y *= heightC;
                    p0=(double*)&pValidPix[0];
                    p1=(double*)&pValidPix[1];
                    p2=(double*)&pValidPix[2];

                    if ( numPix == 4 )
                    {
                        // Use pointers instead of datacopy
                        double v_x[4];
                        double v_y[4];

                        pValidPix[3].x *= widthC; pValidPix[3].y *= heightC;
                        p3=(double*)&pValidPix[3];

                        v_x[0] = p0[0];
                        v_x[1] = p1[0];
                        v_x[2] = p2[0];
                        v_x[3] = p3[0];

                        xmin = (int) ceil(min_of_array(v_x, 4));
                        xmax = (int)max_of_array(v_x, 4);
                        if (xmin < xmin_uvInvRoi) xmin = xmin_uvInvRoi;
                        if (xmax > xmax_uvInvRoi) xmax = xmax_uvInvRoi;

                        v_y[0] = p0[1];
                        v_y[1] = p1[1];
                        v_y[2] = p2[1];
                        v_y[3] = p3[1];

                        ymin = (int)ceil(min_of_array(v_y, 4));
                        ymax = (int)max_of_array(v_y, 4);
                        if (ymin < ymin_uvInvRoi) ymin = ymin_uvInvRoi;
                        if (ymax > ymax_uvInvRoi) ymax = ymax_uvInvRoi;

                        // TriangleInteriorTest
                        if (p0[0] > p1[0]) std::swap(p0, p1);
                        if (p0[1] > p2[1]) std::swap(p0, p2);
                        if (p2[0] > p3[0]) std::swap(p2, p3);
                        if (p1[1] > p3[1]) std::swap(p1, p3);

                        dx0 = p1[0] - p0[0];
                        dy0 = p1[1] - p0[1];
                        dx1 = p2[0] - p1[0];
                        dy1 = p2[1] - p1[1];
                        dx2 = p0[0] - p2[0];
                        dy2 = p0[1] - p2[1];
                        dx3 = p3[0] - p2[0];
                        dy3 = p3[1] - p2[1];
                        dx4 = p1[0] - p3[0];
                        dy4 = p1[1] - p3[1];

                        a0 = dy0 * (p0[0] - xmin + 1) - dx0 * (p0[1] - ymin);
                        b0 = dy1 * (p1[0] - xmin + 1) - dx1 * (p1[1] - ymin);
                        c0 = dy2 * (p2[0] - xmin + 1) - dx2 * (p2[1] - ymin);
                        d0 = dy3 * (p2[0] - xmin + 1) - dx3 * (p2[1] - ymin);
                        e0 = dy4 * (p3[0] - xmin + 1) - dx4 * (p3[1] - ymin);

                        pUVInv = ((unsigned char*)uvInv + ymin * uvInvStep);
                        for (iY = ymin ; iY <= ymax ; ++iY)
                        {
                            a1 = a0;
                            b1 = b0;
                            c1 = c0;
                            d1 = d0;
                            e1 = e0;

                            uvInvPtr = (pointF32*)pUVInv;
                            for (iX=xmin ; iX <= xmax ; ++iX)
                            {
                                a1 -= dy0;
                                b1 -= dy1;
                                c1 -= dy2;
                                d1 -= dy3;
                                e1 -= dy4;
                                if (uvInvPtr[iX].x==-1)
                                {
                                    if( b1 >= 0 )
                                    {
                                        if( a1 >= 0 )
                                        {
                                            if( c1 >= 0 )
                                            {
                                                uvInvPtr[iX].x = (float)posC; uvInvPtr[iX].y = (float)posR;
                                                continue;
                                            }
                                        }
                                        if( d1 >= 0 )
                                        {
                                            if( e1 >= 0 )
                                            {
                                                uvInvPtr[iX].x = (float)posC; uvInvPtr[iX].y = (float)posR;
                                                continue;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        if( a1 < 0 )
                                        {
                                            if( c1 < 0 )
                                            {
                                                uvInvPtr[iX].x = (float)posC; uvInvPtr[iX].y = (float)posR;
                                                continue;
                                            }
                                        }
                                        if( d1 < 0 )
                                        {
                                            if( e1 < 0 )
                                            {
                                                uvInvPtr[iX].x = (float)posC; uvInvPtr[iX].y = (float)posR;
                                                continue;
                                            }
                                        }
                                    }
                                }
                            }
                            a0 += dx0;
                            b0 += dx1;
                            c0 += dx2;
                            d0 += dx3;
                            e0 += dx4;
                            pUVInv += uvInvStep;
                        }
                    }
                    else if ( numPix == 3 )
                    {
                        // only 3 elements detected
                        double v_x[] = {p0[0], p1[0], p2[0]};
                        double v_y[] = {p0[1], p1[1], p2[1]};
                        xmin = (int) ceil(min_of_array(v_x, 3));
                        xmax = (int)max_of_array(v_x, 3);
                        if (xmin < xmin_uvInvRoi) xmin = xmin_uvInvRoi;
                        if (xmax > xmax_uvInvRoi) xmax = xmax_uvInvRoi;

                        ymin = (int)ceil(min_of_array(v_y, 3));
                        ymax = (int)max_of_array(v_y, 3);
                        if (ymin < ymin_uvInvRoi) ymin = ymin_uvInvRoi;
                        if (ymax > ymax_uvInvRoi) ymax = ymax_uvInvRoi;

                        // TriangleInteriorTest
                        dx0 = p1[0] - p0[0];
                        dy0 = p1[1] - p0[1];
                        dx1 = p2[0] - p1[0];
                        dy1 = p2[1] - p1[1];
                        dx2 = p0[0] - p2[0];
                        dy2 = p0[1] - p2[1];

                        a0 = dy0 * (p0[0] - xmin + 1) - dx0 * (p0[1] - ymin);
                        b0 = dy1 * (p1[0] - xmin + 1) - dx1 * (p1[1] - ymin);
                        c0 = dy2 * (p2[0] - xmin + 1) - dx2 * (p2[1] - ymin);

                        pUVInv = ((unsigned char*)uvInv + ymin * uvInvStep);
                        for (iY = ymin ; iY <= ymax ; ++iY)
                        {
                            a1 = a0;
                            b1 = b0;
                            c1 = c0;

                            uvInvPtr = (pointF32*)pUVInv;
                            for (iX=xmin ; iX <= xmax ; ++iX)
                            {
                                a1 -= dy0;
                                b1 -= dy1;
                                c1 -= dy2;
                                if (uvInvPtr[iX].x==-1)
                                {
                                    if( a1 >= 0 )
                                    {
                                        if( b1 >= 0 )
                                        {
                                            if( c1 >= 0 )
                                            {
                                                uvInvPtr[iX].x = (float)posC; uvInvPtr[iX].y = (float)posR;
                                                continue;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        if( b1 < 0 )
                                        {
                                            if( c1 < 0 )
                                            {
                                                uvInvPtr[iX].x = (float)posC; uvInvPtr[iX].y = (float)posR;
                                                continue;
                                            }
                                        }
                                    }
                                }
                            }
                            a0 += dx0;
                            b0 += dx1;
                            c0 += dx2;
                            pUVInv += uvInvStep;
                        }


                    } // endif numPix
                }
                uvMapPtr0 += uvMapStep;
                uvMapPtr1 += uvMapStep;
            }

            return status::status_no_error;
        }
    }
}
