# Intel® Linux RealSense™ SDK

[ ![Release] [release-image] ] [releases]
[ ![License] [license-image] ] [license]

[release-image]: http://img.shields.io/badge/release-0.3.0-blue.svg?style=flat
[releases]: https://github.com/IntelRealSense/realsense_sdk

[license-image]: http://img.shields.io/badge/license-Apache--2-blue.svg?style=flat
[license]: LICENSE


The Linux RealSense™ SDK provides libraries, tools and samples to develop applications using Intel® RealSense™ cameras over librealsense API. 
The SDK provides functionality of record and playback of camera streams for test and validation.The SDK includes libraries which support the camera stream projection of streams into a common world-space viewpoint, and libraries which enable use of multiple Middleware modules simultaneously for common multi-modal scenarios.  


# Table of Contents 
* [Compatible Devices](#compatible-devices)
* [Supported Platforms](#compatible-platforms)
* [Compatible Languages](#supported-languages-and-frameworks)
* [Functionality](#functionality)
* [Installation Guide](#installation-guide)

## Compatible Devices

RealSense ZR300


## Compatible Platforms

The library is written in standards-conforming C++11. It is developed and tested on the following platforms:

1. Ubuntu 16.04 x64 (GCC 4.9 toolchain)
2. Ostro Build 102 - Intel® RealSense™ layers for Yocto build can be found at https://github.com/IntelRealSense/meta-intel-realsense 


## Supported Languages and Frameworks

C++ 

## Functionality

**API is experimental and not an official Intel product. It is subject to incompatible API changes in future updates. Breaking API changes are noted through release numbers**

1. **Recording and Playback-**    
    The SDK provides recording and playback
    modules for application developers testing and validation usages.
    The modules are standalone utilities, which expose camera API, as
    defined by librealsense, and can replace the calls to the camera
    from the application. The record module is loading librealsense to
    access the physical camera, configure it and stream images. The
    module is saving to file all application operations on the camera
    and the outputs streams of the camera and motion module.  The
    playback module is supporting files which were recorded using the
    linux SDK recorder and the windows RSSDK recorder (till version 2016 R2).
2. **Frame data container -**  
    The SDK provides image container for raw image access, and basic image processing services, such as format
    conversion, mirror, rotation etc. It is caching the processing
    output, to optimize multiple requests of the same operation. The SDK
    provides access to camera and motion sensors output images using a
    frame container with synchronized images and samples from the
    relevant streams. The frame container includes all relevant raw
    buffers, metadata and information required to access the attached
    images.
3. **Spatial correlation and projection -**    
    The SDK provides a library which executes spatial correlation and projection functions. It receives the
    streams intrinsics and extrinsics parameters, as provided by the camera, and the depth image.
    The library provides:
    Depth image to color image pixels mapping, and color to depth.
    Depth / color image pixels (un)projection to/from world coordinates.
    Create full images:
    - uv map (depth to color)
    - inverse uv map (color to depth)
    - point cloud (same as the corresponding stream)
    - color mapped to depth and depth mapped to color (same as the corresponding stream)
4. **Tools -**
   - Spatial mapping and projection tool for visualization of mapping features

# Installation Guide

Dependencies list
-------------
In order to successfully compile and use the SDK, you should install the following list of dependencies

 - [librealsense v1.11.0](https://github.com/IntelRealSense/librealsense/tree/v1.11.0)
 - log4cxx
 - opencv3.1
 - cmake
 - gtest (googletest)
 
How to enable logging in your application
-------------

Prerequisites: 

 - Install log4cxx.

   >sudo apt-get install liblog4cxx10 liblog4cxx10-dev

1. log is not compiling by default . In order to compile it add -DBUILD_LOGGER=ON option to cmake
2. Copy "rslog.properties" file to your ~/realsense/logs/ folder. You may copy it to any other directory you want, in this case, create enviromental variable REALSENSE_SDK_LOG_PATH pointing to that folder.
3. Edit rslog.properties file to the output logs files you want to create. Root logger is the logger that always exists, but you may add your own logger. Pay attention to the log level hierarchy.
4. Inlude file "log_utils.h" to your source/header files.
5. Add "realsense_log_utils" to your link libraries (librealsense_log_utils.so)
6. Use defines from "log_utils.h" to log, (File name and ine number will be logged automatically) in example:
	> LOG_DEBUG("This is my demo DEBUG message number %d with string %s\n", 1, "HELLO WORLD");

   NOTE: Due to ABI issues, log4cxx.so should be compiled with the same compiler you use to compile RealSense SDK. The default log4cxx package contains GCC 4.9 compiled library for Ubuntu 14.04 and GCC 5.0 compiled 
   library for Ubuntu 16.04. Using different compiler may cause problems loading shared library. If you use compiler version different from default, please compile log4cxx from source code and install it.
   
   
## License

Copyright 2016 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this project except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
