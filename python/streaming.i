/* File : example.i */
%module streaming
%include "carrays.i"
%include "cdata.i"

%{

#include "/usr/local/include/librealsense/rs.hpp"

#include <string>
#include <memory>
#include <iostream>
#include <vector>
#include "unistd.h"
%}


%ignore motion_callback;
%ignore timestamp_callback;
%ignore frame;
%ignore frame_callback;
%ignore log_callback;

%include "/usr/local/include/librealsense/rs.hpp"


