#include "stdafx.h"

#include "Platform/Headers/PlatformDefines.h"


void* operator new[](const size_t size, size_t alignment, size_t alignmentOffset,
                     const char* pName, int flags, unsigned int debugFlags,
                     const char* file, int line) {
    ACKNOWLEDGE_UNUSED(alignmentOffset);
    ACKNOWLEDGE_UNUSED(pName);
    ACKNOWLEDGE_UNUSED(flags);
    ACKNOWLEDGE_UNUSED(debugFlags);
    ACKNOWLEDGE_UNUSED(file);
    ACKNOWLEDGE_UNUSED(line);

    // this allocator doesn't support alignment
    assert(alignment <= 8);
    ACKNOWLEDGE_UNUSED(alignment);

    return malloc(size);
}

void* operator new[](const size_t size, const char* pName, int flags,
                     unsigned int debugFlags, const char* file, int line) {
    ACKNOWLEDGE_UNUSED(pName);
    ACKNOWLEDGE_UNUSED(flags);
    ACKNOWLEDGE_UNUSED(debugFlags);
    ACKNOWLEDGE_UNUSED(file);
    ACKNOWLEDGE_UNUSED(line);

    return malloc(size);
}

int Vsnprintf8(char* pDestination, const size_t n, const char* pFormat, va_list const arguments) {
    return vsnprintf(pDestination, n, pFormat, arguments);
}
