# Intel® Linux RealSense™ SDK

[ ![Release] [release-image] ] [releases]
[ ![License] [license-image] ] [license]

[release-image]: http://img.shields.io/badge/release-0.6.0-blue.svg?style=flat
[releases]: https://github.com/IntelRealSense/realsense_sdk/tree/v0.6.0

[license-image]: http://img.shields.io/badge/license-Apache--2-blue.svg?style=flat
[license]: LICENSE


The Linux RealSense™ SDK provides libraries, tools and samples to develop applications using Intel® RealSense™ cameras over librealsense API. 
The SDK provides functionality of record and playback of camera streams for test and validation.The SDK includes libraries which support the camera stream projection of streams into a common world-space viewpoint, and libraries which enable use of multiple Middleware modules simultaneously for common multi-modal scenarios.  


# Table of Contents 
* [Compatible Devices](#compatible-devices)
* [Supported Platforms](#compatible-platforms)
* [Compatible Languages](#supported-languages-and-frameworks)
* [Functionality](#functionality)
* [Dependencies list](#dependencies-list)
* [Learn More](#learn-more)

## Compatible Devices

RealSense ZR300


## Compatible Platforms

The library is written in standards-conforming C++11. It is developed and tested on Ubuntu 16.04 x64 (GCC 4.9 toolchain)


## Supported Languages and Frameworks

C++ 

## Functionality

**API is experimental and not an official Intel product. It is subject to incompatible API changes in future updates. Breaking API changes are noted through release numbers**

1. **Recording and Playback-**    
    - Record - the record module provides a utility to create a file, which can be used by the playback module to create a video source.
    The record module provides the same camera API as defined by the SDK (librealsense) and the record API to configure recording parameters such
    as output file and state (pause and resume).
    The record module loads librealsense to access the camera device and execute the set requests and reads, while writing the configuration and changes to the file.
    - Playback - the playback module provides a utility to create a video source from a file. 
    The playback module provides the same camera API as defined by the SDK (librealsense), and the playback API to configure recording
    parameters such as input file, playback mode, seek, and playback state (pause and resume).
    The playback module is supporting files which were recorded using the
    linux SDK recorder and the windows RSSDK recorder (till version 2016 R2).
2. **Frame data container -**  
    The SDK provides an image container for raw image access and basic image processing services, 
    such as format conversion, mirror, rotation, and more. It caches the processing output to optimize multiple requests of the same operation.
    The image container includes image metadata, which may be used by any pipeline component to attach additional data or computer vision (CV) module processing output
    to be used by other pipeline components. The SDK uses a correlated samples container to provide access to camera images and motion sensor samples from the relevant streams,
    which are time synchronized. The correlated samples container includes all relevant raw buffers, metadata, and information required to access the attached images. 
3. **Spatial correlation and projection -**    
    The Spatial Correlation & Projection library provides utilities for spatial mapping:
    - Map between color or depth image pixel coordinates and real world coordinates
    - Correlate depth and color images and align them in space
4. **Pipeline -**    
    The pipeline is a class, which abstracts the details of how the cognitive data is produced by the computer vision modules.
    Instead, the application can focus on consuming the computer vision output, leaving the camera configuration and streaming details for the pipeline to handle.
    The application merely has to configure the requested perceptual output, and handle the cognitive data it gets during streaming. 
5. **Samples -**
    - Projection sample - the sample demonstrates how to use the different spatial correlation and projection functions, from live camera and recorded file
    - Record and Playback sample - the sample demonstrates how to record and play back a file while the application is streaming, with and without an active CV module,
      with minimal changes to the application compared to live streaming.
    - Video module asynchronized sample -the sample demonstrates an application usage of a Computer Vision module, which implements asynchronous sample processing. 
    - Video module synchronized sample - the sample demonstrates an application usage of a Computer Vision module, which implements synchronous samples processing.
    - Fatal error recovery - the sample demonstrates how the application can recover from fatal error in one of the SDK components (CV module or core module), without having to terminate.
6. **Tools -**
   - Capture tool - provides GUI to view camera streams, create a new file from a live camera, and play a file of the supported formats. The tool provides options to render the camera or file images.
   - Projection tool - provides simple visualization of the projection functions output, to allow human eye detection of major offsets in the projection computation.
   - System Info tool - presents system data such as Linux name, Linux kernal version, CPU information etc.
7. **Utilities -**
   - Log - the SDK provide a logging library, which can be used by the SDK components and the application to log meaningful events. 
   - Time Sync utility - provides methods to synchronize multiple streams of images and motion samples based on the samples time-stamp or sample number. 
   - SDK datat path utility - The SDK provides a utility to locate SDK files in the system.
     The utility is used by CV modules, which need to locate data files in the system that are constant for all applications (not application- or algorithm-instance specific).
   - FPS counter -  


## Dependencies list

In order to successfully compile and use the SDK, you should install the following list of dependencies

 - [librealsense v1.11.0](https://github.com/IntelRealSense/librealsense/tree/v1.11.0)
 - opencv3.1
 - cmake
 - openGL GLFW version 3
 - liblz4-dev
 - Apache log4cxx – optional. Needed only if you want to enable logs.

 
## Learn More

For more information please refer to the [doc folder](https://github.com/IntelRealSense/realsense_sdk/tree/master/sdk/doc) 
For quick start of developing an application over the SDK refer to the **Intel® RealSense™ SDK Getting Started Guide**.
For detailed instruction on developing over the SDK refer to the **Intel® RealSense™ SDK Developer Guide**.
For detailed description of the SDK API functions refer to the **Intel® RealSense™ SDK Developer Reference**. 

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
