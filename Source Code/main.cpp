#include "config.h"
#include "Core/Headers/Application.h"

#ifdef main
#undef main
#endif 

I32 main(I32 argc, char **argv){

	freopen(OUTPUT_LOG_FILE, "w", stdout);
	freopen(ERROR_LOG_FILE, "w", stderr);
	///Initialize our application based on XML configuration
	Application::getInstance().Initialize("main.xml");  
	///When the main loop returns, destroy the application
	Application::getInstance().run();
	///Stop our application
	Application::getInstance().Deinitialize();  
	///When the application is deleted, the last kernel used gets deleted as well
	Application::getInstance().DestroyInstance();  
	PRINT_FN("Application shutdown successfull!");
	return 0;
}

