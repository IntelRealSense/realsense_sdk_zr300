/* File : example.i */
%module playback
%include "carrays.i"
%include "cdata.i"

%{

#include "/usr/local/include/librealsense/rs.hpp"
#include "rs/playback/playback_context.h"
#include "rs/playback/playback_device.h"
#include "rs/core/types.h"

#include <string>
#include <memory>
#include <iostream>
#include <vector>
#include "unistd.h"

%}




%include "rs/playback/playback_context.h"
    %ignore context;
    %ignore motion_callback;
    %ignore timestamp_callback;
    %ignore frame;
    %ignore frame_callback;
    %ignore log_callback;

%include "/usr/local/include/librealsense/rs.hpp"


