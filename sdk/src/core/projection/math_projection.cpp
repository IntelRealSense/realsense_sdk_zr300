// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <cstring>
#include <utility>

#include "math_projection_interface.h"

const float MINABS_32F = 1.175494351e-38f;
const double EPS52 = 2.2204460492503131e-016;

namespace rs
{
    namespace core
    {
        inline double abs(double a) { return (a < 0) ? (-a) : a; }

        static status r_own_iuvmap_invertor(const pointF32 *uvmap, int uvmap_step, sizeI32 uvmap_size, rect uvmap_roi,
                                            pointF32 *uvInv, int uvinv_step, sizeI32 uvinv_size, rect uvinv_roi, int uvinv_units_is_relative, pointF32 threshold);


        math_projection::math_projection() {}

        //Added
        status REFCALL math_projection::rs_projection_init_32f(sizeI32 roi_size, float camera_src[4], float inv_distortion[5], projection_spec_32f *pspec)
        {
            unsigned char* pbuffer = (unsigned char*)pspec;
            sizeI32 context_roi_size = ((sizeI32*)pbuffer)[0];
            int update_preset  = (context_roi_size.width != roi_size.width || context_roi_size.height != roi_size.height) ? 1 : 0;

            if(camera_src == 0 || pspec == 0) return status::status_handle_invalid;
            if (roi_size.width <= 0 || roi_size.height <= 0) return status::status_data_not_initialized;

            pbuffer += sizeof(roi_size);
            update_preset |= memcmp( pbuffer, (unsigned char*)camera_src, sizeof(float) * 4 );
            if(inv_distortion)
            {
                pbuffer += sizeof(float) * 4;
                update_preset |= memcmp( pbuffer, (unsigned char*)inv_distortion, sizeof(float) * 5 );
            }

            if(update_preset)
            {
                pbuffer = (unsigned char*)pspec;
                ((sizeI32*)pbuffer)->width  = roi_size.width;
                ((sizeI32*)pbuffer)->height = roi_size.height;
                pbuffer += sizeof(roi_size);
                memcpy(pbuffer, (unsigned char*)camera_src, 4 * sizeof(float));
                pbuffer += sizeof(float) * 4;
                if(inv_distortion)
                {
                    memcpy(pbuffer, (unsigned char*)inv_distortion, 5 * sizeof(float));
                    pbuffer += sizeof(float) * 5;
                }

                pbuffer = (unsigned char*)pspec + sizeof(float) * 16;
                pointF32 *rowUV = (pointF32*)pbuffer;
                double invFx = (double)(1. / camera_src[0]);
                double invFy = (double)(1. / camera_src[2]);
                double u, v;
                int x, y;
                for ( y = 0; y < roi_size.height; ++y)
                {
                    for ( x = 0; x < roi_size.width; ++x, rowUV++)
                    {
                        u = (( x - camera_src[1] ) * invFx);
                        v = (( y - camera_src[3] ) * invFy);
                        rowUV->x = (float)u;
                        rowUV->y = (float)v;
                        if(inv_distortion)
                        {
                            double r2  = u * u + v * v;
                            double r4  = r2 * r2;
                            double fDist = 1.f + (inv_distortion[0] + (inv_distortion[1] + inv_distortion[4] * r2) * r2) * r2;
                            if(inv_distortion[2] != 0 || inv_distortion[3] != 0)
                            {
                                r4  = 2.f * u * v;
                                rowUV->x = (float)(u * fDist + inv_distortion[2] * r4 + inv_distortion[3] * (r2 + 2.f * u * u));
                                rowUV->y = (float)(v * fDist + inv_distortion[3] * r4 + inv_distortion[2] * (r2 + 2.f * v * v));
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
        status REFCALL math_projection::rs_3d_array_projection_32f(const float *psrc, float *pdst, int length, float camera_src[4],
                float inv_distortionSrc[5], float rotation[9], float translation[3], float distortion_dst[5], float camera_dst[4])
        {

            status sts = status::status_no_error;
            signed int n = 0;
            double invFx, invFy;
            const float* src = (float*) psrc;
            float* dst = (float*) pdst;
            int dst_step = 3;
            if(camera_dst) dst_step = 2;
            double u, v, fDist, zPlane;

            if (psrc == 0 || pdst == 0) return status::status_handle_invalid;
            if (length <= 0) return status::status_data_not_initialized;

            if(camera_src)
            {
                invFx = (double)(1. / camera_src[0]);
                invFy = (double)(1. / camera_src[2]);
            }

            for( ; n < length; n++, src += 3, dst += dst_step)
            {
                dst[0] = src[0];
                dst[1] = src[1];
                zPlane = src[2];
                if(camera_src)
                {
                    dst[0] = u = (dst[0] - camera_src[1]) * invFx;
                    dst[1] = v = (dst[1] - camera_src[3]) * invFy;

                    if(inv_distortionSrc)
                    {
                        double r2  = u * u + v * v;
                        double r4  = r2 * r2;
                        fDist = 1.f + inv_distortionSrc[0] * r2 + inv_distortionSrc[1] * r4 + inv_distortionSrc[4] * r2 * r4;
                        if( inv_distortionSrc[2] != 0 || inv_distortionSrc[3] != 0 )
                        {
                            r4  = 2.f * u * v;
                            dst[0] = u * fDist + inv_distortionSrc[2] * r4 + inv_distortionSrc[3] * (r2 + 2.f * u * u);
                            dst[1] = v * fDist + inv_distortionSrc[3] * r4 + inv_distortionSrc[2] * (r2 + 2.f * v * v);
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

                if(camera_dst)
                {
                    if ( abs(zPlane) <= MINABS_32F )
                    {
                        dst[0] = dst[1] = 0.f;
                        continue;
                    }

                    u = v = 1.f / zPlane;
                    u *= dst[0];
                    v *= dst[1];

                    if( distortion_dst )
                    {
                        double r2  = u * u + v * v;
                        double r4  = r2 * r2;
                        fDist = 1.f + distortion_dst[0] * r2 + distortion_dst[1] * r4 + distortion_dst[4] * r2 * r4;
                        if( distortion_dst[2] != 0 )
                        {
                            r4  = 2.f * u * v;
                            u = u * fDist + distortion_dst[2] * r4 + distortion_dst[3] * (r2 + 2.f * u * u);
                            v = v * fDist + distortion_dst[3] * r4 + distortion_dst[2] * (r2 + 2.f * v * v);
                        }
                        else
                        {
                            u *= fDist;
                            v *= fDist;
                        }
                    }

                    dst[0] = u * camera_dst[0] + camera_dst[1];
                    dst[1] = v * camera_dst[2] + camera_dst[3];
                }
                else
                {
                    dst[2] = zPlane;
                }
            }
            return sts;
        }

        //Added
        #pragma vector
        status REFCALL math_projection::rs_projection_16u32f_c1cxr(const unsigned short *psrc, sizeI32 roi_size, int src_step, float *pdst, int dst_step,
                float rotation[9], float translation[3], float distortion_dst[5], float camera_dst[4], const projection_spec_32f *pspec)
        {
            if(psrc == 0 || pdst == 0 || pspec == 0) return status::status_handle_invalid;
            if (roi_size.width <= 0 || roi_size.height <= 0) return status::status_data_not_initialized;

            sizeI32 context_roi_size = ((sizeI32*)pspec)[0];
            if( roi_size.width != context_roi_size.width || roi_size.height != context_roi_size.height ) return status::status_param_unsupported ;
            status sts = status::status_no_error;

            double u, v, zPlane;
            int dstPi_x = 3;
            if(camera_dst) dstPi_x = 2;

            unsigned char* pbuffer = (unsigned char*)pspec + sizeof(float) * 16;
            pointF32 *rowUV = (pointF32*)pbuffer;

            for (int y = 0; y < roi_size.height; ++y)
            {
                float* dst = (float*)((unsigned char*)pdst + y * dst_step);
                unsigned short* src_value = (unsigned short*)psrc;
                for (int x = 0; x < roi_size.width; ++x, rowUV++, dst += dstPi_x)
                {
                    if (src_value[x] == 0)
                    {
                        if(camera_dst)
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

                    zPlane = (float)src_value[x];
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

                    if(camera_dst)
                    {
                        if ( abs(zPlane) <= MINABS_32F )
                        {
                            dst[0] = dst[1] = 0.f;
                            sts = status::status_handle_invalid;
                            continue;
                        }

                        u = v = 1.f / zPlane;
                        u *= dst[0];
                        v *= dst[1];

                        if( distortion_dst )
                        {
                            double r2  = u * u + v * v;
                            double r4  = r2 * r2;
                            double fDist = 1.f + distortion_dst[0] * r2 + distortion_dst[1] * r4 + distortion_dst[4] * r2 * r4;
                            if( distortion_dst[2] != 0 )
                            {
                                r4  = 2.f * u * v;
                                u = u * fDist + distortion_dst[2] * r4 + distortion_dst[3] * (r2 + 2.f * u * u);
                                v = v * fDist + distortion_dst[3] * r4 + distortion_dst[2] * (r2 + 2.f * v * v);
                            }
                            else
                            {
                                u *= fDist;
                                v *= fDist;
                            }
                        }

                        dst[0] = u * camera_dst[0] + camera_dst[1];
                        dst[1] = v * camera_dst[2] + camera_dst[3];
                    }
                    else
                    {
                        dst[2] = zPlane;
                    }
                }
                psrc  = (unsigned short*)((unsigned char*)psrc + src_step);
            }
            return sts;
        }

        //Added
        status REFCALL math_projection::rs_projection_get_size_32f(sizeI32 roi_size, int *pspec_size)
        {
            if(pspec_size == 0) return status::status_handle_invalid;
            if (roi_size.width <= 0 || roi_size.height <= 0) return status::status_data_not_initialized;

            (*pspec_size) = sizeof(float) * 16;
            (*pspec_size) +=  roi_size.width * roi_size.height * sizeof(float) * 2;

            return status::status_no_error;
        }

        //Added
        status REFCALL math_projection::rs_remap_16u_c1r(const unsigned short* psrc, sizeI32 src_size, int src_step, const float* pxy_map,
                int xy_map_step, unsigned short* pdst, sizeI32 dst_roi_size,
                int dst_step, int interpolation_type, unsigned short default_value)
        {
            int x, y, sx, sy;
            if (psrc == 0 || pdst == 0 || pxy_map == 0) return status::status_handle_invalid;
            if (src_size.width <= 0 || src_size.height <= 0 || dst_roi_size.width <= 0 || dst_roi_size.height <= 0) return status::status_data_not_initialized;
            if(interpolation_type != 0) return status::status_data_not_initialized;

            for (y = 0; y < dst_roi_size.height; y++)
            {
                for (x = 0; x < dst_roi_size.width; x++)
                {
                    const float* xy = pxy_map + x * 2;
                    sx = (int)(xy[0] + (1.5)) - 1; /* this formula guarantees the same half-down rounding from -1 to infinity */
                    sy = (int)(xy[1] + (1.5)) - 1; /* this formula guarantees the same half-down rounding from -1 to infinity */
                    if( sx >= 0  && sy >= 0 && sx < src_size.width  && sy < src_size.height)
                    {
                        ((unsigned short*)pdst)[x] = ((unsigned short*)((unsigned char*)psrc + sy * src_step))[sx];
                    }
                    else
                    {
                        ((unsigned short*)pdst)[x] = default_value;
                    }
                }
                pxy_map  = (const float*)((unsigned char*)pxy_map + xy_map_step);
                pdst    = (unsigned short*)((unsigned char*)pdst + dst_step);
            }
            return status::status_no_error;
        }

        //Added
        status REFCALL math_projection::rs_uvmap_filter_32f_c2ir(float *psrc_dst, int srcdst_step, sizeI32 roi_size,
                const unsigned short *pdepth, int depth_step, unsigned short invalid_depth)
        {
            pointF32 *uvTest;
            int y, x;
            for( y = 0; y < roi_size.height; y ++ )
            {
                if (pdepth != NULL)
                {
                    for ( x = 0; x < roi_size.width; x++ )
                    {
                        uvTest = ((pointF32*)psrc_dst) + x;
                        if( ((unsigned short*)pdepth)[x] > 0 && ((unsigned short*)pdepth)[x] != invalid_depth
                                && uvTest->x >= 0.f && uvTest->x < 1.f && uvTest->y >= 0.f && uvTest->y < 1.f )
                        {
                            continue;
                        }
                        uvTest->x = uvTest->y = -1.f;
                    }
                    pdepth  = (const unsigned short*)((const unsigned char*)pdepth + depth_step);
                }
                else
                {
                    for ( x = 0; x < roi_size.width; x++ )
                    {
                        uvTest = ((pointF32*)psrc_dst) + x;
                        if( uvTest->x >= 0.f && uvTest->x < 1.f && uvTest->y >= 0.f && uvTest->y < 1.f )
                        {
                            continue;
                        }
                        uvTest->x = uvTest->y = -1.f;
                    }
                }
                psrc_dst = (float*)((unsigned char*)psrc_dst + srcdst_step);
            }
            return status::status_no_error;
        }

        //Added
        status REFCALL math_projection::rs_uvmap_invertor_32f_c2r(const float *psrc, int src_step, sizeI32 src_size, rect src_roi,
                float *pdst, int dst_step, sizeI32 dst_size, int units_is_relative, pointF32 threshold)
        {
            rect uvinv_roi = {0, 0, dst_size.width, dst_size.height};
            int i, j;
            float *dst = pdst;

            for (i = 0; i < dst_size.height; ++i)
            {
                for (j = 0; j < dst_size.width * 2; ++j)
                {
                    dst[j] = -1.f;
                }
                dst = (float*)((unsigned char*)dst + dst_step);
            }
            return r_own_iuvmap_invertor((pointF32*)psrc, src_step, src_size, src_roi, (pointF32*)pdst, dst_step, dst_size, uvinv_roi, units_is_relative, threshold);
        }

        status REFCALL math_projection::rs_qr_decomp_m_64f(const double* psrc, int src_stride1, int src_stride2, double* pbuffer,
                double* pdst, int dststride1, int dststride2, int width, int height)
        {
            double  sum, beta, norm;
            int  i, j, l;
            int  srcS1, dstS1;
            int  srcS2, dstS2;

            /* check array pointers */
            if( 0 == psrc || 0 == pdst ) return status::status_handle_invalid;

            /* check buffer pointer */
            if( 0 == pbuffer ) return status::status_handle_invalid;

            /* check values of matri_x sizes */
            if( 0 >= width  ) return status::status_param_unsupported;
            if( 0 >= height ) return status::status_param_unsupported;

            /* check: data stride should be dividen to the size of data type */
            if( 0 >= src_stride1 || 0 != src_stride1  % sizeof(double)) return status::status_param_unsupported;
            if( 0 >= src_stride2 || 0 != src_stride2  % sizeof(double)) return status::status_param_unsupported;

            if( 0 >= dststride1 || 0 != dststride1  % sizeof(double)) return status::status_param_unsupported;
            if( 0 >= dststride2 || 0 != dststride2  % sizeof(double)) return status::status_param_unsupported;

            /* Mathematical operation code */

            srcS1  = src_stride1 /sizeof(double);
            srcS2  = src_stride2 /sizeof(double);

            dstS1  = dststride1 /sizeof(double);
            dstS2  = dststride2 /sizeof(double);

            if( height < width) return status::status_param_unsupported;
            /* Copy array to buffer */
            for(j = 0; j < height; j++)
            {
                for(i = 0; i < width; i++)
                {
                    pdst[dstS1*j+i*dstS2] = psrc[srcS1*j+i*srcS2];
                }
            }

            int size = (width == height) ? (width - 1):width;

            /* QR factorization */
            for(j = 0; j < size; j++)
            {
                norm = 0.0;
                for(i = j; i < height; i++)
                {
                    norm += pdst[dstS1*i+j*dstS2] * pdst[dstS1*i+j*dstS2];
                }

                if(fabs(norm) < EPS52)
                {
                    return status::status_handle_invalid;
                }

                norm = (double)sqrt(norm);
                norm = (pdst[dstS1*j+j*dstS2] > 0) ? norm:(-norm);
                norm += pdst[dstS1*j+j*dstS2];
                norm = (double)1.0 / norm;

                /* Culculate beta factor */
                sum = 1.0;
                pbuffer[j] = 1.0;

                for(i = j + 1; i < height; i++)
                {
                    pbuffer[i] = pdst[dstS1*i+j*dstS2] * norm;
                    sum += pbuffer[i] * pbuffer[i];
                }
                beta = (double)(-2.0) / sum;

                /*Multiply by Househokder matri_x*/
                for(i = j; i < width; i++)
                {
                    sum = pdst[dstS1*j+i*dstS2];
                    for(l = j + 1; l < height; l++)
                    {
                        sum += pdst[dstS1*l+i*dstS2] * pbuffer[l];
                    }
                    sum *= beta;
                    for(l = j; l < height; l++)
                    {
                        pdst[dstS1*l+i*dstS2] += sum * pbuffer[l];
                    }
                }

                /* Store Householder vector */
                for(i=j+1; i<height; i++)
                {
                    pdst[dstS1*i+j*dstS2] = pbuffer[i];
                }
            }

            return status::status_no_error;
        }

        status REFCALL math_projection::rs_qr_back_subst_mva_64f(const double* psrc1, int src1stride1, int src1stride2, double* pbuffer,
                const double* psrc2, int src2stride0, int src2stride2, double* pdst,
                int dststride0, int dststride2, int width, int height, int count)
        {
            double  sum, beta, w;
            int  i, j, k, size;
            int  src1S1, src1S2;
            int  src2S2, dstS2;
            int  src2S0, dstS0;

            /* check array pointers */
            if( 0 == psrc1 || 0 == psrc2 || 0 == pdst ) return status::status_handle_invalid;

            /* check buffer pointer */
            if( 0 == pbuffer ) return status::status_handle_invalid;

            /* check values of matri_x sizes */
            if( 0 >= width  ) return status::status_param_unsupported;
            if( 0 >= height ) return status::status_param_unsupported;

            /* check values of count */
            if( 0 >= count ) return status::status_param_unsupported;

            /* check: data stride should be dividen to the size of data type */
            if( 0 >= src1stride1 || 0 != src1stride1 % sizeof(double)) return status::status_param_unsupported;
            if( 0 >= src1stride2 || 0 != src1stride2 % sizeof(double)) return status::status_param_unsupported;
            if( 0 >= src2stride2 || 0 != src2stride2 % sizeof(double)) return status::status_param_unsupported;

            if( 0 >= dststride0  || 0 != dststride0  % sizeof(double)) return status::status_param_unsupported;
            if( 0 >= dststride2  || 0 != dststride2  % sizeof(double)) return status::status_param_unsupported;

            /* Mathematical operation code */

            src1S1 = src1stride1/sizeof(double);
            src1S2 = src1stride2/sizeof(double);

            src2S0 = src2stride0/sizeof(double);
            dstS0  = dststride0 /sizeof(double);

            src2S2 = src2stride2/sizeof(double);
            dstS2  = dststride2 /sizeof(double);

            for(k = 0; k < count; k++)
            {
                /* Copy vector to buffer */
                for(i=0; i<height; i++)
                {
                    pbuffer[i] = psrc2[src2S0*k+i*src2S2];
                }
                size = (width == height) ? (width-1):width;

                for(j=0; j<size; j++)
                {
                    /* Culculate beta and w */
                    beta = 1.0;
                    w = pbuffer[j];
                    for(i = j + 1; i < height; i++)
                    {
                        beta += psrc1[src1S1*i+j*src1S2] * psrc1[src1S1*i+j*src1S2];
                        w += pbuffer[i] * psrc1[src1S1*i+j*src1S2];
                    }

                    beta = (double)(-2.0) / beta;
                    w *= beta;

                    /*Multiply by Househokder matri_x*/
                    pbuffer[j] += w;
                    for(i = j + 1; i < height; i++)
                    {
                        pbuffer[i] += psrc1[src1S1*i+j*src1S2] * w;
                    }
                }
                pdst[dstS0*k+(width-1)*dstS2] = pbuffer[width-1] / psrc1[src1S1*(width-1)+(width-1)*src1S2];
                for(j = width - 1; j > 0; j--)
                {
                    sum = 0.0;
                    for(i = j; i < width; i++)
                    {
                        sum += psrc1[src1S1*(j-1)+i*src1S2] * pdst[dstS0*k+i*dstS2];
                    }
                    pdst[dstS0*k+(j-1)*dstS2] = (pbuffer[j-1] - sum) / psrc1[src1S1*(j-1)+(j-1)*src1S2];
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

        status r_own_iuvmap_invertor ( const pointF32 *uvmap, int uvmap_step, sizeI32 uvmap_size, rect uvmap_roi,
                                       pointF32 *uvInv, int uvinv_step, sizeI32 uvinv_size, rect uvinv_roi, int uvinv_units_is_relative, pointF32 threshold )
        {
            typedef struct
            {
                double x;
                double y;
            } point_64f;


            point_64f pvalid_pi_x[4];
            int r, c, xmin,xmax,ymin,ymax, num_pi_x;
            float  fxmin,fxmax,fymin,fymax;
            double a0, b0, c0, a1, b1, c1, d1, e1, d0, e0;
            double dx0, dy0, dx1, dy1, dx2, dy2, dx3, dy3, dx4, dy4;
            double *p0, *p1, *p2, *p3;
            double width_c = (double)uvinv_size.width;
            double height_c = (double)uvinv_size.height;
            int  xmin_uvinv_roi = uvinv_roi.x;
            int  ymin_uvinv_roi = uvinv_roi.y;
            int  xmax_uvinv_roi = uvinv_roi.x + uvinv_roi.width  - 1;
            int  ymax_uvinv_roi = uvinv_roi.y + uvinv_roi.height - 1;
            int i_y, i_x;

            double x_norming = uvinv_units_is_relative?(1. / (double)uvmap_size.width):1.f;
            double y_norming = uvinv_units_is_relative?(1. / (double)uvmap_size.height):1.f;

            unsigned char *uvmap_ptr0 = (unsigned char*)uvmap + uvmap_roi.x * sizeof(pointF32) + uvmap_roi.y * uvmap_step;
            unsigned char *uvmap_ptr1 = uvmap_ptr0 + uvmap_step;
            unsigned char *puvinv    = (unsigned char*)uvInv;
            pointF32 *uv_ptr0, *uv_ptr1, *uvinv_ptr;
            // Since RGB has smaller FOV than depth, we can sweep through a subset of the depth pi_xels
            for ( r=0; r < uvmap_roi.height-1; r++ )
            {
                uv_ptr0 = (pointF32*)uvmap_ptr0;
                uv_ptr1 = (pointF32*)uvmap_ptr1;
                for ( c=0; c < uvmap_roi.width-1; c++ )
                {
                    // Get the number of valid pi_xels (around (c,r))
                    const double posC = (uvmap_roi.x + c + 0.5f) * x_norming;
                    const double posR = (uvmap_roi.y + r + 0.5f) * y_norming;

                    num_pi_x = 0;
                    if( uv_ptr0[c].x >= 0.f )
                    {
                        pvalid_pi_x[num_pi_x].x = uv_ptr0[c].x;
                        pvalid_pi_x[num_pi_x].y = uv_ptr0[c].y;
                        num_pi_x ++;
                    }
                    if( uv_ptr0[c+1].x >= 0.f )
                    {
                        pvalid_pi_x[num_pi_x].x = uv_ptr0[c+1].x;
                        pvalid_pi_x[num_pi_x].y = uv_ptr0[c+1].y;
                        num_pi_x ++;
                    }
                    if (num_pi_x == 0)
                        continue;

                    if( uv_ptr1[c].x >= 0.f )
                    {
                        pvalid_pi_x[num_pi_x].x = uv_ptr1[c].x;
                        pvalid_pi_x[num_pi_x].y = uv_ptr1[c].y;
                        num_pi_x ++;
                    }
                    if( uv_ptr1[c+1].x >= 0.f )
                    {
                        pvalid_pi_x[num_pi_x].x = uv_ptr1[c+1].x;
                        pvalid_pi_x[num_pi_x].y = uv_ptr1[c+1].y;
                        num_pi_x ++;
                    }

                    if (num_pi_x < 3)
                        continue;

                    pvalid_pi_x[0].x *= width_c; pvalid_pi_x[0].y *= height_c;
                    pvalid_pi_x[1].x *= width_c; pvalid_pi_x[1].y *= height_c;
                    pvalid_pi_x[2].x *= width_c; pvalid_pi_x[2].y *= height_c;
                    p0=(double*)&pvalid_pi_x[0];
                    p1=(double*)&pvalid_pi_x[1];
                    p2=(double*)&pvalid_pi_x[2];

                    if ( num_pi_x == 4 )
                    {
                        // Use pointers instead of datacopy
                        double v_x[4];
                        double v_y[4];

                        pvalid_pi_x[3].x *= width_c; pvalid_pi_x[3].y *= height_c;
                        p3=(double*)&pvalid_pi_x[3];

                        v_x[0] = p0[0];
                        v_x[1] = p1[0];
                        v_x[2] = p2[0];
                        v_x[3] = p3[0];

                        fxmin = min_of_array(v_x, 4);
                        fxmax = max_of_array(v_x, 4);

                        xmin = (int)ceil(fxmin);
                        xmax = (int)fxmax;
                        if (xmin < xmin_uvinv_roi) xmin = xmin_uvinv_roi;
                        if (xmax > xmax_uvinv_roi) xmax = xmax_uvinv_roi;

                        v_y[0] = p0[1];
                        v_y[1] = p1[1];
                        v_y[2] = p2[1];
                        v_y[3] = p3[1];

                        fymin = min_of_array(v_y, 4);
                        fymax = max_of_array(v_y, 4);

                        if(fxmax - fxmin > threshold.x || fymax - fymin > threshold.y)
                        {
                            continue;
                        }

                        ymin = (int)ceil(fymin);
                        ymax = (int)fymax;
                        if (ymin < ymin_uvinv_roi) ymin = ymin_uvinv_roi;
                        if (ymax > ymax_uvinv_roi) ymax = ymax_uvinv_roi;

                        // TriangleInteriorTest
                        if (p0[0] > p1[0]) std::swap( p0, p1 );
                        if (p0[1] > p2[1]) std::swap( p0, p2 );
                        if (p2[0] > p3[0]) std::swap( p2, p3 );
                        if (p1[1] > p3[1]) std::swap( p1, p3 );

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

                        puvinv = ((unsigned char*)uvInv + ymin * uvinv_step);
                        for (i_y = ymin ; i_y <= ymax ; ++i_y)
                        {
                            a1 = a0;
                            b1 = b0;
                            c1 = c0;
                            d1 = d0;
                            e1 = e0;

                            uvinv_ptr = (pointF32*)puvinv;
                            for (i_x=xmin ; i_x <= xmax ; ++i_x)
                            {
                                a1 -= dy0;
                                b1 -= dy1;
                                c1 -= dy2;
                                d1 -= dy3;
                                e1 -= dy4;
                                if (uvinv_ptr[i_x].x==-1)
                                {
                                    if( b1 >= 0 )
                                    {
                                        if( a1 >= 0 )
                                        {
                                            if( c1 >= 0 )
                                            {
                                                uvinv_ptr[i_x].x = (float)posC; uvinv_ptr[i_x].y = (float)posR;
                                                continue;
                                            }
                                        }
                                        if( d1 >= 0 ) {
                                            if( e1 >= 0 ) {
                                                uvinv_ptr[i_x].x = (float)posC; uvinv_ptr[i_x].y = (float)posR;
                                                continue;
                                            }
                                        }
                                    } else {
                                        if( a1 < 0 )
                                        {
                                            if( c1 < 0 )
                                            {
                                                uvinv_ptr[i_x].x = (float)posC; uvinv_ptr[i_x].y = (float)posR;
                                                continue;
                                            }
                                        }
                                        if( d1 < 0 )
                                        {
                                            if( e1 < 0 )
                                            {
                                                uvinv_ptr[i_x].x = (float)posC; uvinv_ptr[i_x].y = (float)posR;
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
                            puvinv += uvinv_step;
                        }
                    }
                    else if ( num_pi_x == 3 )
                    { // only 3 elements detected
                        double v_x[] = {p0[0], p1[0], p2[0]};
                        double v_y[] = {p0[1], p1[1], p2[1]};

                        fxmin = min_of_array(v_x, 3);
                        fxmax = max_of_array(v_x, 3);

                        xmin = (int)ceil(fxmin);
                        xmax = (int)fxmax;

                        if (xmin < xmin_uvinv_roi) xmin = xmin_uvinv_roi;
                        if (xmax > xmax_uvinv_roi) xmax = xmax_uvinv_roi;

                        fymin = min_of_array(v_y, 3);
                        fymax = max_of_array(v_y, 3);

                        if(fxmax - fxmin > threshold.x || fymax - fymin > threshold.y)
                        {
                            continue;
                        }

                        ymin = (int)ceil(fymin);
                        ymax = (int)fymax;
                        if (ymin < ymin_uvinv_roi) ymin = ymin_uvinv_roi;
                        if (ymax > ymax_uvinv_roi) ymax = ymax_uvinv_roi;

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

                        puvinv = ((unsigned char*)uvInv + ymin * uvinv_step);
                        for (i_y = ymin ; i_y <= ymax ; ++i_y)
                        {
                            a1 = a0;
                            b1 = b0;
                            c1 = c0;

                            uvinv_ptr = (pointF32*)puvinv;
                            for (i_x=xmin ; i_x <= xmax ; ++i_x)
                            {
                                a1 -= dy0;
                                b1 -= dy1;
                                c1 -= dy2;
                                if (uvinv_ptr[i_x].x==-1) {
                                    if( a1 >= 0 )
                                    {
                                        if( b1 >= 0 )
                                        {
                                            if( c1 >= 0 )
                                            {
                                                uvinv_ptr[i_x].x = (float)posC; uvinv_ptr[i_x].y = (float)posR;
                                                continue;
                                            }
                                        }
                                    } else {
                                        if( b1 < 0 )
                                        {
                                            if( c1 < 0 )
                                            {
                                                uvinv_ptr[i_x].x = (float)posC; uvinv_ptr[i_x].y = (float)posR;
                                                continue;
                                            }
                                        }
                                    }
                                }
                            }
                            a0 += dx0;
                            b0 += dx1;
                            c0 += dx2;
                            puvinv += uvinv_step;
                        }


                    } // endif num_pi_x
                }
                uvmap_ptr0 += uvmap_step;
                uvmap_ptr1 += uvmap_step;
            }

            return status::status_no_error;
        }



    }
}
