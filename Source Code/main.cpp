#include "core.h"
#include "Core/Headers/Application.h"

///Comment this out to show the debug console
#pragma comment( linker,"/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )

int main(int argc, char **argv) {

    FILE* output = nullptr;
    output = freopen(OUTPUT_LOG_FILE, "w", stdout);
	output = freopen(ERROR_LOG_FILE, "w", stderr);
	//Initialize our application based on XML configuration. Error codes are always less than 0
	Divide::ErrorCode returnCode = Divide::Application::getOrCreateInstance().initialize("main.xml",
                                                                                         argc,
                                                                                         argv);
	if(returnCode != Divide::NO_ERR){
		//If any error occurred, close the application as details should already be logged
        Divide::ERROR_FN("System failed to initialize properly. Error [ %s ] ", 
                         getErrorCodeName(returnCode));
		return returnCode;
	}
	Divide::Application::getInstance().run();
	//Stop our application
	Divide::Application::getInstance().deinitialize();
	//When the application is deleted, the last kernel used gets deleted as well
	Divide::Application::getInstance().destroyInstance();
	return Divide::NO_ERR;
}