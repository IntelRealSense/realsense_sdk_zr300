Projection Tool
===================

**Projection Tool** provides visual representation of Projection Library of Intel® Linux RealSense™ SDK. The tool uses *both* command line and GUI to interact with user.

`Live stream`, `playback` modes are available. Using the tool, draw points on images or press certain keys to see `Projection API` visually.

User Interface
-------------------
 - **Static window** includes four elements:
	* Projection Tool **help message** describes usage and user input
	* **Color window** shows `color image` (each pixel is RGB or RGBA) from camera
	* **Depth window** shows `depth image` (each pixel is Depth value) from camera
	* **World window** shows `world image` (each pixel is Z point coordinate from 3D world image) calculated by Projection Library
 - **Mapped points coordinates** that depend on a **depth interval** show **all mapped points** for the specified depth range. **Maximal** depth interval: `[0;10]` in meters with a step of `0.5` meters:
	* **Color** points coordinates calculated from **UVMap** is a `color points vector` that contain color points corresponding to a certain depth range (e.g. from 0 to 1 meters)
	* **Depth** points coordinates calculated from **Inversed UVMap** is a `depth points vector` that contain depth points corresponding to a certain depth range interval (e.g. from 0 to 1 meters)
 - **Dynamic windows** that one can show and hide:
	* **Color Image Mapped To Depth window** shows `color image mapped to depth` created with Projection Library
	* **Depth Image Mapped To Color window** shows `depth image mapped to color` created with Projection Library
 - **User Input** specifies the interaction between user and the tool


Command Line
-------------------
**Projection Tool** provides command line parsing mechanism to enable passing of command line arguments:

 1. **Help** option provides description of command line parameters
 2. **Playback** option specifies the playback file, thus enabling playback mode instead of live stream mode
 3. **Depth Configuration** option specifies depth live stream resolution in format: *WIDTHxHEIGHTxFPS*. Default resolution is `628x468x30`
 4. **Color Configuration** option specifies color live stream resolution in format: *WIDTHxHEIGHTxFPS*. Default resolution is `640x480x30`
 5. **Depth Format** option specifies depth pixel format. Default pixel format is `z16`
 6. **Color Format** option specifies color pixel format. Default pixel format is `rgb8`

To see detailed description of command line parameters run the tool with `-h` or `--help` option

Notes
-------------------
Projection Tool **supports** `bgra8 bgr8 rgba8 rgb8` formats of `color stream` and `z16` format of `depth stream` provided by [librealsense](https://github.com/IntelRealSense/librealsense).
The resolutions in live stream mode are by default `640x480 pixels with 30 frames per second` for color stream and `628x468 pixels with 30 frames per second` for depth stream.

Please, see Projection Library documentation section as a reference for the `Projection API` presented in Projection Tool.
