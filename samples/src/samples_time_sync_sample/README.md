# Intel&reg; RealSense&trade; Linux SDK
## Time Sync Sample
---
### Description
This sample demonstrates an application usage of a samples time sync utility.  The samples time sync utility syncronizes the frames from different streams. The streams need to be registered with the samples time sync module,
prior to first usage. For zr300 cmarea, the sync between all color/depth/ir streams is calculates based on equity of timestamps. For fisheye stream and IMU, best matched is chosen.



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
    -lrs_samples_time_sync -lrs_image -lrealsense -lrs_log_utils -lopencv_imgproc -lopencv_core

### Date
    22/08/2016
    
