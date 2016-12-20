// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once

template <typename Callback>
class streamer_interface
{
public:
    virtual bool init() = 0;
    virtual void start_streaming(Callback frame_callback) = 0;
    virtual void stop_streaming() = 0;
};