portVideo 0.5
————————————-
https://github.com/mkalten/portVideo

PortVideo is a cross-platform framework that provides uniform access to camera devices for video processing or display. It comes with a simple SDL demo application which compiles and runs on Windows, Linux and MacOS X systems and supports many USB, Firewire and DV cameras. This library has been developed by Martin Kaltenbrunner [martin@tuio.org] as a part of the reacTIVision framework.

Although the API functionality is still quite rudimentary and its design is intentionally simple, it already provides efficient access to 8bit grayscale or 24bit RGB image data on various platforms and devices. In general there is a simple CameraEngine class and various subclasses for each platform and camera type. The CameraTool simply returns the chosen or first available camera, initializes it with the desired width, height and color depth. Once started, a new image buffer will be returned every time you call getFrame().

DEMO APPLICATION:
-----------------
There is a simple SportVideo demo application with a SDL2 interface included, which shows the basic usage of this framework. If you want to extend this application, simply implement and add a subclass of a FrameProcessor, just see the simple FrameInverter example source code for more information.

Upon startup, SportVideo is looking for the first available camera on the system. Alternative cameras and image settings can be configured within the provided camera.xml file (which on a Mac is found inside the application bundle). If a camera is found and correctly initialized, the live image from the camera is  displayed. Hitting 'T' will show the processed image, which in this demo is a simple imversion of the original. You can toggle the fullscreen mode by pressing the 'F1' key. Hitting 'P' can pause the display and hitting 'ESC' will quit the application. In Debug mode you also can save a bitmap image by typing 'B' as well as a raw buffer image by typing 'R'. 

PLATFORM NOTES:
---------------
Win32: On this platform we are using the videoInput library by Theodore Watson as a back-end to access the camera. This basically allows you to choose any WDM camera that is supported by Windows, which means you can hopefully use any USB, Firewire or DV camera.  A Visual Studio 2012 project including the necessary SDL2 library and headers provides the SportVideo demo project. 

Mac OS X: The camera back-end for Mac OS X is using the AVFoundation components, which should allow to access any camera that is supported by MacOS 10.8 or later. Firewire (and IIDC over USB) cameras are handled by the DC1394 library which provides better performance than the native system drivers. The non-conventional PS3Eye camera is supported through a dedicated driver module.
The SportVideo demo is provided as a Xcode 3.2 (or later) project, the build will require the SDL2 and VVUVCKit frameworks, in order to compile properly, just unzip the included Frameworks.zip

Linux: On Linux we currently support Firewire (and IIDC over USB) cameras based on DC1394 as well as USB cameras and other devices based on Video4Linux2, currently ther is no support for DV cameras. In order to compile the SportVideo demo type "make" in the "linux" folder". Make sure you have the libSDL 2.0 and libdc1394 2.0 (or later)  as well as turbojpeg libraries and headers installed. 

LICENSE:
--------
This framework is free software, licensed under the GNU Lesser General Public License, please read the file license.txt for the exact license terms of the LGPL.