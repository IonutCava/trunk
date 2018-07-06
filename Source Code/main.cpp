#include "core.h"

#include "Core/Headers/Application.h"

I32 main(I32 argc, char **argv){

    FILE* output = nullptr;
    output = freopen(OUTPUT_LOG_FILE, "w", stdout);
	output = freopen(ERROR_LOG_FILE, "w", stderr);
	//Initialize our application based on XML configuration. Error codes are always less than 0
	ErrorCode returnCode = Application::getOrCreateInstance().initialize("main.xml",argc,argv);
	if(returnCode != NO_ERR){
		//If any error occurred, close the application as details should already be logged
        ERROR_FN("System failed to initialize properly. Error [ %s ] ", getErrorCodeName(returnCode));
		return returnCode;
	}
	Application::getInstance().run();
	//Stop our application
	Application::getInstance().deinitialize();
	//When the application is deleted, the last kernel used gets deleted as well
	Application::getInstance().destroyInstance();
	return NO_ERR;
}