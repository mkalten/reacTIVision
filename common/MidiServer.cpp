/*  reacTIVision tangible interaction framework
    MidiServer.cpp
    Copyright (C) 2005-2014 Martin Kaltenbrunner <martin@tuio.org>

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
*/

#include "MidiServer.h"

MidiServer::MidiServer(const char* cfg)
{
	device=-1;
	//read config from file
	readConfig(cfg);
	if (device<0) return;

	int no_devices = Pm_CountDevices();
	if (no_devices==0) { std::cout << " no midi devices found!" << std::endl; return; }
	if (device>no_devices) {
		std::cout << "invalid midi device number: " << device << std::endl; 
		device = -1;
		return; 
	}
	
	PmDeviceID device_id = Pm_GetDefaultOutputDeviceID();
	if ((device>=0) && (device<=no_devices)) {
		device_id = (PmDeviceID)device;
		const PmDeviceInfo *info = Pm_GetDeviceInfo( device_id );
		if (info->input) { 
			std::cout << "invalid midi device number: " << device << std::endl; 
			device = -1;
			return; 
		}
	}

	const PmDeviceInfo *info = Pm_GetDeviceInfo(device_id);
	std::cout << "opening midi device: " << info->name << std::endl;
	
	Pm_Initialize();	
	Pt_Start(1,0,0);
	Pm_OpenOutput(&midi_out,device_id,NULL,0,NULL,NULL,0);
	running=true;
}

void MidiServer::listDevices()
{
	int no_devices = Pm_CountDevices();	
	int out_devices = 0;
	for (int i=0;i<no_devices;i++) {
		const PmDeviceInfo *info = Pm_GetDeviceInfo( (PmDeviceID)i );
		if (info->output) out_devices++;
	}

	if (out_devices==0) {
		std::cout << "no MIDI out devices found!" << std::endl;
		return;
	} else if (out_devices==1) std::cout << "1 MIDI out device found:" << std::endl;
	else std::cout << out_devices << " MIDI out devices found:" << std::endl;
		
	for (int i=0;i<no_devices;i++) {
		const PmDeviceInfo *info = Pm_GetDeviceInfo( (PmDeviceID)i );
		if (info->output) std::cout << "\t" << i << ": " << info->name << std::endl;
	}
}



void MidiServer::readConfig(const char* cfg) {

#ifdef __APPLE__
	char cfg_path[1024];
	if (cfg[0]=='/') sprintf(cfg_path,"%s",cfg);
	else {	
		char app_path[1024];
		CFBundleRef mainBundle = CFBundleGetMainBundle();
		CFURLRef mainBundleURL = CFBundleCopyBundleURL( mainBundle);
		CFStringRef cfStringRef = CFURLCopyFileSystemPath( mainBundleURL, kCFURLPOSIXPathStyle);
		CFStringGetCString( cfStringRef, app_path, 1024, kCFStringEncodingASCII);	
		CFRelease( mainBundleURL);
		CFRelease( cfStringRef);
		if (cfg[0]=='.') sprintf(cfg_path,"%s/.%s",app_path,cfg);
		else sprintf(cfg_path,"%s/Contents/Resources/%s",app_path,cfg);
	}
	//printf("%s %s\n",cfg,cfg_path);
	TiXmlDocument document( cfg_path );

#else
	TiXmlDocument document( cfg );
#endif	
	
	document.LoadFile();
	if( document.Error() )
	{
#ifdef __APPLE__
		std::cout << "Error loading MIDI configuration file: " << cfg_path << std::endl;
#else
		std::cout << "Error loading MIDI configuration file: " << cfg << std::endl;
#endif	
		return;
	}

	TiXmlHandle docHandle( &document );

	TiXmlElement* midi_device = docHandle.FirstChild("midi").Element();
	if(midi_device->Attribute("device")!=NULL) device = atoi(midi_device->Attribute("device"));

	TiXmlElement* map = docHandle.FirstChild("midi").FirstChild("map").Element();

	while (map) {

		mapping m = {0,0,0,0,0,0.0f,0.0f,0.0f,0};

	
		if( map->Attribute("fiducial")!=NULL ) m.f_id = atoi(map->Attribute("fiducial"));
		else { std::cout << "missing 'fiducial' atribute in MIDI configuration file" << std::endl; return; }
	
		if( map->Attribute("type")!=NULL ) {
			if ( strcmp( map->Attribute("type"), "hfader" ) == 0 ) m.dimension=XPOS;
			if ( strcmp( map->Attribute("type"), "vfader" ) == 0 ) m.dimension=YPOS;
			if ( strcmp( map->Attribute("type"), "knob" ) == 0 )   m.dimension=ANGLE;
			if ( strcmp( map->Attribute("type"), "note" ) == 0 )   m.dimension=PRESENCE;
		}
		else { std::cout << "missing 'type' atribute in MIDI configuration file" << std::endl; return; }
	
		if( map->Attribute("channel")!=NULL ) {
			m.channel = atoi(map->Attribute("channel"));
			if (m.channel<0) m.channel=0;
			if (m.channel>15) m.channel=15;
		} else m.channel = 0;
	
		if( map->Attribute("control")!=NULL ) {
			m.control = atoi(map->Attribute("control"));

			if( map->Attribute("min")!=NULL ) {
				m.min = atof(map->Attribute("min"));
				if (m.min<0) m.min=0.0;
				if (m.min>1) m.min=1.0;
				if (m.dimension==ANGLE) m.min*=2*M_PI;
			} else m.min = 0.0;
	
			if( map->Attribute("max")!=NULL ) {
				m.max = atof(map->Attribute("max"));
				if (m.max<0) m.max=0.0;
				if (m.max>1) m.max=1.0;
				if (m.dimension==ANGLE) m.max*=2*M_PI;
			} else m.max = 1.0;
			
			m.range = m.max  - m.min;
			
		} else if( map->Attribute("note")!=NULL ) m.control = atoi(map->Attribute("note"));
		else { std::cout << "missing 'control' or 'note' atribute in MIDI configuration file" << std::endl; return; }
	
		m.value = 0;
		mapping_list.push_back(m);

		map = map->NextSiblingElement( "map" );
	}
}

void MidiServer::sendControlMessage(int f_id, float xpos, float ypos, float angle)
{	
	if(device<0) return;

	if(invert_x) xpos=1-xpos;
	if(invert_y) ypos=1-ypos;
	if(invert_a) angle=TWO_PI-angle;

	for(std::list<mapping>::iterator map = mapping_list.begin(); map!=mapping_list.end(); map++) {

		if (map->f_id==f_id) {
			switch (map->dimension) {
				case XPOS:
					if ((xpos>=map->min) && (xpos<=map->max)) {
						map->value = (int)(((xpos - map->min)/map->range)*127);
						Pm_WriteShort(midi_out,0,Pm_Message((char)map->channel + 0xB0,(char)map->control, (char)map->value));
					}
					break;
				case YPOS:
					if ((ypos>=map->min) && (ypos<=map->max)) {
						map->value = (int)(((map->max - ypos)/map->range)*127);
						Pm_WriteShort(midi_out,0,Pm_Message((char)map->channel + 0xB0,(char)map->control, (char)map->value));
					}
					break;
				case ANGLE:
					if ((angle>=map->min) && (angle<=map->max)) {
						map->value = (int)(((angle - map->min)/map->range)*127);
						Pm_WriteShort(midi_out,0,Pm_Message((char)map->channel + 0xB0,(char)map->control, (char)map->value));
					}
					break;
			}
		}
	}
}

void MidiServer::sendAddMessage(int f_id)
{
	if(device<0) return;

	for(std::list<mapping>::iterator map = mapping_list.begin(); map!=mapping_list.end(); map++) {
		if ((map->f_id==f_id) && (map->dimension==PRESENCE)) {
			Pm_WriteShort(midi_out,0,Pm_Message(NOTE_ON | (char)map->channel,(char)map->control,127));
		}
	}
}


void MidiServer::sendRemoveMessage(int f_id)
{
	if(device<0) return;

	for(std::list<mapping>::iterator map = mapping_list.begin(); map!=mapping_list.end(); map++) {
		if ((map->f_id==f_id) && (map->dimension==PRESENCE)) {
			Pm_WriteShort(midi_out,0,Pm_Message(NOTE_OFF |(char)map->channel,(char)map->control,127));
		}
	}
}

void MidiServer::sendNullMessages()
{
	if(device<0) return;

	for(std::list<mapping>::iterator map = mapping_list.begin(); map!=mapping_list.end(); map++) {
		if (map->dimension==PRESENCE) Pm_WriteShort(midi_out,0,Pm_Message(NOTE_OFF | (char)map->channel,(char)map->control,127));
		else Pm_WriteShort(midi_out,0,Pm_Message((char)map->channel + 0xB0,(char)map->control,(char)0) );
	}
}


MidiServer::~MidiServer()
{
	// terminate MIDI
	if (device>=0) {
		Pm_Close(midi_out);
		Pm_Terminate();
	}
	running=false;
}
