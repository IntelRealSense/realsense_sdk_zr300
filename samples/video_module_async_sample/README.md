# Intel&reg; RealSense&trade; Linux SDK
## Video Module Asynchronized Sample
---
### Description
This sample demonstrates an application usage of a computer vision module, which implements asynchronous samples processing.  The video module implements the video module interface, which is a common way for the application or SDK to interact with the module.  It also implements a module specific interface, in this example - the module calculates the maximal depth value in the image.

### Category
    RealSense(TM) SDK

### Author
    Intel(R) Corporation
    
### Hardware Requirements
    zr300

### Libraries
    

### Compiler Flags
    -std=c++11

### Libraries Flags
    -lrealsense -lrs_max_depth_value_module -lrs_image -lrs_playback -lrs_log_utils -lrs_projection -lpthread -lopencv_imgproc -lopencv_core

### Date
    22/08/2016
    