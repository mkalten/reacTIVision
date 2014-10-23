reacTIVision 1.5
----------------
(c) 2005-2014 by Martin Kaltenbrunner <martin@tuio.org>
https://github.com/mkalten/reacTIVision

reacTIVision is an open source, cross-platform computer vision
framework for the fast and robust tracking of fiducial markers
attached onto physical objects, as well as for multi-touch
finger tracking. It was mainly designed as a toolkit for the 
rapid development of table-based tangible user interfaces (TUI)
and multi-touch interactive surfaces. This framework has been 
developed by Martin Kaltenbrunner and Ross Bencina as part of
the Reactable project, a tangible modular synthesizer.
http://www.reactable.com/

reacTIVision is a standalone application, which sends Open Sound
Control (OSC) messages via a UDP network socket to any connected
client application. It implements the TUIO protocol, which has been
specially designed for transmitting the state of tangible objects
and multi-touch events from a tabletop surface. As an alternative 
to TUIO, the application is also capabable of sending MIDI messages.
http://www.tuio.org/

The TUIO framework includes a set of example client projects
for various programming languages, which serve as a base for
the easy development of tangible user interface or multi-touch
applications. Example projects are available for languages such as:
C/C++, Java, C#, Processing, Pure Data, Max/MSP and Flash. 

The reacTIVision application currently runs under the 
following operating systems: Windows, Mac OS X and Linux
Under Windows it supports any camera with a proper WDM driver,
such as most USB, FireWire and DV cameras. Under Mac OS X most
UVC compliant USB as well as Firewire cameras should work.
Under Linux FireWire cameras are as well supported as many 
Video4Linux2 compliant USB cameras.

Fiducial Symbols
----------------
This application was designed to track specially designed fiducial
markers. You will find the default "amoeba" set of 216 fiducials in
the document "default.pdf" within the symbols folder. Print this
document and attach the labels to any object you want to track.
The default fiducial tracking engine is based on the included fidtrack
library, which also provides an alternative "classic“ fiducial set,
which are a reimplementation of Enrico Costanza’s d-touch concept.
See below how to configure the application to use these symbol sets.

reacTIVision detects the ID, position and rotation angle of fiducial 
markers in the realtime video and transmits these values to the client
application via the TUIO protocol. TUIO also assigns a session ID to 
each object in the scene and transmits this session ID along with the
actual fiducial ID. This allows the identification and tracking of 
several objects with the same marker ID.

Finger Tracking
---------------
reacTIVision also allows multi-touch finger tracking, which is basically
interpreting any round white region of a given size as a finger that
is touching the surface. Finger tracking is turned off by default and 
can be enabled by pressing the 'F' key and adjusting the average finger 
size, which is given in pixels. The general recognition sensitivity can 
also be adjusted, where its value is given as a percentage. 75 would be 
less sensitive and 125 more sensitive, which means that also less probable 
regions are interpreted as a finger. The finger tracking should work with 
DI (diffuse illumination) as well as with FTIR illumination setups.

Application Handling
--------------------
Before starting the reacTIVision application make sure you have
a supported camera connected to your system, since the application
obviously will not work at all without a camera. The application
will show a single window with the B&W camera video in realtime.

Pressing the 'S' key will show to the original camera image.
Pressing 'T' will show the binary tresholded image, pressing the
'N' key will turn the display off, which reduces the CPU usage.

The thresholder gradient gate can be adjusted by hitting the 'G' key.
Lowering the value can improve the thresholder performance in low
light conditions with insufficient finger contrast for example.
You can gradually lower the value before noise appears in the image.
Optionally you can also adjust the tile size of the thresholder.

The camera options can be adjusted by pressing the 'O' key. 
On Windows and Mac OS this will show a system dialog that allows 
the adjustment of the available camera parameters. On Linux (Mac OS X
when using Firewire cameras), the available camera settings can be
adjusted with a simple on screen display. 

In order to produce some more verbose debugging output, hitting
the 'V' will print the symbol and finger data to the console.
Pressing the 'H' key will display all these options on the screen.
'F1' will toggle the full screen mode, the 'P' key pauses the
image analysis completely, hitting 'ESC' will quit the application. 

XML configuration file
----------------------

Common settings can be edited within the file "reacTIVision.xml" where 
all changes are stored automatically when closing the application.
Under Mac OS X this XML configuration file can be found within the 
application bundle's Resources folder. Select "Show Package Contents" 
from the application's context menu in order to access and edit the file. 

The reacTIVision application usually sends the TUIO messages 
to port 3333 on localhost (127.0.0.1)
You can change this setting by adding or editing the XML tag
<tuio host="127.0.0.1" port="3333"> to the configuration.

The <fiducial engine="amoeba" tree="default"/> XML tag lets you
select the fiducial engine or an alternative amoeba tree order.
The default engine is using the fastest and effective 'amoeba' 
fiducial set. Add the 'classic' option in order to use the dtouch
reimplementation from libfidtrack.

The display attribute defines the default screen upon startup.
The <image display="dest" equalize="false" gradient="32" tile="10"/>
lets you adjust the default gradient gate value and tile size.
reacTIVision comes with a background subtraction module,  which in
some cases can increase the recognition performance of both the finger
and fiducial tracking. Within the running application you can toggle
this with the 'E' key or recalculate the background subtraction 
by hitting the SPACE bar.

The overall camera and imagee settings can be configured within the
"camera.xml" configuration file. On Mac OS X this file is as well 
located in the Resources folder within the application bundle.
You can select the camera ID and specify its dimension and framerate,
as well as the most relevant image adjustments. Optionally you can also
crop the raw camera frames to reduce the final image size. 
Please see the example options in the file for further information. 

You can list all available cameras with the "-l" startup option.

TUIO vs. MIDI
-------------
The application can alternatively send  MIDI messages, which allows
the mapping of any object dimension (xpos, ypos, angle) to a MIDI
control message through an XML configuration file. Adding and removing
objects can also be mapped to simple note ON/OFF events.
Keep in mind though that MIDI has less bandwidth and data resolution
compared to Open Sound Control, so the MIDI feature is only meant
as a convenience alternative to TUIO in some application scenarios.

Adding <midi config="midi.xml" /> to reacTIVision.xml switches to
MIDI mode and specifies the MIDI configuration file that contains the
mappings and MIDI device selection. An example configuration file along
with a simple PD patch example can be found in the midi folder.

You can list all available MIDI devices with the "-l" startup option.

Calibration and Distortion
--------------------------
Many tables, such as the reacTable are using wide-angle lenses
to increase the area visible to the camera at a minimal distance. 
Since these lenses unfortunately distort the image, reacTIVision
can correct this distortion as well as the alignment of the image.

For the calibration, print and place the provided calibration sheet
on the table and adjust the grid points to the grid on the sheet.
To calibrate reacTIVision switch to calibration mode hitting 'C'

Use the keys A,D,W,X to move within grid, moving with the cursor keys
will adjust the position (center point) and size (lateral points) 
of the grid alignment. By pressing 'Q' You can toggle into the precise 
calibration mode, which allows you to adjust each individual grid point.

'J' resets the whole calibration grid, 'U' resets the selected point
and 'K' reverts to the saved grid.

To check if the distortion is working properly press 'R'. This will show
the fully distorted live video image in the target window. Of course the
distortion algorithm only corrects the found fiducial positions instead 
of the full image.

Compilation
-----------
The source distribution includes projects for all three supported
platforms and their respective build systems: Linux, Win32, MacOS X.

Win32:
A Visual Studio 2012 project as well as all the necessary libraries
and headers (SDL2, videoInput) are included. The project should build
right away without any additional configuration.

Mac OS X:
An Xcode project for Xcode version 3.2 or later is included.
The build will require an installed SDL2 framework in order to
compile properly.

Linux:
Call make to build the application, the distribution also
includes configurations for the creation of RPM packages, as well
as a project file for the Codeblocks IDE. 
Make sure you have the libSDL 2.0 and libdc1394 2.0 (or later) 
as well as turbojpeg libraries and headers installed. 

License
-------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
