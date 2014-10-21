reacTIVision is a standalone application, which sends OpenSound
Control messages via a UDP network socket to any connected
client application. It implements the TUIO protocol, which was
specially designed for transmitting the state of tangible objects
on a table surface. Alternatively it can also send MIDI messages,
which are fully configurable via an XML configuration file.

The alternative MIDI mode can map any object dimension (xpos, ypos, angle)
to a MIDI control via an XML configuration file. Adding and removing objects
can be mapped to simple note ON/OFF events. Keep in mind though that MIDI 
has less bandwidth and data resolution compared to Open Sound Contol, 
so the new MIDI feature is meant as an convenient alternative in some cases,
but TUIO still will be the primary messaging layer.

Adding <midi config="midi/demo.xml" /> to reacTIVision.xml switches to
MIDI mode and specifies the MIDI configuration file that contains the mappings
and MIDI device selection. An example configuration file "demo.xml" along with an 
example PD patch "demo.pd" can be found in the "midi" folder. 
You can list all available MIDI devices with the "-l" option.

The demo configuration file "demo.xml" defines a simple controller
setup with a few vertical and horizontal faders, some knobs and
a free controller area for a single object. See the "demo.pd" patch
for Pure Data for an example MIDI client using this setup.

