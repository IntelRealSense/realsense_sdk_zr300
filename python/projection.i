// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

%module projection

%include "cpointer.i"
%include "carrays.i"
%include "cdata.i"
%include "typemaps.i"
%include "stdint.i"
%include "sdk_exception.i"

%{
#include <librealsense/rs.hpp>
#include "rs/utils/librealsense_conversion_utils.h"
#include "rs_sdk.h"

using namespace rs::core;
%}


%include "rs/core/types.h"
%include "rs/core/status.h"
%include "rs/utils/librealsense_conversion_utils.h"

//code bellow generated python functions for working with arrays and pointers on C++ side
%array_functions(rs::core::pointF32, pointF32Array);
%array_functions(rs::core::point3dF32, point3dF32Array);
%array_functions(unsigned char, ucharArray);
%pointer_cast(void *, unsigned char *, void_to_char)


// flatnested is needed for nested structure
%feature("flatnested");
%include "rs/core/image_interface.h"
%feature("flatnested", "");

%include "rs/core/projection_interface.h"
