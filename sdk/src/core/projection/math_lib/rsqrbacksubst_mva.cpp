// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "rs_math.h"


status REFCALL rsQRBackSubst_mva_64f( const double* pSrc1, int src1Stride1, int src1Stride2, double* pBuffer, const double* pSrc2, int src2Stride0, int src2Stride2, double* pDst, int dstStride0, int dstStride2, int width, int height, int count)
{
    double  sum, beta, w;
    int  i, j, k, size;
    int  src1S1, src1S2;
    int  src2S2, dstS2;
    int  src2S0, dstS0;

    /* check array pointers */
    if( 0 == pSrc1 || 0 == pSrc2 || 0 == pDst ) return status_handle_invalid;

    /* check buffer pointer */
    if( 0 == pBuffer ) return status_handle_invalid;

    /* check values of matrix sizes */
    if( 0 >= width  ) return status_param_unsupported;
    if( 0 >= height ) return status_param_unsupported;

    /* check values of count */
    if( 0 >= count ) return status_param_unsupported;

    /* check: data stride should be dividen to the size of data type */
    if( 0 >= src1Stride1 || 0 != src1Stride1 % sizeof(double)) return status_param_unsupported;
    if( 0 >= src1Stride2 || 0 != src1Stride2 % sizeof(double)) return status_param_unsupported;
    if( 0 >= src2Stride2 || 0 != src2Stride2 % sizeof(double)) return status_param_unsupported;

    if( 0 >= dstStride0  || 0 != dstStride0  % sizeof(double)) return status_param_unsupported;
    if( 0 >= dstStride2  || 0 != dstStride2  % sizeof(double)) return status_param_unsupported;

    /* Mathematical operation code */

    src1S1 = src1Stride1/sizeof(double);
    src1S2 = src1Stride2/sizeof(double);

    src2S0 = src2Stride0/sizeof(double);
    dstS0  = dstStride0 /sizeof(double);

    src2S2 = src2Stride2/sizeof(double);
    dstS2  = dstStride2 /sizeof(double);

    for(k=0; k<count; k++)
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
            for(i=j+1; i<height; i++)
            {
                beta += pSrc1[src1S1*i+j*src1S2] * pSrc1[src1S1*i+j*src1S2];
                w += pBuffer[i] * pSrc1[src1S1*i+j*src1S2];
            }

            beta = (double)(-2.0) / beta;
            w *= beta;

            /*Multiply by Househokder matrix*/
            pBuffer[j] += w;
            for(i=j+1; i<height; i++)
            {
                pBuffer[i] += pSrc1[src1S1*i+j*src1S2] * w;
            }
        }
        pDst[dstS0*k+(width-1)*dstS2] = pBuffer[width-1] / pSrc1[src1S1*(width-1)+(width-1)*src1S2];
        for(j=width-1; j>0; j--)
        {
            sum = 0.0;
            for(i=j; i<width; i++)
            {
                sum += pSrc1[src1S1*(j-1)+i*src1S2] * pDst[dstS0*k+i*dstS2];
            }
            pDst[dstS0*k+(j-1)*dstS2] = (pBuffer[j-1] - sum) / pSrc1[src1S1*(j-1)+(j-1)*src1S2];
        }
    }

    return status_no_error;
}


