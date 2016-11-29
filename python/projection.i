/* File : example.i */
%module projection

%include "cpointer.i"
%include "carrays.i"
%include "cdata.i"

%{


#include <memory>
#include <vector>
#include <iostream>
#include <librealsense/rs.hpp>

#include "rs/utils/librealsense_conversion_utils.h"

#include "rs/core/types.h"
#include "rs_sdk.h"

using namespace rs::core;

%}


%ignore __inline;
%ignore CONSTRUCT_UID;

%include "rs/core/types.h"
%array_functions(rs::core::pointF32, pointF32Array);
%array_functions(rs::core::point3dF32, point3dF32Array);
%array_functions(unsigned char, ucharArray);
%pointer_cast(void *, unsigned char *, void_to_char)

%ignore convert_pixel_format(rs::core::pixel_format framework_pixel_format);


%include "rs/utils/librealsense_conversion_utils.h"
%include "rs/core/projection_interface.h"
