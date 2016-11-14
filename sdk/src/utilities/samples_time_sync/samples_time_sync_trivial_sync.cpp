// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "samples_time_sync_trivial_sync.h"

using namespace std;
using namespace rs::core;
using namespace rs::utils;

bool rs::utils::samples_time_sync_trivial_sync::sync_all(streams_map& streams, motions_map& motions, rs::core::correlated_sample_set &sample_set)
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
        pair.second.front()->add_ref();
        sample_set[st] = pair.second.front().get();
        pair.second.pop_front();
    }

    for (auto& pair : motions)
    {
        assert (pair.second.size() > 0);

        motion_type mt = pair.first;
        sample_set[mt] = pair.second.front();
        pair.second.pop_front();
    }
    return true;
}
