SWIG Python wrapper
===================

The wrapper contains basic functionality for **streaming** (from camera/from pre-recorded file) and **projection**. After successful build you will have **3 modules**, each module contains `<module_name>.so` and `<module_name>.py` files 

Modules: 

1. **Streaming** – streaming from device, only synchronous streaming was implemented
2. **Playback** – playback from file
3. **Projection** – projection between color and depths images 

Requirements
-------------------

You should have all things which are necessary for SDK building plus:

1. **Swig** dev package
2. **Python** dev package. *Note*: If you want to run **samples** you should also install `python-imaging module`
> sudo apt-get install swig python-dev  python-imaging

How to build and install
-------------------

To build call cmake with `-DBUILD_PYTHON_INTERFACE=ON` option. Example:
> cmake -DBUILD\_PYTHON\_INTERFACE=ON /path/to/sdk

To use built modules **copy** `*.so` and `*.py` files from **python/lib/release/** folder  to your *python package directory*

Samples description
-------------------

To run samples **without copying** to *python package directory*: **copy** samples from  **python/samples** directory to **python/lib/** directory

Samples demonstrate simple scenarios of using Intel(R) RealSense(TM) Cameras via python.  They work with `rgb8 color format` and `z16 depth format`.

To get description of input parameters: call `python <sample_name> –help`

Development Tips
-------------------

You are able to call all functions like in C++, but **names should be different**: 

Typical **rule** for name: `<module_name>.<interface_name_function_name>([parameters])`. If you are in doubt, open `python/lib/release/<module_name>.py` and find the function name you need. 

**The most complicated thing** in swig python wrapper is to work with pointers:

 - `streaming.cdata(array_variable, size_in_butes)` – return python `string` with data 
 - `projection.new_pointF32Array(size)` – create array of `pointF32` with `size` elements
 - `projection.pointF32Array_getitem(array_variable, element_number)` – return element of `pointF32` array
 - `projection.new_ point3dF32Array(size)` – create array of `point3dF32` with `size` elements 
 - `projection.point3dF32Array_getitem(array_variable, element_number)` – return element of `point3dF32` array
 - `projection.new_ucharArray(size)` – create array of `unsigned char` with `size` elements 
 - `projection.ucharArray_getitem(array_variable, element_number)` – return element of `unsigned char` array
 - `projection.void_to_char(frame_data_color)` – convert `void*` array to `unsigned char` array

For more information see: [c arrays](http://www.swig.org/Doc1.3/Library.html#Library_carrays) and [c pointers](http://www.swig.org/Doc1.3/Library.html#Library_nn4)