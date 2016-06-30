// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs/core/status.h"
#include "rs/core/types.h"
#include "math.h"

#define REFCALL

using namespace rs::core;

struct ProjectionSpec_32f;
typedef struct ProjectionSpec_32f ProjectionSpec_32f;


#define MINABS_32F ( 1.175494351e-38f )
#define EPS52 ( 2.2204460492503131e-016 )

#define ABS( a ) ( ((a) < 0) ? (-(a)) : (a) )
#define MAX( a, b ) ( ((a) > (b)) ? (a) : (b) )
#define MIN( a, b ) ( ((a) < (b)) ? (a) : (b) )

//Line 409
status REFCALL rsProjectionInit_32f ( rs::core::sizeI32 roiSize, float cameraSrc[4], float invDistortion[5], ProjectionSpec_32f *pSpec );

//Multiple places
status REFCALL rs3DArrayProjection_32f (const float *pSrc, float *pDst, int length, float cameraSrc[4],
                                        float invDistortionSrc[5], float rotation[9], float translation[3], float distortionDst[5], float cameraDst[4]);

//Multiple places
status REFCALL rsProjection_16u32f_C1CXR ( const unsigned short *pSrc, rs::core::sizeI32 roiSize, int srcStep, float *pDst, int dstStep,
        float rotation[9], float translation[3], float distortionDst[5], float cameraDst[4], const ProjectionSpec_32f *pSpec);

//Line 403
status REFCALL rsProjectionGetSize_32f ( rs::core::sizeI32 roiSize, int *pSpecSize );


//Line 1009
status REFCALL rsRemap_16u_C1R (const unsigned short* pSrc, rs::core::sizeI32 srcSize, int srcStep, const float* pxyMap,
                                int xyMapStep, unsigned short* pDst, rs::core::sizeI32 dstRoiSize, int dstStep, int interpolation_type, unsigned short defaultValue);

//Line 601
status REFCALL rsUVmapFilter_32f_C2IR( float *pSrcDst, int srcDstStep, rs::core::sizeI32 roiSize, const unsigned short *pDepth, int depthStep, unsigned short invalidDepth );


//Line 652 1007
status REFCALL rsUVmapInvertor_32f_C2R( const float *pSrc, int srcStep, rs::core::sizeI32 srcSize, rect srcRoi, float *pDst, int dstStep,
                                        rs::core::sizeI32 dstSize, int unitsIsRelative );

//Not implemented yet
status REFCALL  rsQRDecomp_m_64f (const double*  pSrc,  int srcStride1, int srcStride2,
                                  double*  pBuffer,
                                  double*  pDst,  int dstStride1, int dstStride2,
                                  int width, int height);

//Not implemented yet
status REFCALL rsQRBackSubst_mva_64f (const double*  pSrc1,  int src1Stride1, int src1Stride2,
                                      double*  pBuffer,
                                      const double*  pSrc2,  int src2Stride0, int src2Stride2,
                                      double*  pDst,   int dstStride0,  int dstStride2,
                                      int width, int height, int count);
