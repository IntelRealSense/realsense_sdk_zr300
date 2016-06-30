// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "rs_math.h"

status REFCALL rsQRDecomp_m_64f( const double* pSrc, int srcStride1, int srcStride2, double* pBuffer, double* pDst, int dstStride1, int dstStride2, int width, int height)
{
    double  sum, beta, norm;
    int  i, j, l, size;
    int  srcS1, dstS1;
    int  srcS2, dstS2;

    /* check array pointers */
    if( 0 == pSrc || 0 == pDst ) return status_handle_invalid;

    /* check buffer pointer */
    if( 0 == pBuffer ) return status_handle_invalid;

    /* check values of matrix sizes */
    if( 0 >= width  ) return status_param_unsupported;
    if( 0 >= height ) return status_param_unsupported;

    /* check: data stride should be dividen to the size of data type */
    if( 0 >= srcStride1 || 0 != srcStride1  % sizeof(double)) return status_param_unsupported;
    if( 0 >= srcStride2 || 0 != srcStride2  % sizeof(double)) return status_param_unsupported;

    if( 0 >= dstStride1 || 0 != dstStride1  % sizeof(double)) return status_param_unsupported;
    if( 0 >= dstStride2 || 0 != dstStride2  % sizeof(double)) return status_param_unsupported;

    /* Mathematical operation code */

    srcS1  = srcStride1 /sizeof(double);
    srcS2  = srcStride2 /sizeof(double);

    dstS1  = dstStride1 /sizeof(double);
    dstS2  = dstStride2 /sizeof(double);

    if( height < width) return status_param_unsupported;
    /* Copy array to buffer */
    for(j=0; j<height; j++)
    {
        for(i=0; i<width; i++)
        {
            pDst[dstS1*j+i*dstS2] = pSrc[srcS1*j+i*srcS2];
        }
    }

    size = (width == height) ? (width-1):width;

    /* QR factorization */
    for(j=0; j<size; j++)
    {
        norm = 0.0;
        for(i=j; i<height; i++)
        {
            norm += pDst[dstS1*i+j*dstS2] * pDst[dstS1*i+j*dstS2];
        }

        if( fabs(norm) < EPS52)
        {
            return status_handle_invalid;
        }

        norm = (double)sqrt(norm);
        norm = (pDst[dstS1*j+j*dstS2] > 0) ? norm:(-norm);
        norm += pDst[dstS1*j+j*dstS2];
        norm = (double)1.0 / norm;

        /* Culculate beta factor */
        sum = 1.0;
        pBuffer[j] = 1.0;

        for(i=j+1; i<height; i++)
        {
            pBuffer[i] = pDst[dstS1*i+j*dstS2] * norm;
            sum += pBuffer[i] * pBuffer[i];
        }
        beta = (double)(-2.0) / sum;

        /*Multiply by Househokder matrix*/
        for(i=j; i<width; i++)
        {
            sum = pDst[dstS1*j+i*dstS2];
            for(l=j+1; l<height; l++)
            {
                sum += pDst[dstS1*l+i*dstS2] * pBuffer[l];
            }
            sum *= beta;
            for(l=j; l<height; l++)
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

    return status_no_error;
}




