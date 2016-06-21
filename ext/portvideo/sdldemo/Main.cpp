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


#include <Main.h>
#include <string.h>
#include <SDLinterface.h>
#include <ConsoleInterface.h>
#include <VisionEngine.h>
#include <FrameInverter.h>

#ifdef LINUX
#include <signal.h>
#endif

VisionEngine *engine;

static void terminate (int param)
{
	if (engine!=NULL) engine->stop();
}

void printUsage(const char *app_name) {
	std::cout << "usage: " << app_name << " -c [config_file]" << std::endl;
	std::cout << "the default configuration file is camera.xml" << std::endl;
	std::cout << "\t -n starts " << app_name << " without GUI" << std::endl;
	std::cout << "\t -l lists all available cameras" << std::endl;
	std::cout << "\t -h shows this help message" << std::endl;
	std::cout << std::endl;
}

void readSettings(application_settings *config) {

	sprintf(config->camera_config,"default");
	config->background = false;
	config->fullscreen = false;
	config->headless = false;
	config->display_mode = 2;

	if (strcmp( config->file, "none" ) == 0) {
#ifdef __APPLE__
	char app_path[1024];
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	CFURLRef mainBundleURL = CFBundleCopyBundleURL( mainBundle);
	CFStringRef cfStringRef = CFURLCopyFileSystemPath( mainBundleURL, kCFURLPOSIXPathStyle);
	CFStringGetCString( cfStringRef, app_path, 1024, kCFStringEncodingASCII);	
	CFRelease( mainBundleURL);
	CFRelease( cfStringRef);
	sprintf(config->file,"%s/Contents/Resources/portvideo.xml",app_path);
#elif !defined WIN32
        if (access ("./portvideo.xml", F_OK )==0) sprintf(config->file,"./portvideo.xml");
        else if (access ("/usr/share/portvideo/portvideo.xml", F_OK )==0) sprintf(config->file,"/usr/share/portvideo/portvideo.xml");
        else if (access ("/usr/local/share/portvideo/portvideo.xml", F_OK )==0) sprintf(config->file,"/usr/local/share/portvideo/portvideo.xml");
        else if (access ("/opt/share/portvideo/portvideo.xml", F_OK )==0) sprintf(config->file,"/opt/share/portvideo/portvideo.xml");
#else
        sprintf(config->file,"./portvideo.xml");
#endif
	}

	tinyxml2::XMLDocument xml_settings;
	xml_settings.LoadFile(config->file);
	if(xml_settings.Error()) {
		std::cout << "Error loading configuration file: " << config->file << std::endl;
		return;
	}

	tinyxml2::XMLHandle docHandle( &xml_settings );
	tinyxml2::XMLHandle config_root = docHandle.FirstChildElement("portvideo");

	tinyxml2::XMLElement* camera_element = config_root.FirstChildElement("camera").ToElement();
	if(camera_element!=NULL) {
		if(camera_element->Attribute("config")!=NULL) sprintf(config->camera_config,"%s",camera_element->Attribute("config"));
	}

	tinyxml2::XMLElement* image_element = config_root.FirstChildElement("image").ToElement();
	if(image_element!=NULL)
	{
		if(image_element->Attribute("display")!=NULL) {
			if ( strcmp( image_element->Attribute("display"), "none" ) == 0 ) config->display_mode = 0;
			else if ( strcmp( image_element->Attribute("display"), "src" ) == 0 )  config->display_mode = 1;
			else if ( strcmp( image_element->Attribute("display"), "dest" ) == 0 )  config->display_mode = 2;
		}
        
		if(image_element->Attribute("equalize")!=NULL) {
			if ((strcmp( image_element->Attribute("equalize"), "true" ) == 0) ||  atoi(image_element->Attribute("equalize"))==1) config->background = true;
		}

        if(image_element->Attribute("fullscreen")!=NULL) {
            if ((strcmp( image_element->Attribute("fullscreen"), "true" ) == 0) ||  atoi(image_element->Attribute("fullscreen"))==1) config->fullscreen = true;
        }
	}
}

void writeSettings(application_settings *config) {

	tinyxml2::XMLDocument xml_settings;
	xml_settings.LoadFile(config->file);
	if(xml_settings.Error()) {
		std::cout << "Error saving configuration file: " << config->file << std::endl;
		return;
	}

	tinyxml2::XMLHandle docHandle(&xml_settings);
	tinyxml2::XMLHandle config_root = docHandle.FirstChildElement("portvideo");

	tinyxml2::XMLElement* camera_element = config_root.FirstChildElement("camera").ToElement();
	if( camera_element!=NULL ) {
		if(camera_element->Attribute("config")!=NULL) camera_element->SetAttribute("config",config->camera_config);
	}

	tinyxml2::XMLElement* image_element = config_root.FirstChildElement("image").ToElement();
	if(image_element!=NULL)
    {
		if(image_element->Attribute("display")!=NULL)  {
			if (config->display_mode == 0) image_element->SetAttribute("display","none");
			else if (config->display_mode == 1) image_element->SetAttribute("display","src");
			else if (config->display_mode == 2) image_element->SetAttribute("display","dest");  
		}
		if(image_element->Attribute("equalize")!=NULL) {
			if (config->background) image_element->SetAttribute("equalize","true");
			else image_element->SetAttribute("equalize","false");
		}
        if(image_element->Attribute("fullscreen")!=NULL) {
            if (config->fullscreen) image_element->SetAttribute("fullscreen","true");
            else image_element->SetAttribute("fullscreen","false");
        }
	}

	xml_settings.SaveFile(config->file);
	if( xml_settings.Error() ) std::cout << "Error saving configuration file: "  << config->file << std::endl;
}

int main(int argc, char* argv[]) {

	application_settings config;
	sprintf(config.file,"none");

	const char *app_name = "SportVideo";
	const char *version_no = "0.6";

	bool headless = false;

	std::cout << app_name << " " << version_no << " (" << __DATE__ << ")" << std::endl << std::endl;

	if (argc>1) {
		if (strcmp( argv[1], "-h" ) == 0 ) {
			printUsage(app_name);
			return 0;
		} else if( strcmp( argv[1], "-c" ) == 0 ) {
			if (argc==3) sprintf(config.file,"%s",argv[2]);
			else {
				printUsage(app_name);
				return 0;
			}
        } else if( strcmp( argv[1], "-n" ) == 0 ) {
            headless = true;
        } else if( strcmp( argv[1], "-l" ) == 0 ) {
            CameraTool::listDevices();
            return 0;
        } else if ( (std::string(argv[1]).find("-NSDocumentRevisionsDebugMode")==0 ) || (std::string(argv[1]).find("-psn_")==0) ){
            // ignore mac specific arguments
        } else {
 			printUsage(app_name);
		}
	}

#ifndef WIN32
	signal(SIGINT,terminate);
	signal(SIGHUP,terminate);
	signal(SIGQUIT,terminate);
	signal(SIGTERM,terminate);
#endif

	readSettings(&config);
    config.headless = headless;

	engine = new VisionEngine(app_name, &config);
 
	UserInterface *uiface;
    if (!headless) {
        uiface = new SDLinterface(app_name,config.fullscreen);
        switch (config.display_mode) {
            case 0: uiface->setDisplayMode(NO_DISPLAY); break;
            case 1: uiface->setDisplayMode(SOURCE_DISPLAY); break;
            case 2: uiface->setDisplayMode(DEST_DISPLAY); break;
        }
	} else uiface = new ConsoleInterface(app_name);

	engine->setInterface(uiface);


	FrameProcessor *frameinverter = new FrameInverter();
    engine->addFrameProcessor(frameinverter);
    
	engine->start();
    
	engine->removeFrameProcessor(frameinverter);
	delete frameinverter;

	delete engine;
	writeSettings(&config);
	return 0;
}
