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
#ifndef _TEMPLATE_ALLOCATOR_H_
#define _TEMPLATE_ALLOCATOR_H_

#include "config.h"
#include <EASTL/allocator.h>
#include <Allocator/stl_allocator.h>
template <typename Type>
using dvd_allocator = stl_allocator<Type>;


namespace eastl {
    class dvd_eastl_allocator
    {
    public:
        inline dvd_eastl_allocator(const char* pName = EASTL_NAME_VAL("dvd custom eastl allocator"))
        {
        #if EASTL_NAME_ENABLED
            mpName = pName ? pName : EASTL_ALLOCATOR_DEFAULT_NAME;
        #else
            (void)pName;
        #endif
        }

        inline dvd_eastl_allocator(const allocator& x, const char* pName = EASTL_NAME_VAL("dvd custom eastl allocator"))
        {
            (void)x;
        #if EASTL_NAME_ENABLED
            mpName = pName ? pName : EASTL_ALLOCATOR_DEFAULT_NAME;
        #else
            (void)pName;
        #endif
        }

        inline dvd_eastl_allocator& operator=(const dvd_eastl_allocator& EASTL_NAME(x))
        {
            #if EASTL_NAME_ENABLED
                mpName = x.mpName;
            #endif

            return *this;
        }

        inline void* allocate(size_t n, int flags = 0)
        {
            #if EASTL_NAME_ENABLED
                #define pName mpName
            #else
                #define pName EASTL_ALLOCATOR_DEFAULT_NAME
            #endif

            //return ::new((char*)0, flags, 0, (char*)0, 0) char[n];
            return xmalloc(n);
        }

        inline void* allocate(size_t n, size_t alignment, size_t offset, int flags = 0)
        {
            return xmalloc(n);
            //return ::new(alignment, offset, (char*)0, flags, 0, (char*)0, 0) char[n];
            #undef pName  // See above for the definition of this.
        }

        inline void deallocate(void* p, size_t n)
        {
            (void)n;
            //delete[](char*)p;
            xfree(p);
        }

        inline const char* get_name() const
        {
        #if EASTL_NAME_ENABLED
            return mpName;
        #else
            return "dvd custom eastl allocator";
        #endif
        }

        inline void set_name(const char* pName)
        {
        #if EASTL_NAME_ENABLED
            mpName = pName;
        #else
            (void)pName;
        #endif
        }

    protected:
        // Possibly place instance data here.

#if EASTL_NAME_ENABLED
        const char* mpName; // Debug name, used to track memory.
#endif
    };


    inline bool operator==(const dvd_eastl_allocator& a, const dvd_eastl_allocator& b)
    {
        (void)a; (void)b;
        return true; // All allocators are considered equal, as they merely use global new/delete.
    }

    inline bool operator!=(const dvd_eastl_allocator& a, const dvd_eastl_allocator& b)
    {
        (void)a; (void)b;
        return false; // All allocators are considered equal, as they merely use global new/delete.
    }
}
#endif //_TEMPLATE_ALLOCATOR_H_
