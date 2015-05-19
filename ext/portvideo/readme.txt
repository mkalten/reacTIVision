PortVideo is a cross-platform framework that provides uniform access to camera devices for video processing or display. It comes with a simple SDL demo application which compiles and runs on Windows, Linux and MacOS X systems and supports many USB, Firewire and DV cameras. This library has been developed by Martin Kaltenbrunner [martin@tuio.org] as a part of the reacTIVision framework.

The API is still in its early stages and basically has been built out of spare parts found in other open-source projects and currently provides a reasonably working software for the task. Its design is quite rudimentary and will therefore see some major improvements and changes in the future.

In general there is a simple wrapper class CameraEngine and various subclasses for each platform and camera type. The CameraTool simply returns the chosen or first available camera, initializes it with the desired width, height and color depth. Once started, a new image buffer (8bit grayscale or 24bit RGB in color mode) will be returned every time you call getFrame().

DEMO APPLICATION:
-----------------
There is a simple SDL2 demo application included, which shows the basic usage of this framework. If you want to start right away, simply implement and add a subclass of FrameProcessor. See the FrameInverter example source code for more information.

Upon startup, the application is looking for the first available camera on the system. If a camera is found and correctly initialized, the live image from the camera is  displayed. Hitting 'T' will show the processed image, which in this demo is a simple imversion of the original. You can toggle the fullscreen mode by pressing the 'F1‘ key. Hitting 'P' can pause the display and hitting 'ESC' will quit the application. In Debug mode you also can save a bitmap image by typing 'B' as well as a raw buffer image by typing 'R'. Alternative cameras and specific settings can be configured within the provided camera.xml file (which on a Mac is found inside the application bundle).

PLATFORM NOTES:
---------------
Win32: On this platform we are using the videoInput library by Theodore Watson as a back-end to access the camera. This basically allows you to choose any WDM camera which is installed correctly, which means you can hopefully use any USB, Firewire or DV camera. The necessary SDL2 library as well as a Visual Studio 2012 project is provided, so you should be able to compile the project right away. 

Mac OS X: The camera back-end for Mac OS X is using the AVFoundation components, which should allow to access any camera that is supported by MacOS 10.8 or later. Firewire cameras are handled by the dc1394 library which provides improved configuration options and much better performance than QuickTime. The non-conventional PS3Eye camera is supported through a dedicated driver module.
There is an Xcode 3.2 project included, which should also allow you to compile the code without problems. The build will require the SDL2 and VVUVCKit frameworks, in order to compile properly, just unzip the included Frameworks.zip

Linux: On Linux we currently support Firewire cameras and USB cameras based on Video4Linux2. There is currently no support for DV cameras. In order to compile the demo application type "make" in the "linux" folder". Make sure you have the libSDL 2.0 and libdc1394 2.0 (or later) 
as well as turbojpeg libraries and headers installed. 

LICENSE:
--------
This framework is free software, licensed under the GNU Lesser General Public License, please read the file license.txt for the exact license terms of the LGPL.