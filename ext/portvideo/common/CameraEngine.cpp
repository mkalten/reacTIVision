/*  portVideo, a cross platform camera framework
 Copyright (C) 2005-2015 Martin Kaltenbrunner <martin@tuio.org>
 
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

#include "CameraEngine.h"
using namespace tinyxml2;

bool CameraEngine::showSettingsDialog(bool lock) {
    if (settingsDialog) {
        settingsDialog = false;
        updateSettings();
        saveSettings();
        return false;
    } else if (!lock) {
        currentCameraSetting = -1;
        for (int i=BRIGHTNESS;i<=GAMMA;i++) {
            if (hasCameraSetting(i)) {
                currentCameraSetting = i;
                break;
            }
        }
        if ( currentCameraSetting < 0) return false;
        settingsDialog = true;
        return true;
    } else return lock;
}

void CameraEngine::control(unsigned char key) {
    if(!settingsDialog) return;
    
    int value,min,max,step = 0;
    switch(key) {
        case VALUE_DOWN:
            value = getCameraSetting(currentCameraSetting);
            min = getMinCameraSetting(currentCameraSetting);
            max = getMaxCameraSetting(currentCameraSetting);
            step = getCameraSettingStep(currentCameraSetting);
            if (step==1) step = (int)((float)max/256.0f);
            if (step<1) step=1;
            value -= step;
            if (value<min) value=min;
            setCameraSetting(currentCameraSetting,value);
            break;
        case VALUE_UP:
            value = getCameraSetting(currentCameraSetting);
            min = getMinCameraSetting(currentCameraSetting);
            max = getMaxCameraSetting(currentCameraSetting);
            step = getCameraSettingStep(currentCameraSetting);
            if (step==1) step = (int)((float)max/256.0f);
            if (step<1) step=1;
            value += step;
            if (value>max) value=max;
            setCameraSetting(currentCameraSetting,value);
            break;
        case SETTING_PREVIOUS:
            currentCameraSetting--;
            if(currentCameraSetting<0) {
                if (colour) currentCameraSetting=COLOR_BLUE;
                else currentCameraSetting=GAMMA;
            }
            if (!hasCameraSetting(currentCameraSetting)) control(SETTING_PREVIOUS);
            break;
        case SETTING_NEXT:
            currentCameraSetting++;
            if ((colour) && (currentCameraSetting>COLOR_BLUE)) currentCameraSetting=0;
            else if ((!colour) && (currentCameraSetting>GAMMA)) currentCameraSetting=0;
            if (!hasCameraSetting(currentCameraSetting)) control(SETTING_NEXT);
            
            break;
    }
    
}

void CameraEngine::showInterface(UserInterface *interface) {
    if(!settingsDialog) return;
    
    int settingValue = getCameraSetting(currentCameraSetting);
    int maxValue =  getMaxCameraSetting(currentCameraSetting);
    int minValue =  getMinCameraSetting(currentCameraSetting);
    
    const char *settingText = NULL;
    switch (currentCameraSetting) {
        case BRIGHTNESS:	settingText = "Brightness"; break;
        case CONTRAST:		settingText = "Contrast";   break;
        case SHARPNESS:		settingText = "Sharpness";  break;
        case AUTO_GAIN:     settingText = "Auto Gain";  break;
        case GAIN:          settingText = "Gain";       break;
        case AUTO_EXPOSURE:	settingText = "Auto Exposure"; break;
        case EXPOSURE:		settingText = "Exposure";   break;
        case SHUTTER:		settingText = "Shutter";    break;
        case AUTO_FOCUS:    settingText = "Auto Focus"; break;
        case FOCUS:         settingText = "Focus";      break;
        case GAMMA:         settingText = "Gamma";      break;
        case AUTO_WHITE:    settingText = "Auto White"; break;
        case WHITE:         settingText = "White Balance"; break;
        case BACKLIGHT:     settingText = "Backlight Compensation"; break;
        case COLOR_HUE:     settingText = "Auto Hue";  break;
        case AUTO_HUE:      settingText = "Color Hue";  break;
        case COLOR_RED:     settingText = "Red Balance";  break;
        case COLOR_BLUE:    settingText = "Blue Balance"; break;
        case COLOR_GREEN:   settingText = "Green Balance"; break;
    }
    
    char displayText[256];
    sprintf(displayText,"%s %d",settingText,settingValue);
    
    interface->displayControl(displayText, minValue, maxValue, settingValue);
}

void CameraEngine::readSettings() {
    
    config.device = 0;
    config.color = false;
    config.compress = false;
    config.cam_width = SETTING_MAX;
    config.cam_height = SETTING_MAX;
    config.cam_fps = SETTING_MAX;
    config.frame = false;
    config.frame_xoff = 0;
    config.frame_yoff = 0;
    config.frame_width = SETTING_MAX;
    config.frame_height = SETTING_MAX;
    sprintf(config.file,"none");
    sprintf(config.folder,"none");
    config.brightness = SETTING_DEFAULT;
    config.contrast = SETTING_DEFAULT;
    config.sharpness = SETTING_DEFAULT;
    config.gain = SETTING_AUTO;
    config.gamma = SETTING_AUTO;
    config.exposure = SETTING_AUTO;
    config.shutter = SETTING_AUTO;
    config.focus = SETTING_AUTO;
    config.white = SETTING_AUTO;
    config.backlight = SETTING_AUTO;
    config.hue = SETTING_AUTO;
    config.blue = SETTING_DEFAULT;
    config.red = SETTING_DEFAULT;
    config.green = SETTING_DEFAULT;
    
    default_brightness = SETTING_DEFAULT;
    default_contrast = SETTING_DEFAULT;
    default_sharpness = SETTING_DEFAULT;
    default_gain = SETTING_AUTO;
    default_exposure = SETTING_AUTO;
    default_shutter = SETTING_AUTO;
    default_focus = SETTING_AUTO;
    default_gamma = SETTING_AUTO;
    default_white = SETTING_AUTO;
    default_backlight = SETTING_AUTO;
    default_hue = SETTING_AUTO;
    default_red = SETTING_DEFAULT;
    default_blue = SETTING_DEFAULT;
    default_green = SETTING_DEFAULT;
    
#ifdef __APPLE__
    char path[1024];
#endif
    
    if (strcmp( config_file, "none" ) == 0) {
#ifdef __APPLE__
        CFBundleRef mainBundle = CFBundleGetMainBundle();
        CFURLRef mainBundleURL = CFBundleCopyBundleURL( mainBundle);
        CFStringRef cfStringRef = CFURLCopyFileSystemPath( mainBundleURL, kCFURLPOSIXPathStyle);
        CFStringGetCString( cfStringRef, path, 1024, kCFStringEncodingASCII);
        CFRelease( mainBundleURL);
        CFRelease( cfStringRef);
        sprintf(full_path,"%s/Contents/Resources/camera.xml",path);
        config_file = full_path;
#elif !defined WIN32
        if (access ("./camera.xml", F_OK )==0) config_file = "./camera.xml";
        else if (access ("/usr/share/reacTIVision/camera.xml", F_OK )==0) config_file = "/usr/share/reacTIVision/camera.xml";
        else if (access ("/usr/local/share/reacTIVision/camera.xml", F_OK )==0) config_file = "/usr/local/share/camera.xml";
        else if (access ("/opt/share/reacTIVision/camera.xml", F_OK )==0) config_file = "/opt/share/reacTIVision/camera.xml";
#else
        config_file = "./camera.xml";
#endif
    }
    
    XMLDocument xml_settings;
    xml_settings.LoadFile( config_file);
    if( xml_settings.Error() )
    {
        std::cout << "Error loading camera configuration file: " << config_file << std::endl;
        return;
    }
    
    XMLHandle docHandle( &xml_settings );
    XMLHandle camera = docHandle.FirstChildElement("portvideo").FirstChildElement("camera");
    XMLElement* camera_element = camera.ToElement();
    
    
    if( camera_element==NULL )
    {
        std::cout << "Error loading camera configuration file: " << config_file << std::endl;
        return;
    }
    
    if(camera_element->Attribute("id")!=NULL) {
        if (strcmp(camera_element->Attribute("id"), "auto" ) == 0) config.device=SETTING_AUTO;
        else config.device = atoi(camera_element->Attribute("id"));
    }
    
    if(camera_element->Attribute("file")!=NULL) {
#ifdef __APPLE__
        sprintf(config.file,"%s/../%s",path,camera_element->Attribute("file"));
#else
        sprintf(config.file,"%s",camera_element->Attribute("file"));
#endif
    }
    if(camera_element->Attribute("folder")!=NULL) {
#ifdef __APPLE__
        sprintf(config.folder,"%s/../%s",path,camera_element->Attribute("folder"));
#else
        sprintf(config.folder,"%s",camera_element->Attribute("folder"));
#endif
    }
    
    XMLElement* image_element = camera.FirstChildElement("capture").ToElement();
    
    if (image_element!=NULL) {
        if ((image_element->Attribute("color")!=NULL) && ( strcmp( image_element->Attribute("color"), "true" ) == 0 )) config.color = true;
        
        if ((image_element->Attribute("compress")!=NULL) && ( strcmp( image_element->Attribute("compress"), "true" ) == 0)) config.compress = true;
        
        if(image_element->Attribute("width")!=NULL) {
            if (strcmp( image_element->Attribute("width"), "max" ) == 0) config.cam_width = SETTING_MAX;
            else config.cam_width = atoi(image_element->Attribute("width"));
        }
        if(image_element->Attribute("height")!=NULL) {
            if (strcmp( image_element->Attribute("height"), "max" ) == 0) config.cam_height = SETTING_MAX;
            else config.cam_height = atoi(image_element->Attribute("height"));
        }
        if(image_element->Attribute("fps")!=NULL) {
            if (strcmp( image_element->Attribute("fps"), "max" ) == 0) config.cam_fps = SETTING_MAX;
            else config.cam_fps = atof(image_element->Attribute("fps"));
        }
    }
    
    XMLElement* frame_element = camera.FirstChildElement("frame").ToElement();
    if (frame_element!=NULL) {
        config.frame = true;
        
        if(frame_element->Attribute("width")!=NULL) {
            if (strcmp( frame_element->Attribute("width"), "max" ) == 0) config.frame_width = SETTING_MAX;
            else if (strcmp( frame_element->Attribute("width"), "min" ) == 0) config.frame_width = 0;
            else config.frame_width = atoi(frame_element->Attribute("width"));
        }
        
        if(frame_element->Attribute("height")!=NULL) {
            if (strcmp( frame_element->Attribute("height"), "max" ) == 0) config.frame_height = SETTING_MAX;
            else if (strcmp( frame_element->Attribute("height"), "min" ) == 0) config.frame_height = 0;
            else config.frame_height = atoi(frame_element->Attribute("height"));
        }
        
        
        if(frame_element->Attribute("xoff")!=NULL) {
            if (strcmp( frame_element->Attribute("xoff"), "max" ) == 0) config.frame_xoff = SETTING_MAX;
            else if (strcmp( frame_element->Attribute("xoff"), "min" ) == 0) config.frame_xoff = 0;
            else config.frame_xoff = atoi(frame_element->Attribute("xoff"));
        }
        
        if(frame_element->Attribute("yoff")!=NULL) {
            if (strcmp( frame_element->Attribute("yoff"), "max" ) == 0) config.frame_yoff = SETTING_MAX;
            else if (strcmp( frame_element->Attribute("yoff"), "min" ) == 0) config.frame_yoff = 0;
            else config.frame_yoff = atoi(frame_element->Attribute("yoff"));
        }
    }
    
    XMLElement* settings_element = camera.FirstChildElement("settings").ToElement();
    if (settings_element!=NULL) {
        
        config.brightness = readAttribute(settings_element, "brightness");
        config.contrast = readAttribute(settings_element, "contrast");
        config.sharpness = readAttribute(settings_element, "sharpness");
        config.gain = readAttribute(settings_element, "gain");
        config.exposure = readAttribute(settings_element, "exposure");
        config.shutter = readAttribute(settings_element, "shutter");
        config.focus = readAttribute(settings_element, "focus");
        config.white = readAttribute(settings_element, "white");
        config.gamma = readAttribute(settings_element, "gamma");
        config.backlight = readAttribute(settings_element, "backlight");
        
        if (config.color) {
            config.hue = readAttribute(settings_element, "hue");
            config.red = readAttribute(settings_element, "red");
            config.green = readAttribute(settings_element, "green");
            config.blue = readAttribute(settings_element, "blue");
        }
    }
    
    cam_width = config.cam_width;
    cam_height = config.cam_height;
    colour = config.color;
    bytes = (colour?3:1);
}

int CameraEngine::readAttribute(XMLElement* settings,const char *attribute) {
    
    if(settings->Attribute(attribute)!=NULL) {
        if (strcmp(settings->Attribute(attribute), "min" ) == 0) return SETTING_MIN;
        else if (strcmp(settings->Attribute(attribute), "max" ) == 0) return SETTING_MAX;
        else if (strcmp(settings->Attribute(attribute), "auto" ) == 0) return SETTING_AUTO;
        else if (strcmp(settings->Attribute(attribute), "default" ) == 0) return SETTING_DEFAULT;
        else return atoi(settings->Attribute(attribute));
    }
    
    return SETTING_DEFAULT;
}

void CameraEngine::saveAttribute(XMLElement* settings,const char *attribute,int config) {
    
    if (config==SETTING_MIN) settings->SetAttribute(attribute,"min");
    else if (config==SETTING_MAX) settings->SetAttribute(attribute,"max");
    else if (config==SETTING_AUTO) settings->SetAttribute(attribute,"auto");
    else if (config==SETTING_DEFAULT) settings->SetAttribute(attribute,"default");
    else {
        char value[64];
        sprintf(value,"%d",config);
        settings->SetAttribute(attribute,value);
    }
}

void CameraEngine::saveSettings() {
    
    if (strcmp( config_file, "none" ) == 0) {
#ifdef __APPLE__
        char path[1024];
        CFBundleRef mainBundle = CFBundleGetMainBundle();
        CFURLRef mainBundleURL = CFBundleCopyBundleURL( mainBundle);
        CFStringRef cfStringRef = CFURLCopyFileSystemPath( mainBundleURL, kCFURLPOSIXPathStyle);
        CFStringGetCString( cfStringRef, path, 1024, kCFStringEncodingASCII);
        CFRelease( mainBundleURL);
        CFRelease( cfStringRef);
        sprintf(full_path,"%s/Contents/Resources/camera.xml",path);
        config_file = full_path;
#else
        config_file = "./camera.xml";
#endif
    }
    
    XMLDocument xml_settings;
    xml_settings.LoadFile(config_file);
    if( xml_settings.Error() )
    {
        std::cout << "Error loading camera configuration file: " << config_file << std::endl;
        return;
    }
    
    XMLHandle docHandle( &xml_settings );
    XMLHandle camera = docHandle.FirstChildElement("portvideo").FirstChildElement("camera");
    XMLElement* settings_element = camera.FirstChildElement("settings").ToElement();
    
    if (settings_element!=NULL) {
        
        if (hasCameraSetting(BRIGHTNESS)) saveAttribute(settings_element, "brightness", config.brightness);
        else settings_element->DeleteAttribute("brightness");
        if (hasCameraSetting(CONTRAST)) saveAttribute(settings_element, "contrast", config.contrast);
        else settings_element->DeleteAttribute("contrast");
        if (hasCameraSetting(SHARPNESS)) saveAttribute(settings_element, "sharpness", config.sharpness);
        else settings_element->DeleteAttribute("sharpness");
        if (hasCameraSetting(GAIN)) saveAttribute(settings_element, "gain", config.gain);
        else settings_element->DeleteAttribute("gain");
        if (hasCameraSetting(EXPOSURE)) saveAttribute(settings_element, "exposure", config.exposure);
        else settings_element->DeleteAttribute("exposure");
        if (hasCameraSetting(FOCUS)) saveAttribute(settings_element, "focus", config.focus);
        else settings_element->DeleteAttribute("focus");
        if (hasCameraSetting(SHUTTER)) saveAttribute(settings_element, "shutter", config.shutter);
        else settings_element->DeleteAttribute("shutter");
        if (hasCameraSetting(WHITE)) saveAttribute(settings_element, "white", config.white);
        else settings_element->DeleteAttribute("white");
        if (hasCameraSetting(BACKLIGHT)) saveAttribute(settings_element, "backlight", config.backlight);
        else settings_element->DeleteAttribute("backlight");
        if (hasCameraSetting(GAMMA)) saveAttribute(settings_element, "gamma", config.gamma);
        else settings_element->DeleteAttribute("gamma");
        
        if (colour) {
            if (hasCameraSetting(COLOR_HUE)) saveAttribute(settings_element, "hue", config.hue);
            else settings_element->DeleteAttribute("hue");
            if (hasCameraSetting(COLOR_RED)) saveAttribute(settings_element, "red", config.red);
            else settings_element->DeleteAttribute("red");
            if (hasCameraSetting(COLOR_GREEN)) saveAttribute(settings_element, "green", config.green);
            else settings_element->DeleteAttribute("green");
            if (hasCameraSetting(COLOR_BLUE)) saveAttribute(settings_element, "blue", config.green);
            else settings_element->DeleteAttribute("blue");
        } else {
            settings_element->DeleteAttribute("hue");
            settings_element->DeleteAttribute("red");
            settings_element->DeleteAttribute("green");
            settings_element->DeleteAttribute("blue");
        }
    }
    
    xml_settings.SaveFile(config_file);
    if( xml_settings.Error() ) std::cout << "Error saving camera configuration file: "  << config_file << std::endl;
    
}

void CameraEngine::uyvy2gray(int width, int height, unsigned char *src, unsigned char *dest) {
    
    for (int i=height*width/2;i>0;i--) {
        src++;
        *dest++ = *src++;
        src++;
        *dest++ = *src++;
    }
}

void CameraEngine::yuyv2gray(int width, int height, unsigned char *src, unsigned char *dest) {
    for (int i=height*width/2;i>0;i--) {
        *dest++ = *src++;
        src++;
        *dest++ = *src++;
        src++;
    }
}

void CameraEngine::crop_yuyv2gray(int src_w, int dest_w, int dest_h, int x_off, int y_off, unsigned char *src, unsigned char *dest) {
    src += 2*y_off*src_w;
    int x_end = src_w-(dest_w+x_off);
    
    for (int i=0;i<dest_h;i++) {
        
        src +=  2*x_off;
        for (int j=dest_w/2;j>0;j--) {
            *dest++ = *src++;
            src++;
            *dest++ = *src++;
            src++;
        }
        src +=  2*x_end;
    }
}


void CameraEngine::uyvy2rgb(int width, int height, unsigned char *src, unsigned char *dest) {
    
    int R,G,B;
    int Y1,Y2;
    int cG,cR,cB;
    
    for(int i=height*width/2;i>0;i--) {
        cB = ((*src - 128) * 454) >> 8;
        cG = (*src++ - 128) * 88;
        Y1 = *src++;
        cR = ((*src - 128) * 359) >> 8;
        cG = (cG + (*src++ - 128) * 183) >> 8;
        Y2 = *src++;
        
        R = Y1 + cR;
        G = Y1 + cG;
        B = Y1 + cB;
        
        SAT(R);
        SAT(G);
        SAT(B);
        
        *dest++ = B;
        *dest++ = G;
        *dest++ = R;
        
        R = Y2 + cR;
        G = Y2 + cG;
        B = Y2 + cB;
        
        SAT(R);
        SAT(G);
        SAT(B);
        
        *dest++ = B;
        *dest++ = G;
        *dest++ = R;
    }
}

void CameraEngine::yuyv2rgb(int width, int height, unsigned char *src, unsigned char *dest) {
    
    int R,G,B;
    int Y1,Y2;
    int cG,cR,cB;
    
    for(int i=height*width/2;i>0;i--) {
        Y1 = *src++;
        cR = ((*src - 128) * 454) >> 8;
        cG = (*src++ - 128) * 88;
        Y2 = *src++;
        cB = ((*src - 128) * 359) >> 8;
        cG = (cG + (*src++ - 128) * 183) >> 8;
        
        R = Y1 + cR;
        G = Y1 + cG;
        B = Y1 + cB;
        
        SAT(R);
        SAT(G);
        SAT(B);
        
        *dest++ = B;
        *dest++ = G;
        *dest++ = R;
        
        R = Y2 + cR;
        G = Y2 + cG;
        B = Y2 + cB;
        
        SAT(R);
        SAT(G);
        SAT(B);
        
        *dest++ = B;
        *dest++ = G;
        *dest++ = R;
    }
}


void CameraEngine::rgb2gray(int width, int height, unsigned char *src, unsigned char *dest) {
}

void CameraEngine::setupFrame() {
    
    if(!config.frame) {
        
        frame_width = cam_width;
        frame_height = cam_height;
        return;
    }
    
    // size sanity check
    if (config.frame_width%2!=0) config.frame_width--;
    if (config.frame_height%2!=0) config.frame_height--;
    
    if (config.frame_width<=0) config.frame_width = cam_width;
    if (config.frame_height<=0) config.frame_height = cam_height;
    
    if (config.frame_width > config.cam_width) config.frame_width = config.cam_width;
    if (config.frame_height > config.cam_height) config.frame_height = config.cam_height;
    
    // no cropping if same size
    if ((config.frame_width==config.cam_width) && (config.frame_height==config.cam_height)) {
        
        frame_width = cam_width;
        frame_height = cam_height;
        config.frame = false;
        return;
    }
    
    // offset sanity check
    int xdiff = config.cam_width-config.frame_width;
    if (xdiff<0) config.frame_xoff = 0;
    else if (config.frame_xoff > xdiff) config.frame_xoff = xdiff;
    int ydiff = config.cam_height-config.frame_height;
    if (ydiff<0) config.frame_yoff = 0;
    else if (config.frame_yoff > ydiff) config.frame_yoff = ydiff;
    
    frame_width = config.frame_width;
    frame_height = config.frame_height;
}

void CameraEngine::cropFrame(unsigned char *cambuf, unsigned char *cropbuf, int bytes) {
    
    if(!config.frame) return;
    
    unsigned char *src = cambuf + bytes*(config.frame_yoff*config.cam_width + config.frame_xoff);
    unsigned char *dest = cropbuf;
    
    for (int i=0;i<config.frame_height;i++) {
        memcpy(dest, src, bytes*config.frame_width);
        
        src += bytes*config.cam_width;
        dest += bytes*config.frame_width;
    }
}


void CameraEngine::applyCameraSetting(int mode, int value) {
    
    switch (value) {
        case SETTING_DEFAULT:
            setDefaultCameraSetting(mode);
            return;
        case SETTING_AUTO:
            setCameraSettingAuto(mode,true);
            return;
        case SETTING_MIN:
            setCameraSetting(mode,getMinCameraSetting(mode)); return;
        case SETTING_MAX:
            setCameraSetting(mode,getMaxCameraSetting(mode)); return;
        default: {
            int max = getMaxCameraSetting(mode);
            int min = getMinCameraSetting(mode);
            if (value<min) value = min;
            else if (value>max) value = max;
            setCameraSetting(mode,value);
        }
    }
}

void CameraEngine::applyCameraSettings() {
    
    for (int mode=BRIGHTNESS;mode<=COLOR_BLUE;mode++)
        setDefaultCameraSetting(mode);
    
    applyCameraSetting(BRIGHTNESS,config.brightness);
    applyCameraSetting(CONTRAST,config.contrast);
    applyCameraSetting(SHARPNESS,config.sharpness);
    applyCameraSetting(GAIN,config.gain);
    applyCameraSetting(EXPOSURE,config.exposure);
    applyCameraSetting(SHUTTER,config.shutter);
    applyCameraSetting(FOCUS,config.focus);
    applyCameraSetting(WHITE,config.white);
    applyCameraSetting(BACKLIGHT,config.backlight);
    applyCameraSetting(GAMMA,config.gamma);
    
    applyCameraSetting(COLOR_HUE,config.red);
    applyCameraSetting(COLOR_RED,config.red);
    applyCameraSetting(COLOR_GREEN,config.green);
    applyCameraSetting(COLOR_BLUE,config.blue);
}

int CameraEngine::updateSetting(int mode) {
    
    if (getCameraSettingAuto(mode)) return SETTING_AUTO;
    
    int value = getCameraSetting(mode);
    if (value==getMinCameraSetting(mode)) value = SETTING_MIN;
    else if (value==getMaxCameraSetting(mode)) value = SETTING_MAX;
    else if (value==getDefaultCameraSetting(mode)) value = SETTING_DEFAULT;
    return value;
}

void CameraEngine::updateSettings() {
    
    config.brightness = updateSetting(BRIGHTNESS);
    config.contrast = updateSetting(CONTRAST);
    config.sharpness = updateSetting(SHARPNESS);
    
    config.gain = updateSetting(GAIN);
    config.exposure = updateSetting(EXPOSURE);
    config.shutter = updateSetting(SHUTTER);
    config.focus = updateSetting(FOCUS);
    config.white = updateSetting(WHITE);
    config.backlight = updateSetting(BACKLIGHT);
    config.gamma = updateSetting(GAMMA);
    
    config.hue = updateSetting(COLOR_HUE);
    config.red = updateSetting(COLOR_RED);
    config.green = updateSetting(COLOR_GREEN);
    config.blue = updateSetting(COLOR_BLUE);
    
}
