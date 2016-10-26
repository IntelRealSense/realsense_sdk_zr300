// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "rs_stream_impl.h"
#include "include/linear_algebra.h"

namespace rs
{
    namespace playback
    {
        void rs_stream_impl::create_extrinsics(const std::map<rs_stream,std::unique_ptr<rs_stream_impl>> & streams)
        {
            for(auto it = streams.begin(); it != streams.end(); ++it)
            {
                //set identity matrix if stream types are similar
                if(m_stream_info.stream == it->first)
                {
                    m_extrinsics_to[it->first] = { 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 };
                    continue;
                }
                if(it->first ==  rs_stream::RS_STREAM_DEPTH)
                {
                    m_extrinsics_to[it->first] = m_stream_info.profile.extrinsics;
                    continue;
                }
                //rs_extrinsics is assumed to be identical to rs::utils::pose
                rs::utils::pose from_pose,to_pose;//matrix for linear_algebra lib
                auto from = it->second->get_stream_info().profile.extrinsics;
                memcpy(&from_pose, &m_stream_info.profile.extrinsics, sizeof(from_pose));// from_stream -> depth_stream
                memcpy(&to_pose, &from, sizeof(to_pose));// to_stream -> depth_stream
                auto transform = from_pose * inverse(to_pose);
                rs_extrinsics extrin;
                memcpy(&extrin, &transform, sizeof(extrin));
                m_extrinsics_to[it->first] = extrin;
            }
        }

        rs_extrinsics rs_stream_impl::get_extrinsics_to(const rs_stream_interface &r) const
        {
            auto stream_type = r.get_stream_type();
            auto ext_to = m_extrinsics_to.at(stream_type);
            return ext_to;
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
