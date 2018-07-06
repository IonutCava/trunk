#include "core.h"

#include "Core/Headers/Application.h"

#ifdef main
#undef main
#endif

I32 main(I32 argc, char **argv){
	freopen(OUTPUT_LOG_FILE, "w", stdout);
	freopen(ERROR_LOG_FILE, "w", stderr);
	//Initialize our application based on XML configuration. Error codes are always less than 0
	I8 returnCode = Application::getOrCreateInstance().initialize("main.xml",argc,argv);
	if(returnCode < 0){
		//If any error occured, close the application as details should already be logged
		return returnCode;
	}
	Application::getInstance().run();
	//Stop our application
	Application::getInstance().deinitialize();
	//When the application is deleted, the last kernel used gets deleted as well
	Application::getInstance().destroyInstance();
	return NO_ERR;
}