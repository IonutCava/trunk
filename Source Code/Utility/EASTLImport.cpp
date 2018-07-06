#include "stdafx.h"

#include "Platform/Headers/PlatformDefines.h"

#include <EASTL/src/allocator_eastl.cpp>
#include <EASTL/src/assert.cpp>
#include <EASTL/src/fixed_pool.cpp>
#include <EASTL/src/hashtable.cpp>
#include <EASTL/src/intrusive_list.cpp>
#include <EASTL/src/numeric_limits.cpp>
#include <EASTL/src/red_black_tree.cpp>
#include <EASTL/src/string.cpp>
#include <EASTL/src/thread_support.cpp>
#include <assert.h>

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset,
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

void* operator new[](size_t size, const char* pName, int flags,
                     unsigned int debugFlags, const char* file, int line) {
    ACKNOWLEDGE_UNUSED(pName);
    ACKNOWLEDGE_UNUSED(flags);
    ACKNOWLEDGE_UNUSED(debugFlags);
    ACKNOWLEDGE_UNUSED(file);
    ACKNOWLEDGE_UNUSED(line);

    return malloc(size);
}

int Vsnprintf8(char* pDestination, size_t n, const char* pFormat,
               va_list arguments) {
    return vsnprintf(pDestination, n, pFormat, arguments);
}
