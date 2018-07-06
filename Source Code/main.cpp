#include "config.h"
#include "Utility/Headers/Guardian.h"
#include "Utility/Headers/ParamHandler.h"
using namespace std;

/* Entry point */
//GLEWContext _renderingContext;
//GLEWContext* glewGetContext() {return &_renderingContext; }
#ifdef main
#undef main
#endif 

I32 main(I32 argc, char **argv)
{
	//Target FPS is 60. So all movement is capped around that value
	freopen(OUTPUT_LOG_FILE, "w", stdout);
	freopen(OUTPUT_LOG_FILE, "w", stderr);
	ParamHandler::getInstance().setDebugOutput(false);
	Guardian::getInstance().LoadApplication("main.xml");   
	return 0;
}

