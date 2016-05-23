# reacTIVision 1.6

© 2005-2016 by Martin Kaltenbrunner <martin@tuio.org>
[https://github.com/mkalten/reacTIVision](https://github.com/mkalten/reacTIVision)

**reacTIVision** is an open source, cross-platform computer vision framework for the fast and robust tracking of fiducial markers attached onto physical objects, as well as for multi-touch finger tracking. It was mainly designed as a toolkit for the rapid development of table-based tangible user interfaces (TUI) and multi-touch interactive surfaces. This framework has been developed by Martin Kaltenbrunner and Ross Bencina as part of the [Reactable project](http://www.reactable.com/), a tangible modular synthesizer.


**reacTIVision** is a standalone application, which sends `Open Sound Control` (OSC) messages via a `UDP network socket` to any connected client application. It implements the [TUIO](http://www.tuio.org/) protocol, which has been specially designed for transmitting the state of tangible objects and multi-touch events from a tabletop surface. 

The [TUIO](http://www.tuio.org/) framework includes a set of example client projects for various programming languages, which serve as a base for the easy development of tangible user interface or multi-touch applications. Example projects are available for languages such as:
- C/C++
- C#
- Java
- [Processing](https://processing.org/)
- [Pure Data](https://puredata.info/)
- Max/MSP
- Flash

The **reacTIVision** application currently runs under the following operating systems: Windows, Mac OS X and Linux.

Under **Windows** it supports any camera with a proper `WDM driver`, such as most USB, FireWire and DV cameras. Under **Mac OS X** most UVC compliant USB as well as Firewire cameras should work. Under **Linux** FireWire cameras are as well supported as many `Video4Linux2` compliant USB cameras.

---
[1. Fiducial Symbols](#fiducial-symbols)
[2. Finger Tracking](#finger-tracking)
[3. Blob Tracking](#blob-tracking)
[4. Application Handling](#application-handling)
[5. XML configuration file](#xml-configuration-file)
[6. Calibration and Distortion](#calibration-and-distortion)
[7. Application Cheatsheet](#application-cheatsheet)
[8. Compilation](#compilation)


## Fiducial Symbols

This application was designed to track specially designed fiducial markers. You will find the default *amoeba* set of 216 fiducials in the document `./symbols/default.pdf`. Print this document and attach the labels to any object you want to track.

**reacTIVision** detects the ID, position and rotation angle of fiducial markers in the realtime video and transmits these values to the client application via the [TUIO](http://www.tuio.org/) protocol. [TUIO](http://www.tuio.org/) also assigns a session ID to each object in the scene and transmits this session ID along with the actual fiducial ID. This allows the identification and tracking of several objects with the same marker ID.

## Finger Tracking

**reacTIVision** also allows multi-touch finger tracking, which is basically interpreting any round white region of a given size as a finger that is touching the surface. Finger tracking is turned off by default and can be enabled by pressing the `F` key and adjusting the average finger size, which is given in pixels. The general recognition sensitivity can also be adjusted, where its value is given as a percentage. 75 would be less sensitive and 125 more sensitive, which means that also less probable regions are interpreted as a finger. The finger tracking should work with DI (diffuse illumination) as well as with FTIR illumination setups.

## Blob Tracking

Since version 1.6 **reacTIVision** also can track untagged objects, sending the overall footprint size and orientation via the [TUIO](http://www.tuio.org/) blob profile. 
You can activate this feature and configure the maximum blob size in `./reacTIVision.xml`, where you can also choose to send optional blob messages for fingers and fiducials, in order to receive information about their overall size. These blobs will share the same session ID with the respective finger or fiducial to allow the proper assignment.

## Application Handling

Before starting the **reacTIVision** application make sure you have a supported camera connected to your system, since the application obviously will not work at all without a camera. The application will show a single window with the B&W camera video in realtime.

Pressing the `S` key will show to the original camera image. Pressing `T` will show the binary tresholded image, pressing the `N` key will turn the display off, which reduces the CPU usage.

The thresholder gradient gate can be adjusted by hitting the`G` key. Lowering the value can improve the thresholder performance in low light conditions with insufficient finger contrast for example. You can gradually lower the value before noise appears in the image. Optionally you can also adjust the tile size of the thresholder.

The camera options can be adjusted by pressing the `O` key. On Windows and Mac OS this will show a system dialog that allows the adjustment of the available camera parameters. On Linux (Mac OS X when using Firewire cameras), the available camera settings can be adjusted with a simple on screen display. Pressing the `K` key allows
to browse and select all available cameras and image formats.

In order to produce some more verbose debugging output, hitting the `V` will print the symbol and finger data to the console. Pressing the `H` key will display all these options on the screen. `F1` will toggle the full screen mode, the `P` key pauses the image analysis completely, hitting `ESC` will quit the application.
	
## XML configuration file

**Common settings** can be edited within the file `./reacTIVision.xml` where all changes are stored automatically when closing the application.

Under Mac OS X this XML configuration file can be found within the application bundle's Resources folder. Select *Show Package Contents"* from the application's context menu in order to access and edit the file.

The **reacTIVision** application usually sends `TUIO/UDP` messages to port `3333` on `localhost (127.0.0.1)`. Alternative transport types are 
- `TUIO/TCP` (TCP)
- `TUIO/WEB` (Websocket)
- `TUIO/FLC` (Flash Local Connection).

You can change this setting by editing or adding the XML tag `<tuio type="udp" host="127.0.0.1" port="3333">` to the configuration. Multiple (up to 32) simultaneous TUIO transport options are allowed, the according type options are: udp, tcp, web and flc.

The `<fiducial amoeba="default" yamaarashi="false" />` XML tag lets you select the default or alternative (small,tiny) *amoeba* fiducial set, or you can point to an alternative `amoeba.trees` file if available. You can also additionally enable the new *Yamaarashi* fiducal symbols.

The **display attribute** defines the default screen upon startup. The `<image display="dest" equalize="false" gradient="32" tile="10"/>` lets you adjust the default gradient gate value and tile size. **reacTIVision** comes with an image equalization module,  which in some cases can increase the recognition performance of both the finger and fiducial tracking. Within the running application you can toggle this with the `E` key or reset the equalizer by hitting the `SPACE` bar.

The overall **camera and image settings** can be configured within the `./camera.xml` configuration file. On Mac OS X this file is located in the Resources folder within the application bundle. You can select the camera ID and specify its dimension and framerate, as well as the most relevant image adjustments. Optionally you can also crop the raw camera frames to reduce the final image size.

*Please see the example options in the file for further information.*

You can list **all available cameras** with the `-l` startup option.

## Calibration and Distortion

Many tables, such as the **reacTable** are using wide-angle lenses to increase the area visible to the camera at a minimal distance. Since these lenses unfortunately distort the image, **reacTIVision** can correct this distortion as well as the alignment of the image.

For the **calibration**, print and place the provided calibration sheet on the table and adjust the grid points to the grid on the sheet. To calibrate **reacTIVision** switch to calibration mode hitting `C`.

Use the keys `A`,`D`,`W`,`X` to move within grid, moving with the cursor keys will adjust the position (center point) and size (lateral points) of the grid alignment. By pressing `Q` you can toggle into the precise calibration mode, which allows you to adjust each individual grid point.

`J` resets the whole calibration grid, `U` resets the selected point and `L` reverts to the saved grid.

To check if the distortion is working properly press `R`. This will show the fully distorted live video image in the target window. Of course the distortion algorithm only corrects the found fiducial positions instead of the full image.

##### Application Cheatsheet

| Key 		| Function 										|
| ---------	| --------------------------------------------- |
| `G` 		| Threshold gradient & tile size configuration	|
| `F`		| Finger size and sensitivity configuration		|
| `B`		| Blob size configuration & enable /tuio/2Dblb	|
| `Y` 		| enable/disable Yamaarashi symbol detection	|
| `I` 		| configure TUIO attribute inversion			|
| `E`		| Turns image equalization on/off				|
| `SPACE` 	| Resets image equalization 					|
|			|												|
| `O` 		| Camera configuration							|
| `K` 		| Camera selection								|
|			|												|
| `C` 		| Enter/Exit Calibration mode					|
| `Q` 		| Toggle quick/precise calibration mode 		|
| `U` 		| Resets selected calibration grid point		|
| `J` 		| Resets all calibration grid points			|
| `L` 		| Reverts calibration to saved grid				|
|			|												|
| `S`		| Shows original camera image 					|
| `T` 		| Shows binary thresholded image				|
| `R` 		| Shows calibrated camera image					|
| `N` 		| Turns display off	(saves CPU)					|
| `F1`		| Toggles full screen mode 						|
|			|												|
| `P` 		| Pauses all processing							|
| `V` 		| Verbose output to console						|
| `H` 		| Shows help options							|
| `ESC` 	| Quits application 							|

## Compilation

The source distribution includes projects for all three supported platforms and their respective build systems: Linux, Windows, MacOS X.

##### Windows:
A *Visual Studio 2012* project as well as the necessary `SDL2` libraries and headers are included. The project should build right away for 32bit and 64bit targets without any additional configuration.

##### Mac OS X:
An *Xcode* project for *Xcode* version 3.2 or later is included. The build will require the `SDL2` and `VVUVCKit` frameworks, in order to compile properly, just unzip the provided `./ext/portvideo/macosx/Frameworks.zip` into the directory `./ext/portvideo/macosx/`

##### Linux:
Call make to build the application, the distribution also includes configurations for the creation of RPM packages, as well as a project file for the *Codeblocks IDE*. Make sure you have the `libSDL-2.0` and `libdc1394-2.0` (or later) as well as `libjpeg-turbo` libraries and headers installed. 

## License

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA