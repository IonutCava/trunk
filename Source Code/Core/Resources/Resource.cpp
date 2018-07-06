#include "Headers/Resource.h"

#include "core.h"
#include "Core/Headers/Application.h"

namespace Divide {

void Resource::refModifyCallback(bool increase){
    TrackedObject::refModifyCallback(increase);
}

namespace Memory {
    char outputLogBuffer[512];
    U32 maxAlloc = 0;
    char* zMaxFile = "";
    I16 nMaxLine = 0;
};


void log_new(size_t t ,char* zFile, I32 nLine){
    if (t > Memory::maxAlloc)	{
        Memory::maxAlloc = (U32)t;
        Memory::zMaxFile = zFile;
        Memory::nMaxLine = nLine;
    }

    sprintf(Memory::outputLogBuffer,
            "[ %9.2f ] : New allocation [ %d ] in: \"%s\" at line: %d.\n\t Max allocation: [ %d ] in file \"%s\" at line: %d\n\n",
            GETTIME(), t, zFile, nLine, Memory::maxAlloc, Memory::zMaxFile, Memory::nMaxLine);

    Application::getInstance().logMemoryOperation(true, Memory::outputLogBuffer, t);
}

void log_delete(size_t t,char* zFile, I32 nLine){

    sprintf(Memory::outputLogBuffer,
            "[ %9.2f ] : New deallocation [ %d ] in: \"%s\" at line: %d.\n\n",
            GETTIME(), t, zFile, nLine);
    Application::getInstance().logMemoryOperation(false, Memory::outputLogBuffer, t);
}

}; //namespace Divide

#if !defined(NDEBUG)
void* operator new[]( size_t t,char* zFile, int nLine ){
    Divide::log_new(t ,zFile, nLine);
    return malloc (t);
}

void  operator delete[]( void *p,char* zFile, int nLine ){
    free(p);
}

void* operator new(size_t t ,char* zFile, int nLine){
    return malloc (t);
}

void  operator delete( void *p, char* zFile, int nLine){
    free(p);
}

#endif