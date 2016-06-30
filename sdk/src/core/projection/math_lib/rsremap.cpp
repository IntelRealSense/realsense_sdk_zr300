// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "rs_math.h"

#define EPS 1e-7

//Added
status REFCALL rsRemap_16u_C1R (const unsigned short* pSrc, rs::core::sizeI32 srcSize, int srcStep, const float* pxyMap,
                                int xyMapStep, unsigned short* pDst, rs::core::sizeI32 dstRoiSize, int dstStep, int interpolation_type, unsigned short defaultValue)
{
    int x, y, sx, sy;
    if (pSrc == 0 || pDst == 0 || pxyMap == 0) return status_handle_invalid;
    if (srcSize.width <= 0 || srcSize.height <= 0 || dstRoiSize.width <= 0 || dstRoiSize.height <= 0) return status_data_not_initialized;
    if(interpolation_type != 0) return status_data_not_initialized;

    for ( y = 0; y < dstRoiSize.height; y ++ )
    {
        for ( x = 0; x < dstRoiSize.width; x++ )
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

    return status_no_error;
}
