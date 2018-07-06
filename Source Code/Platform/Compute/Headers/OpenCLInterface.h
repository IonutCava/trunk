/*
Copyright (c) 2018 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#pragma once
#ifndef _OPENCL_INTERFACE_H_
#define _OPENCL_INTERFACE_H_

#include "Core/Headers/Application.h"

//ref: https://github.com/morousg/simple-opencl
namespace Divide {

class OpenCLProgram {
public:
    ~OpenCLProgram();

    // If enque == false, a CL flush command is issued after the enque to launch the kernel immediately
    template <typename... T>
    cl_event run(const sclHard& hardware, 
                 bool enque,
                 size_t *globalWorkSize,
                 size_t *localWorkSize,
                 const char* sizesValues,
                 T&&... args) const;

private:
    explicit OpenCLProgram(const sclSoft& program);

private:
    friend class OpenCLInterface;
    const sclSoft& _program;
};

/// OpenCL implementation
DEFINE_SINGLETON(OpenCLInterface)

public:
    ErrorCode init();
    bool deinit();

    cl_int flushHardware(I32 deviceIndex = -1) const;
    
    template <typename... T>
    cl_event runProgram(const OpenCLProgram& clProgram,
                        bool enque,
                        I32 deviceIndex,
                        size_t *globalWorkSize,
                        size_t *localWorkSize,
                        const char* sizesValues,
                        T&&... args);

    void setKernelArg(const OpenCLProgram& clProgram, I32 argnum, size_t typeSize, void *argument);
    template <typename... T>
    void setKernelArgs(const OpenCLProgram& clProgram, const char *sizesValues, T&&... args);

    cl_ulong getEventTime(cl_event event, I32 deviceIndex = -1) const;

    cl_mem 	malloc(cl_int mode, size_t size, I32 deviceIndex = -1) const;
    cl_mem  mallocWrite(cl_int mode, size_t size, void* hostPointer, I32 deviceIndex = -1) const;
    void write(size_t size, cl_mem buffer, void* hostPointer, I32 deviceIndex = -1) const;
    void read(size_t size, cl_mem buffer, void *hostPointer, I32 deviceIndex = -1) const;
    void release(cl_mem object) const;

private:
    OpenCLInterface();
    ~OpenCLInterface();

    const sclHard& getHardwareByIndex(I32 deviceIndex) const;
    OpenCLProgram loadProgram(const stringImpl& programPath, const stringImpl& programName, I32 deviceIndex = -1);
    
private:
    sclHard* _hardware = nullptr;
    sclHard  _fastestDevice = {};
    I32      _deviceCount = 0;

END_SINGLETON

template <typename... T>
void OpenCLInterface::setKernelArgs(const OpenCLProgram& clProgram, const char *sizesValues, T&&... args) {
    sclSetKernelArgs(clProgram._program, sizeValues, std::forward<T>(args)...);
}

template <typename... T>
cl_event OpenCLInterface::runProgram(const OpenCLProgram& clProgram,
                                     bool enque,
                                     I32 deviceIndex,
                                     size_t *globalWorkSize,
                                     size_t *localWorkSize,
                                     const char* sizesValues,
                                     T&&... args)
{
    return clProgram.run(getHardwareByIndex(deviceIndex),
                         enque,
                         globalWorkSize,
                         localWorkSize,
                         sizesValues,
                         std::forward<T>(args)...);
}

template <typename... T>
cl_event OpenCLProgram::run(const sclHard& hardware,
                            bool enque,
                            size_t *globalWorkSize,
                            size_t *localWorkSize,
                            const char* sizesValues,
                            T&&... args) const {
    
    cl_event returnValue;
    if (sizeValues == nullptr) {
        if (enque) {
            returnValue = sclEnqueueKernel(hardware, _program, globalWorkSize, localWorkSize);
        } else {
            returnValue = sclLaunchKernel(hardware, _program, globalWorkSize, localWorkSize);
        }
    } else {
        if (enque) {
            returnValue = sclSetArgsEnqueueKernel(hardware, _program, globalWorkSize, localWorkSize, sizesValues, std::forward<T>(args));
        } else {
            returnValue = sclSetArgsLaunchKernel(hardware, _program, globalWorkSize, localWorkSize, sizesValues, std::forward<T>(args));
        }
    }

    return returnValue;
}

}; //namespace Divide

#endif //_OPENCL_INTERFACE_H_
