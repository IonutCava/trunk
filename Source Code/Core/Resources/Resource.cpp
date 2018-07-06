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
    char zMaxFile[256];
    I16 nMaxLine = 0;
};


void log_new(size_t t, const char* zFile, I32 nLine){
    if (t > Memory::maxAlloc)	{
        Memory::maxAlloc = (U32)t;
        strcpy(Memory::zMaxFile, zFile);
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
void* operator new[]( size_t size,char* zFile, int nLine ){
    Divide::log_new(size ,zFile, nLine);
    return malloc (size);
}

void  operator delete[]( void *ptr,char* zFile, int nLine ){
    free(ptr);
}

void* operator new(size_t size ,char* zFile, int nLine){
    return malloc (size);
}

void  operator delete( void *ptr, char* zFile, int nLine){
    free(ptr);
}
#endif

void* operator new[](size_t size, const char* pName, int flags, unsigned int debugFlags, const char* file, int line)
{
    //Divide::log_new(size ,file, line);
    return malloc(size);
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned int debugFlags, const char* file, int line)
{
    //Divide::log_new(size ,file, line);
    // this allocator doesn't support alignment
    assert(alignment <= 8);
    return malloc(size);
}
// E
int Vsnprintf8(char* pDestination, size_t n, const char* pFormat, va_list arguments)
{
    #ifdef _MSC_VER
    return _vsnprintf(pDestination, n, pFormat, arguments);
    #else
    return vsnprintf(pDestination, n, pFormat, arguments);
    #endif
}
