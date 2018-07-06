#include "config.h"
#include "Rendering/Framerate.h"
#include "Utility/Headers/Guardian.h"
#include "Utility/Headers/ParamHandler.h"
using namespace std;

/* Entry point */
//GLEWContext _renderingContext;
//GLEWContext* glewGetContext() {return &_renderingContext; }

void main()
{
	//Target FPS is 60. So all movement is capped around that value
	freopen(OUTPUT_LOG_FILE, "w", stdout);
	freopen(OUTPUT_LOG_FILE, "w", stderr);
	Framerate::getInstance().Init(60);
	ParamHandler::getInstance().setDebugOutput(false);
	Guardian::getInstance().LoadApplication("main.xml");   
	return;
}

