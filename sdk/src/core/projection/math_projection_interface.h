// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "rs/core/status.h"
#include "rs/core/types.h"
#include "math.h"

namespace rs
{
    namespace core
    {

#define REFCALL

        struct projection_spec_32f;
        typedef struct projection_spec_32f projection_spec_32f;

        class math_projection
        {
        public:
            math_projection();

            rs::core::status REFCALL rs_projection_init_32f(rs::core::sizeI32 roiSize, float cameraSrc[4], float invDistortion[5], projection_spec_32f *pSpec);

            rs::core::status REFCALL rs_3d_array_projection_32f(const float *pSrc, float *pDst, int length, float cameraSrc[4],
                                                                float invDistortionSrc[5], float rotation[9], float translation[3],
                                                                float distortionDst[5], float cameraDst[4]);

            rs::core::status REFCALL rs_projection_16u32f_c1cxr(const unsigned short *pSrc, rs::core::sizeI32 roiSize, int srcStep, float *pDst, int dstStep,
                                                                float rotation[9], float translation[3], float distortionDst[5],
                                                                float cameraDst[4], const projection_spec_32f *pSpec);

            rs::core::status REFCALL rs_projection_get_size_32f(rs::core::sizeI32 roiSize, int *pSpecSize);

            rs::core::status REFCALL rs_remap_16u_c1r(const unsigned short* pSrc, rs::core::sizeI32 srcSize, int srcStep, const float* pxyMap,
                                                      int xyMapStep, unsigned short* pDst, rs::core::sizeI32 dstRoiSize,
                                                      int dstStep, int interpolation_type, unsigned short defaultValue);

            rs::core::status REFCALL rs_uvmap_filter_32f_c2ir(float *pSrcDst, int srcDstStep, rs::core::sizeI32 roiSize,
                                                              const unsigned short *pDepth, int depthStep, unsigned short invalidDepth);

            rs::core::status REFCALL rs_uvmap_invertor_32f_c2r(const float *pSrc, int srcStep, rs::core::sizeI32 srcSize, rs::core::rect srcRoi,
                                                               float *pDst, int dstStep, rs::core::sizeI32 dstSize, int unitsIsRelative);

            rs::core::status REFCALL rs_qr_decomp_m_64f(const double*  pSrc,  int srcStride1, int srcStride2,
                                                        double*  pBuffer,
                                                        double*  pDst,  int dstStride1, int dstStride2,
                                                        int width, int height);

            rs::core::status REFCALL rs_qr_back_subst_mva_64f(const double*  pSrc1,  int src1Stride1, int src1Stride2, double*  pBuffer,
                                                              const double*  pSrc2,  int src2Stride0, int src2Stride2,
                                                              double*  pDst,   int dstStride0,  int dstStride2, int width, int height, int count);
        };
    }
}
