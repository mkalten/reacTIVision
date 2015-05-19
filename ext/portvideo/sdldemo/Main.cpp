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


#include <Main.h>
#include <string.h>
#ifdef __APPLE__
#include <SDL2/SDL.h>
#else
#include "SDL.h"
#endif
#include "SDLinterface.h"
#include "FrameInverter.h"

#ifdef LINUX
#include <signal.h>
#endif

SDLinterface *engine;

static void terminate (int param)
{
	printf("terminating portVideoSDL ...\n");
	if (engine!=NULL) engine->stop();
}

void printUsage() {
	std::cout << "usage: portVideoSDL -c [config_file]" << std::endl;
	std::cout << "the default configuration file is camera.xml" << std::endl;
	std::cout << "\t -n starts portVideoSDL without GUI" << std::endl;
	std::cout << "\t -l lists all available cameras" << std::endl;
	std::cout << "\t -h shows this help message" << std::endl;
	std::cout << std::endl;
}

void readSettings(application_settings *config) {

	sprintf(config->camera_config,"none");
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

	TiXmlDocument xml_settings( config->file );
	xml_settings.LoadFile();
	if( xml_settings.Error() )
	{
		std::cout << "Error loading configuration file: " << config->file << std::endl;
		return;
	}

	TiXmlHandle docHandle( &xml_settings );
	TiXmlHandle config_root = docHandle.FirstChild("portvideo");

	TiXmlElement* camera_element = config_root.FirstChild("camera").Element();
	if( camera_element!=NULL )
	{
		if(camera_element->Attribute("config")!=NULL) sprintf(config->camera_config,"%s",camera_element->Attribute("config"));
	}

	TiXmlElement* image_element = config_root.FirstChild("image").Element();
	if( image_element!=NULL )
	{
		if(image_element->Attribute("display")!=NULL)  {
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

	TiXmlDocument xml_settings( config->file );
	xml_settings.LoadFile();
	if( xml_settings.Error() )
	{
		std::cout << "Error loading configuration file: " << config->file << std::endl;
		return;
	}

	TiXmlHandle docHandle( &xml_settings );
	TiXmlHandle config_root = docHandle.FirstChild("portivideo");

	TiXmlElement* camera_element = config_root.FirstChild("camera").Element();
	if( camera_element!=NULL )
	{
		if(camera_element->Attribute("config")!=NULL) camera_element->SetAttribute("config",config->camera_config);
	}

	TiXmlElement* image_element = config_root.FirstChild("image").Element();
	if( image_element!=NULL )
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

	xml_settings.SaveFile();
	if( xml_settings.Error() ) std::cout << "Error saving configuration file: "  << config->file << std::endl;

}


CameraEngine* setupCamera(char *camera_config) {

    CameraEngine *camera = CameraTool::findCamera(camera_config);
    if (camera == NULL) return NULL;

    bool success = camera->initCamera();

    if(success) {
        int width = camera->getWidth();
        int height = camera->getHeight();
        float fps = camera->getFps();

        printf("camera: %s\n",camera->getName());
        if (fps>0) {
		if (int(fps)==fps) printf("format: %dx%d, %dfps\n",width,height,int(fps));
		else printf("format: %dx%d, %'.1f fps\n",width,height,fps);
        } else printf("format: %dx%d\n",width,height);

        return camera;
    } else {
        printf("could not initialize camera\n");
        camera->closeCamera();
        delete camera;
        return NULL;
    }
}

void teardownCamera(CameraEngine *camera)
{
    if (camera!=NULL) {
        camera->stopCamera();
        camera->closeCamera();
        delete camera;
    }
}

int main(int argc, char* argv[]) {

	application_settings config;
	sprintf(config.file,"none");

	const char *app_name = "portVideoSDL";
	const char *version_no = "0.5";

	bool headless = false;

	std::cout << app_name << " " << version_no << " (" << __DATE__ << ")" << std::endl << std::endl;

	if (argc>1) {
		if (strcmp( argv[1], "-h" ) == 0 ) {
			printUsage();
			return 0;
		} else if( strcmp( argv[1], "-c" ) == 0 ) {
			if (argc==3) sprintf(config.file,"%s",argv[2]);
			else {
				printUsage();
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
 			printUsage();
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

	CameraEngine *camera = setupCamera(config.camera_config);

	engine = new SDLinterface(app_name, camera, &config);

    if (!headless) {
        switch (config.display_mode) {
            case 0: engine->setDisplayMode(engine->NO_DISPLAY); break;
            case 1: engine->setDisplayMode(engine->SOURCE_DISPLAY); break;
            case 2: engine->setDisplayMode(engine->DEST_DISPLAY); break;
        }
    } else engine->setDisplayMode(engine->NO_DISPLAY);

	FrameProcessor *frameinverter = new FrameInverter();
    engine->addFrameProcessor(frameinverter);
	engine->run();

	config.display_mode = engine->getDisplayMode();
	engine->removeFrameProcessor(frameinverter);
    teardownCamera(camera);

	delete frameinverter;
	delete engine;

	writeSettings(&config);
	return 0;
}
