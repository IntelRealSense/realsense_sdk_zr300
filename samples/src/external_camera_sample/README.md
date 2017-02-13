# Intel&reg; RealSense&trade; Linux SDK
## External Camera Sample
---
### Description
    This sample shows how to use video4linux along with the RealSense(TM) SDK and librealsense.
    The sample uses v4l2 lib to capture color frames from an external camera (webcam).
    Color frames provided via v4l2 and depth frames provided via librealsense are grouped together into a correlated sample set using a samples_time_sync_interface for external devices.
    The correlated sample set can be passed to computer vision (CV) modules to be processed together, however this sample does not use any specific CV module and instead, simply calls process_samples(...) to demonstrate the flow up to this point
### Category
    RealSense(TM) SDK

### Author
    Intel(R) Corporation

### Hardware Requirements
    zr300, web-cam

### Libraries


### Compiler Flags
    -std=c++11

### Libraries Flags
    -lrrealsense -lrrealsense_v4l_image -lrrealsense_samples_time_sync -lrrealsense_lrs_image -lrpthread

### Date
    22/01/2017
