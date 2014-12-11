reacTIVision is a standalone application, which sends Open Sound
Control (OSC) messages via a UDP network socket to any connected
client application. It implements the TUIO protocol, which has been
specially designed for transmitting the state of tangible objects
and multi-touch events from a tabletop surface. As an alternative 
to TUIO, the application is also capabable of sending MIDI messages.
http://www.tuio.org/

Alternatively it can also send MIDI messages, which are configurable
via an XML configuration file. Keep in mind though that MIDI has less
bandwidth and data resolution compared to Open Sound Contol, so the
MIDI feature is meant as an convenient alternative in some cases,
but TUIO still will be the primary messaging layer.

The alternative MIDI mode can map any object dimension (xpos, ypos, angle)
to a MIDI control via an XML configuration file. Adding and removing objects
can be mapped to simple note ON/OFF events. 

Adding <midi config="midi.xml" /> to reacTIVision.xml switches to MIDI mode
and specifies the MIDI configuration file that contains the mappings and MIDI
device selection. An example PD patch "midi_demo.pd" can be found this folder. 

You can list all available MIDI devices with the "-l" option.

The demo configuration file "midi.xml" defines a simple controller
setup with a few vertical and horizontal faders, some knobs and
a free controller area for a single object. See the "midi_demo.pd" patch
for Pure Data for an example MIDI client using this setup.

