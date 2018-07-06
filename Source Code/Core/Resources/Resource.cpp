#include "Headers/Resource.h"

#include "core.h"
#include "Core/Headers/Application.h"


void Resource::refModifyCallback(bool increase){
    TrackedObject::refModifyCallback(increase);
}

namespace Divide {
    namespace Memory {
        char outputLogBuffer[512];
        U32 maxAlloc = 0;
        char* zMaxFile = "";
        I16 nMaxLine = 0;
    };
};

#ifndef NDEBUG

void log_new(size_t t ,char* zFile, I32 nLine){
    if (t > Divide::Memory::maxAlloc)	{
        Divide::Memory::maxAlloc = (U32)t;
        Divide::Memory::zMaxFile = zFile;
        Divide::Memory::nMaxLine = nLine;
    }

    sprintf(Divide::Memory::outputLogBuffer,
            "[ %9.2f ] : New allocation [ %d ] in: \"%s\" at line: %d.\n\t Max allocation: [ %d ] in file \"%s\" at line: %d\n\n",
            GETTIME(), t, zFile, nLine, Divide::Memory::maxAlloc, Divide::Memory::zMaxFile, Divide::Memory::nMaxLine);

    Application::getInstance().logMemoryOperation(true, Divide::Memory::outputLogBuffer, t);
}

void log_delete(size_t t,char* zFile, I32 nLine){

    sprintf(Divide::Memory::outputLogBuffer,
            "[ %9.2f ] : New deallocation [ %d ] in: \"%s\" at line: %d.\n\n",
            GETTIME(), t, zFile, nLine);
    Application::getInstance().logMemoryOperation(false, Divide::Memory::outputLogBuffer, t);
}

void* operator new[]( size_t t,char* zFile, int nLine ){
    log_new(t ,zFile, nLine);
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

void * malloc_simd(const size_t bytes) {
#if defined(HAVE_ALIGNED_MALLOC)
#   if defined(__GNUC__)
        return aligned_malloc(bytes,ALIGNED_BYTES);
#   else
        return _aligned_malloc(bytes,ALIGNED_BYTES);
#   endif
#elif defined(HAVE_POSIX_MEMALIGN)
    void *ptr=nullptr;
    I32 ret = posix_memalign(&ptr,ALIGNED_BYTES,bytes);
    if (ret) THROW_EXCEPTION("posix_memalign returned an error.");
    return ptr;
#else
    // We don't have aligned memory:
    return ::malloc(bytes);
#endif
}

void free_simd(void * pxData){
#if defined(HAVE_ALIGNED_MALLOC)
#   if defined(__GNUC__)
    aligned_free(pxData);
#   else
    _aligned_free(pxData);
#   endif
#else
    free(p);
#endif
}