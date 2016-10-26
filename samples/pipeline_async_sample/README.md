# Intel&reg; RealSense&trade; Linux SDK
## Pipeline Asynchronized Sample
---
### Description
This sample demonstrates an application usage of an asynchronized pipeline. The pipeline simplifies the user interaction with computer vision modules. It abstracts the camera configuration and streaming, the video modules triggering and threading, and lets the application focus on the computer vision output of the modules. The pipeline can manage computer vision modules, which implement the video module interface. The pipeline is the consumer of the video module interface, while the application consumes the module specific interface, which completes the video module interface. The async pipeline provides the user main loop, which runs on the calling thread, and computer vision modules callbacks, which are triggered on different threads. In this sample an example computer vision module, the max depth value module is used to demonstrate the pipeline usage.
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
    -lrealsense_pipeline -lrealsense_max_depth_value_module -lrealsense_image -lrealsense_playback -lrealsense_projection -lrealsense_samples_time_sync -lrealsense_log_utils -lrealsense -lpthread -lopencv_imgproc -lopencv_core

### Date
    25/10/2016
    
