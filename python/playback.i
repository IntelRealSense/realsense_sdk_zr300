// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

%module playback
%include "carrays.i"
%include "cdata.i"
%include "exception.i"

%{
#include "librealsense/rs.hpp"
#include "rs/playback/playback_context.h"
#include "rs/playback/playback_device.h"
#include "rs/core/types.h"
%}


%exception {
  try {
    $action
  } catch(rs::error &e) {
    std::string s("rs::error - "), s2(e.what());
    s = s + s2;
    SWIG_exception(SWIG_RuntimeError, s.c_str());
  } catch(...) {
    SWIG_exception(SWIG_RuntimeError,"Unknown exception");
  }
}

%include "rs/playback/playback_context.h"

// We want to use context class from playback, but we should use devise class from rs.hpp
// We are disable context and all callback functions. Callback functions will be enabled in future.
%ignore context;
%ignore motion_callback;
%ignore timestamp_callback;
%ignore frame;
%ignore frame_callback;
%ignore log_callback;

%include "librealsense/rs.hpp"
