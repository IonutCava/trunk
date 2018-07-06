#include "Headers/Kernel.h"
#include "Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Hardware/Video/Headers/GFXDevice.h"

Application::Application() : _kernel(NULL),
							 _mainWindowId(-1),
							 _requestShutdown(false)
{
    _threadId = boost::this_thread::get_id();
	ParamHandler::createInstance();
	Console::createInstance();
	Framerate::createInstance();
}

Application::~Application(){
	ParamHandler::DestroyInstance();
	PRINT_FN(Locale::get("STOP_KERNEL"));
	SAFE_DELETE(_kernel);
	PRINT_FN(Locale::get("STOP_APPLICATION"));
	Console::DestroyInstance();
	Framerate::DestroyInstance();
}

I8 Application::Initialize(const std::string& entryPoint,I32 argc, char **argv){
	assert(!entryPoint.empty());
	//Read language table
	ParamHandler::getInstance().setDebugOutput(false);
	//Print a copyright notice in the log file
	Console::getInstance().printCopyrightNotice();
    CONSOLE_TIMESTAMP_ON();
	PRINT_FN(Locale::get("START_APPLICATION"));
	//Create a new kernel
	_kernel = New Kernel(argc,argv,this->getInstance());
	assert(_kernel != NULL);
	//and load it via an XML file config
	_mainWindowId = _kernel->Initialize(entryPoint);
	return _mainWindowId;
}

void Application::setMousePosition(D32 x, D32 y) const {
	_kernel->getGFXDevice().setMousePosition(x,y);
}

void Application::run(){
	_kernel->beginLogicLoop();
}

void Application::Deinitialize(){
	_kernel->Shutdown();
}