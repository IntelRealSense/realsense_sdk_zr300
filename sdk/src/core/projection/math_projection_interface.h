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

            rs::core::status REFCALL rs_projection_init_32f(rs::core::sizeI32 roi_size, float camera_src[4], float inv_distortion[5], projection_spec_32f *pspec);

            rs::core::status REFCALL rs_3d_array_projection_32f(const float *psrc, float *pdst, int length, float camera_src[4],
                    float inv_distortion_src[5], float rotation[9], float translation[3],
                    float distortion_dst[5], float camera_dst[4]);

            rs::core::status REFCALL rs_projection_16u32f_c1cxr(const unsigned short *psrc, rs::core::sizeI32 roi_size, int src_step, float *pdst, int dst_step,
                    float rotation[9], float translation[3], float distortion_dst[5],
                    float camera_dst[4], const projection_spec_32f *pspec);

            rs::core::status REFCALL rs_projection_get_size_32f(rs::core::sizeI32 roi_size, int *pspec_size);

            rs::core::status REFCALL rs_remap_16u_c1r(const unsigned short* psrc, rs::core::sizeI32 src_size, int src_step, const float* pxy_map,
                    int xy_map_step, unsigned short* pdst, rs::core::sizeI32 dstroi_size,
                    int dst_step, int interpolation_type, unsigned short default_value);

            rs::core::status REFCALL rs_uvmap_filter_32f_c2ir(float *psrc_dst, int srcdst_step, rs::core::sizeI32 roi_size,
                    const unsigned short *pdepth, int depth_step, unsigned short invalid_depth);

            rs::core::status REFCALL rs_uvmap_invertor_32f_c2r(const float *psrc, int src_step, rs::core::sizeI32 src_size, rs::core::rect src_roi,
                    float *pdst, int dst_step, rs::core::sizeI32 dst_size, int units_is_relative, pointF32  threshold);

            rs::core::status REFCALL rs_qr_decomp_m_64f(const double*  psrc,  int src_stride1, int src_stride2,
                    double*  pbuffer,
                    double*  pdst,  int dststride1, int dststride2,
                    int width, int height);

            rs::core::status REFCALL rs_qr_back_subst_mva_64f(const double*  psrc1,  int src1stride1, int src1stride2, double*  pbuffer,
                    const double*  psrc2,  int src2stride0, int src2stride2,
                    double*  pdst,   int dststride0,  int dststride2, int width, int height, int count);
        };
    }
}
