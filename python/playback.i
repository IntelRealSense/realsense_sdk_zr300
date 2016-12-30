// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

%module playback
%include "carrays.i"
%include "cdata.i"

%{
#include "librealsense/rs.hpp"
#include "rs/playback/playback_context.h"
#include "rs/playback/playback_device.h"
#include "rs/core/types.h"
%}




%include "rs/playback/playback_context.h"
    %ignore context;
    %ignore motion_callback;
    %ignore timestamp_callback;
    %ignore frame;
    %ignore frame_callback;
    %ignore log_callback;

%include "librealsense/rs.hpp"


