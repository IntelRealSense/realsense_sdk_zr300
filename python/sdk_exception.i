// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

%include "exception.i"


%exception {
  try {
    $action
  } catch(rs::error &e) {
    std::string s("rs::error - "), s2(e.what());
    s = s + s2;
    SWIG_exception(SWIG_RuntimeError, s.c_str());
  } catch (const std::exception& e) {
    std::string s("std::exception - "), s2(e.what());
    s = s + s2;
    SWIG_exception(SWIG_RuntimeError, s.c_str());
  } catch(...) {
    SWIG_exception(SWIG_RuntimeError,"Unknown exception");
  }
}
