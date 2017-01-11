// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

%module streaming
%include "carrays.i"
%include "cdata.i"
%include "exception.i"

%{
#include "librealsense/rs.hpp"
%}

%exception {
  try {
    $action
  } catch(rs::error &e) {
    std::string s("myModule error: "), s2(e.what());
    s = s + s2;
    SWIG_exception(SWIG_RuntimeError, s.c_str());
  } catch(...) {
    SWIG_exception(SWIG_RuntimeError,"Unknown exception");
  }
}

// We disable all callback functions. Callback functions will be enabled in future.
%ignore motion_callback;
%ignore timestamp_callback;
%ignore frame;
%ignore frame_callback;
%ignore log_callback;

%include "librealsense/rs.hpp"
