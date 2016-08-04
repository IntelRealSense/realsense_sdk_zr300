// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "rs_stream_impl.h"
#include "linear_algebra/linear_algebra.h"

namespace rs
{
    namespace playback
    {
        rs_extrinsics rs_stream_impl::get_extrinsics_to(const rs_stream_interface &r) const
        {
            auto to_si = static_cast<rs_stream_impl*>(const_cast<rs_stream_interface*>(&r));
            if(to_si->get_stream_type() ==  rs_stream::RS_STREAM_DEPTH)
                return m_stream_info.profile.extrinsics;
            if(m_stream_info.stream != rs_stream::RS_STREAM_DEPTH)
            {
                rs_extrinsics empty = {};
                return empty;
            }
            rs_extrinsics ext = to_si->get_extrinsics_to(*static_cast<const rs_stream_interface*>(this));
            const rs::utils::pose mat = {ext.rotation[0], ext.rotation[1], ext.rotation[2],
                                         ext.rotation[3], ext.rotation[4], ext.rotation[5],
                                         ext.rotation[6], ext.rotation[7], ext.rotation[8],
                                         ext.translation[0], ext.translation[1], ext.translation[2]
                                        };

            auto inv = inverse(mat);
            rs_extrinsics rv = {inv.orientation.x.x, inv.orientation.x.y, inv.orientation.x.z,
                                inv.orientation.y.x, inv.orientation.y.y, inv.orientation.y.z,
                                inv.orientation.z.x, inv.orientation.z.y, inv.orientation.z.z,
                                inv.position.x, inv.position.y, inv.position.z
                               };
            return rv;
        }

        void rs_stream_impl::get_mode(int mode, int *w, int *h, rs_format *f, int *fps) const
        {
            if(mode != 0)
            {
                *w = *h = *fps = 0;
                *f = rs_format::RS_FORMAT_ANY;
                return;
            }
            *w = m_stream_info.profile.info.width;
            *h = m_stream_info.profile.info.height;
            *f = m_stream_info.profile.info.format;
            *fps = m_stream_info.profile.frame_rate;
        }
    }
}
