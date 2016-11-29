/* File : example.i */
%module image
%include "carrays.i"
%include "cdata.i"
%include "stdint.i"

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
%ignore create_instance_from_librealsense_frame;

%include "rs/core/types.h"
%array_functions(pointF32, pointF32Array);

%include "rs/core/image_interface.h"
