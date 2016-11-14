// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "samples_time_sync_external_camera.h"

using namespace std;
using namespace rs::core;
using namespace rs::utils;

bool rs::utils::samples_time_sync_external_camera::sync_all(streams_map& streams, motions_map& motions, rs::core::correlated_sample_set &sample_set)
{
    if (empty_list_exists())
    {
        return false;
    }

    //Go over all the lists and get every stream and then motion
    for (auto& pair : streams)
    {
        assert (pair.second.size() > 0);

        stream_type st = pair.first;
        image_interface* image = pair.second.back().get();
        image->add_ref();
        sample_set[st] = image;
        pair.second.pop_back();
    }

    for (auto& pair : motions)
    {
        assert (pair.second.size() > 0);

        motion_type mt = pair.first;
        sample_set[mt] = pair.second.back();
        pair.second.pop_back();
    }
    return true;
}
