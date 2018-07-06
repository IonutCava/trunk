#include "Headers/Kernel.h"
#include "Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Hardware/Video/Headers/GFXDevice.h"

namespace Divide {

Application::Application() : _kernel(nullptr),
                             _totalMemoryOcuppied(0),
                             _requestShutdown(false),
                             _hasFocus(true),
                             _mainLoopActive(false),
                             _mainLoopPaused(false)
{
    _threadId = boost::this_thread::get_id();
    _errorCode = NO_ERR;
    ParamHandler::createInstance();
    Console::createInstance();
    ApplicationTimer::createInstance();
}

Application::~Application(){
    ParamHandler::destroyInstance();
    PRINT_FN(Locale::get("STOP_KERNEL"));
    SAFE_DELETE(_kernel);
    PRINT_FN(Locale::get("STOP_APPLICATION"));
    Console::destroyInstance();
    ApplicationTimer::destroyInstance();
}

ErrorCode Application::initialize(const stringImpl& entryPoint, I32 argc, char **argv){
    assert(!entryPoint.empty());
    //Read language table
    ParamHandler::getInstance().setDebugOutput(false);
    //Print a copyright notice in the log file
    Console::getInstance().printCopyrightNotice();
    CONSOLE_TIMESTAMP_ON();
    PRINT_FN(Locale::get("START_APPLICATION"));
    //Create a new kernel
    _kernel = New Kernel(argc,argv,this->getInstance());
    assert(_kernel != nullptr);
    //and load it via an XML file config
    return _kernel->initialize(entryPoint);
}

void Application::run(){
    _kernel->runLogicLoop();
}

void Application::snapCursorToPosition(U16 x, U16 y) const {
    _kernel->setCursorPosition(x, y);
}

void Application::deinitialize(){
    if(_totalMemoryOcuppied != 0)
        ERROR_FN(Locale::get("ERROR_MEMORY_NEW_DELETE_MISMATCH"), _totalMemoryOcuppied);
}

void Application::logMemoryOperation(bool allocation, const char* logMsg, size_t size)  {
    if(_memLogBuffer.is_open())
        _memLogBuffer << logMsg;

    _totalMemoryOcuppied += size * (allocation ? 1 : -1);
}

};