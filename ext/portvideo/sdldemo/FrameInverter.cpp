/*  portVideo, a cross platform camera framework
 Copyright (C) 2005-2016 Martin Kaltenbrunner <martin@tuio.org>
 
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
 */


#include "FrameInverter.h"

void FrameInverter::process(unsigned char *src, unsigned char *dest) {
	
	if (src_format==FORMAT_GRAY) {
		for (int i=width*height;i>0;i--) {
			*dest++ = 255 - *src++;
		}
	} else {
		for (int i=width*height;i>0;i--) {
			*dest++ = (invert_red)   ? (255 - *src++):*src++;
			*dest++ = (invert_green) ? (255 - *src++):*src++;
			*dest++ = (invert_blue)  ? (255 - *src++):*src++;
		}
		
		if (show_settings) displayControl();
	}
}

bool FrameInverter::toggleFlag(unsigned char flag, bool lock) {
	
	if (src_format==FORMAT_GRAY) return lock;
	
	if (flag==KEY_I) {
		if (currentSetting != INV_NONE) {
			currentSetting = INV_NONE;
			show_settings = false;
			return false;
		} else if (!lock) {
			currentSetting = INV_RED;
			show_settings = true;
			return true;
		}
	} else if ( currentSetting != INV_NONE ) {
		switch(flag) {
			case KEY_LEFT:
				if (currentSetting==INV_RED) invert_red = false ;
				else if (currentSetting==INV_GREEN) invert_green = false ;
				else if (currentSetting==INV_BLUE) invert_blue = false ;
				break;
			case KEY_RIGHT:
				if (currentSetting==INV_RED) invert_red = true ;
				else if (currentSetting==INV_GREEN) invert_green = true ;
				else if (currentSetting==INV_BLUE) invert_blue = true ;
				break;
			case KEY_UP:
				currentSetting--;
				if (currentSetting < INV_RED) currentSetting = INV_BLUE;
				break;
			case KEY_DOWN:
				currentSetting++;
				if (currentSetting > INV_BLUE) currentSetting = INV_RED;
				break;
		}
	}
	return lock;
}

void FrameInverter::displayControl() {
	
	char displayText[64];
	int settingValue = 0;
	int maxValue = 1;
	
	switch (currentSetting) {
		case INV_NONE: return;
		case INV_RED: {
			sprintf(displayText,"invert RED channel %d",(int)invert_red);
			settingValue = (int)invert_red;
			break;
		}
		case INV_GREEN: {
			sprintf(displayText,"invert GREEN channel %d",(int)invert_green);
			settingValue = (int)invert_green;
			break;
		}
		case INV_BLUE: {
			sprintf(displayText,"invert BLUE channel %d",(int)invert_blue);
			settingValue = (int)invert_blue;
			break;
		}
	}
	
	ui->displayControl(displayText, 0, maxValue, settingValue);
}


