#include "stdafx.h"

#include "Headers/OpenCLInterface.h"
#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

void clPrint(FILE* target, const char* format, ...) {
    thread_local static char textBuffer[512 + 1];
    va_list args;
    va_start(args, format);
    assert(_vscprintf(format, args) + 1 < 512);
    vsprintf(textBuffer, format, args);
    va_end(args);
    if (textBuffer[0] == '\n') {
        textBuffer[0] = ' ';
    }
    Divide::Console::printf("--------- ");
    target == stderr ? Divide::Console::errorfn(textBuffer)
                     : Divide::Console::printfn(textBuffer);
}

namespace Divide {

OpenCLInterface::OpenCLInterface() : _hardware(nullptr),
                                     _deviceCount(0)
{
}

OpenCLInterface::~OpenCLInterface()
{
}

ErrorCode OpenCLInterface::init() {
    Console::printfn(Locale::get(_ID("START_OPENCL_BEGIN")));
    bool timestamps = Console::timeStampsEnabled();
    bool threadid = Console::threadIDEnabled();
    Console::toggleTimeStamps(false);
    Console::togglethreadID(false);
    // Get the hardware
    _hardware = sclGetAllHardware(&_deviceCount);
    _fastestDevice = sclGetFastestDevice(_hardware, _deviceCount);
    for (I32 i = 0; i < _deviceCount; ++i) {
        sclPrintHardwareStatus(_hardware[i]);
    }
    sclPrintDeviceNamePlatforms(_hardware, _deviceCount);
    Console::togglethreadID(threadid);
    Console::toggleTimeStamps(timestamps);
    Console::printfn(Locale::get(_ID("START_OPENCL_END")));
    

    return _deviceCount > 0 ? ErrorCode::NO_ERR : ErrorCode::OCL_INIT_ERROR;
}

bool OpenCLInterface::deinit() {
    Console::printfn(Locale::get(_ID("STOP_OPENCL")));
    sclReleaseAllHardware(_hardware, _deviceCount);
    
    return _deviceCount > 0;
}

OpenCLProgram OpenCLInterface::loadProgram(const stringImpl& programPath, const stringImpl& programName, I32 deviceIndex) {
    // Get the software
    return OpenCLProgram(sclGetCLSoftware((char*)(programPath.c_str()),
                                          (char*)(programName.c_str()),
                                          getHardwareByIndex(deviceIndex)));
}

void OpenCLInterface::setKernelArg(const OpenCLProgram& clProgram, I32 argnum, size_t typeSize, void *argument) {
    sclSetKernelArg(clProgram._program, argnum, typeSize, argument);
}

const sclHard& OpenCLInterface::getHardwareByIndex(I32 deviceIndex) const {
    return deviceIndex == -1 ? _fastestDevice :
                               _hardware[std::max(std::min(deviceIndex, _deviceCount), 0)];
}

cl_int OpenCLInterface::flushHardware(I32 deviceIndex) const {
    return sclFinish(getHardwareByIndex(deviceIndex));
}

cl_ulong OpenCLInterface::getEventTime(cl_event event, I32 deviceIndex) const {
    return sclGetEventTime(getHardwareByIndex(deviceIndex), event);
}

cl_mem OpenCLInterface::malloc(cl_int mode, size_t size, I32 deviceIndex) const {
    return sclMalloc(getHardwareByIndex(deviceIndex), mode, size);
}

cl_mem OpenCLInterface::mallocWrite(cl_int mode, size_t size, void* hostPointer, I32 deviceIndex) const {
    return sclMallocWrite(getHardwareByIndex(deviceIndex), mode, size, hostPointer);
}

void OpenCLInterface::write(size_t size, cl_mem buffer, void* hostPointer, I32 deviceIndex) const {
    sclWrite(getHardwareByIndex(deviceIndex), size, buffer, hostPointer);
}

void OpenCLInterface::read(size_t size, cl_mem buffer, void *hostPointer, I32 deviceIndex) const {
    sclRead(getHardwareByIndex(deviceIndex), size, buffer, hostPointer);
}

void OpenCLInterface::release(cl_mem object) const {
    sclReleaseMemObject(object);
}

OpenCLProgram::OpenCLProgram(const sclSoft& program) : _program(program)
{
}

OpenCLProgram::~OpenCLProgram()
{
    sclReleaseClSoft(_program);
}

}; //namespace Divide