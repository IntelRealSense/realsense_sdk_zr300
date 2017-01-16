# Intel® RealSense™ SDK for Linux



[![Release] [release-image] ] [releases]
[![License] [license-image] ] [license]

[release-image]: http://img.shields.io/badge/release-0.6.1-blue.svg?style=flat
[releases]: https://github.com/IntelRealSense/realsense_sdk/tree/v0.6.1

[license-image]: http://img.shields.io/badge/license-Apache--2-blue.svg?style=flat
[license]: LICENSE


The Intel® RealSense™ SDK for Linux provides libraries, tools, and samples to develop applications using Intel® RealSense™ cameras, over the Intel librealsense API. 

The SDK provides functionality of record and playback of camera streams for test and validation. 

The SDK includes libraries which support the camera stream projection of streams into a common world-space viewpoint, and libraries which enable the use of multiple middleware modules simultaneously for common multi-modal scenarios.  

# Table of Contents 
* [Compatible Devices](#compatible-devices)
* [Supported Platforms](#compatible-platforms)
* [Supported Languages and Frameworks](#supported-languages-and-frameworks)
* [Functionality](#functionality)
* [Dependencies List](#dependencies-list)
* [Learn More](#learn-more)

## Compatible Devices

Intel® RealSense™ ZR300


## Compatible Platforms

The library is written in standards-conforming C++11. It is developed and tested on Ubuntu 16.04 x64 (GCC 5.4 toolchain)


## Supported Languages and Frameworks

C++ 

## Functionality

**Note**: This API is experimental and is not an official Intel product. 
It is subject to incompatible API changes in future updates. Breaking API changes are noted through release numbers.

1. **Record and Play**    
    - **Record**: The record module provides a utility to create a file, which can be used by the playback module to create a video source.
    The record module provides the same camera API as defined by the SDK (librealsense) and the record API to configure recording parameters such as output file and state (pause and resume).
    The record module loads librealsense to access the camera device and execute the set requests and reads, while writing the configuration and changes to the file.
    - **Playback**: The playback module provides a utility to create a video source from a file. 
    The playback module provides the same camera API as defined by the SDK (librealsense), and the playback API to configure recording parameters such as input file, playback mode, seek, and playback state (pause and resume).
    The playback module supports files that were recorded using the Linux SDK recorder and the Windows RSSDK recorder (up to version 2016 R2).
    
2. **Frame data container**
    The SDK provides an image container for raw image access and basic image processing services, 
    such as format conversion, mirror, rotation, and more. It caches the processing output to optimize multiple requests of the same operation.
    The image container includes image metadata, which may be used by any pipeline component to attach additional data or computer vision (CV) module processing output to be used by other pipeline components. The SDK uses a correlated samples container to provide access to camera images and motion sensor samples from the relevant streams, which are time-synchronized. The correlated samples container includes all relevant raw buffers, metadata, and information required to access the attached images. 
    
3. **Spatial correlation and projection**
    The Spatial Correlation and Projection library provides utilities for spatial mapping:
    - Map between color or depth image pixel coordinates and real world coordinates
    - Correlate depth and color images and align them in space
	
4. **Pipeline**
    The pipeline is a class, which abstracts the details of how the cognitive data is produced by the computer vision modules.
    The application can focus on consuming the computer vision output, leaving the camera configuration and streaming details for the pipeline to handle.

5. **Samples**
    - **Projection**: The sample demonstrates how to use the different spatial correlation and projection functions, from live camera and recorded file
    - **Record and Playback**: The sample demonstrates how to record and play back a file while the application is streaming, with and without an active CV module,      with minimal changes to the application, compared to live streaming.
    - **Video module, asynchronized**: The sample demonstrates an application usage of a Computer Vision module, which implements asynchronous sample processing. 
    - **Video module, synchronized**: The sample demonstrates an application usage of a Computer Vision module, which implements synchronous samples processing.
    - **Fatal error recovery**: The sample demonstrates how the application can recover from a fatal error in one of the SDK components (CV module or core module), without having to terminate.
   
6. **Tools**
   - **Capture tool**: Provides a GUI to view camera streams, create a new file from a live camera, and play a file in the supported formats. The tool provides options to render the camera or file images.
   - **Projection tool**: Provides simple visualization of the projection functions output, to allow human eye detection of major offsets in the projection computation.
   - **System Info tool**: Presents system data such as Linux name, Linux kernel version, CPU information, and more.
   
7. **Utilities**
   - **Log**: The SDK provides a logging library, which can be used by the SDK components and the application to log meaningful events. 
   - **Time Sync utility**: Provides methods to synchronize multiple streams of images and motion samples based on the samples time-stamp or sample number. 
   - **SDK Data Path utility**: The SDK provides a utility to locate SDK files in the system.
     The utility is used by Computer Vision  modules, which need to locate data files in the system that are constant for all applications (not application- or algorithm-instance specific).
   - **FPS counter**:  Measures the actual FPS in a specific period. You can use this utility to check the actual FPS in all software stack layers in your applications, to analyze FPS latency in those layers.


## Dependencies list

To successfully compile and use the SDK, you should install the following dependencies:

 - [Intel librealsense v1.12.1](https://github.com/IntelRealSense/librealsense/tree/v1.12.1)
 - OpenCV 3.1
 - CMake
 - OpenGL GLFW version 3
 - [liblz4-dev](https://github.com/lz4/lz4)
 - Apache log4cxx – optional. Needed only if you want to enable logs.

 
## Learn More

For more information, please refer to the [doc folder](https://github.com/IntelRealSense/realsense_sdk/tree/master/sdk/doc). 

For a Quick Start guide to developing an application over the SDK, refer to the **Intel® RealSense™ SDK for Linux Getting Started**.

For detailed instructions on developing over the SDK, refer to the **Intel® RealSense™ SDK for Linux Developer's Guide**.

For a detailed description of the SDK API methods, refer to the **Intel® RealSense™ SDK for Linux Developer's Reference**. 


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
