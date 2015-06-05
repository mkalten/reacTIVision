portVideo 0.6
-------------
https://github.com/mkalten/portVideo

PortVideo is a cross-platform framework that provides uniform access to camera devices for video processing or display. It comes with a simple SDL demo application which compiles and runs on Windows, Linux and MacOS X systems and supports many USB, Firewire and DV cameras. This library has been developed by Martin Kaltenbrunner [martin@tuio.org] as a part of the reacTIVision framework.

Although the API functionality is still quite rudimentary and its design is intentionally simple, it already provides efficient access to 8bit grayscale or 24bit RGB image data on various platforms and devices. In general there is a simple CameraEngine class and various subclasses for each platform and camera type. The CameraTool simply returns the chosen or first available camera, initializes it with the desired width, height and color depth. Once started, a new image buffer will be returned every time you call getFrame().

DEMO APPLICATION:
-----------------
There is a simple SportVideo demo application with a SDL2 interface included, which shows the basic usage of this framework. If you want to extend this application, simply implement and add a subclass of a FrameProcessor, just see the simple FrameInverter example source code for more information. The library itself can be used without the SDL interface dependency, an alternative ConsoleInterface allows headless operation.

Upon startup, SportVideo is looking for the first available camera on the system. Alternative cameras and image settings can be configured within the provided camera.xml file (which on a Mac is found inside the application bundle). If a camera is found and correctly initialized, the live image from the camera is  displayed. Hitting 'T' will show the processed image, which in this demo is a simple imversion of the original. If you want to use an alternative camera type 'U' and select the desired camera configuration using the cursor keys, hitting 'ENTER' will apply and save the new camera setting. Pressing 'O' will display a platform-dependent configuration dialog, where you can configure all available camera paramenters.  You can toggle the fullscreen mode by pressing the 'F1' key. Hitting 'P' can pause the display and hitting 'ESC' will quit the application. 

In Debug mode you also can save a bitmap image by typing 'B' as well as an image sequence by typing 'M' to start and stop recording. 
The images will be saved in a 'recording' folder, and can be later used by the FileCamera and FolderCamera driver.

PLATFORM NOTES:
---------------
Win32: This platform is using the videoInput library by Theodore Watson as a back-end to access the camera. This basically allows you to choose any WDM camera that is supported by Windows, which means you can most probably use any USB, Firewire or DV camera. A Visual Studio 2012 project (including SDL2 library and headers) provides the SportVideo demo application. 

Mac OS X: The camera back-end for Mac OS X is using the AVFoundation components, which should allow to access any camera that is supported by MacOS 10.8 or later. Firewire (and IIDC over USB) cameras are handled by the DC1394 library which provides better performance than the native system drivers. The non-conventional PS3Eye camera is supported through a dedicated driver module. The SportVideo demo is provided as a Xcode 3.2 (or later) project, the build will require the SDL2 and VVUVCKit frameworks, in order to compile properly, just unzip the included Frameworks.zip

Linux: On Linux we currently support Firewire (and IIDC over USB) cameras based on DC1394 as well as USB cameras and other devices based on Video4Linux2, currently there is no support for DV cameras. In order to compile the SportVideo demo type "make" in the "linux" folder" or alternatively use the provided CodeBlocks project. Make sure you have the libSDL 2.0 and libdc1394 2.0 (or later) as well as turbojpeg 1.3 (or later) libraries and headers installed. 

LICENSE:
--------
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.
 
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.
 
You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA