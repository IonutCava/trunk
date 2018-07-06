#include "Headers/Kernel.h"
#include "Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Utility/Headers/MemoryTracker.h"
#include "Hardware/Video/Headers/GFXDevice.h"

namespace Divide {

#if defined(_DEBUG)
    bool MemoryManager::MemoryTracker::Ready = false;
    MemoryManager::MemoryTracker MemoryManager::AllocTracer;
#endif

Application::Application() : _kernel(nullptr),
                             _hasFocus(true)
{
	//MemoryTracker::Ready = false; //< faster way of disabling memory tracking
	_requestShutdown = false;
	_mainLoopActive = false;
	_mainLoopPaused = false;
    _threadId = std::this_thread::get_id();
    _errorCode = NO_ERR;
    ParamHandler::createInstance();
    Console::createInstance();
    ApplicationTimer::createInstance();
}

Application::~Application(){
#if defined(_DEBUG)
    MemoryManager::MemoryTracker::Ready = false;
	bool leakDetected = false;
	size_t sizeLeaked = 0;
    stringImpl allocLog = MemoryManager::AllocTracer.Dump( leakDetected, sizeLeaked );
	if ( leakDetected ) {
		ERROR_FN( Locale::get( "ERROR_MEMORY_NEW_DELETE_MISMATCH" ), static_cast<I32>(std::ceil(sizeLeaked / 1024.0f)) );
	}
	std::ofstream memLog;
	memLog.open( _memLogBuffer.c_str() );
	memLog << allocLog.c_str();
	memLog.close();
#endif
	PRINT_FN( Locale::get( "STOP_APPLICATION" ) );
	ParamHandler::destroyInstance();
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

void Application::deinitialize() {
	PRINT_FN( Locale::get( "STOP_KERNEL" ) );
    MemoryManager::SAFE_DELETE( _kernel );
	for ( DELEGATE_CBK<>& cbk : _shutdownCallback ) {
		cbk();
	}
}

};