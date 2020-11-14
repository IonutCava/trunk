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

#include <EASTL/allocator.h>
#include <Allocator/stl_allocator.h>
template <typename Type>
using dvd_allocator = stl_allocator<Type>;

namespace eastl {
    struct dvd_eastl_allocator
    {
        dvd_eastl_allocator() = default;
        dvd_eastl_allocator(const char* pName) noexcept { (void)pName; }
        dvd_eastl_allocator(const allocator& x, const char* pName) noexcept { (void)x;  (void)pName; }
        dvd_eastl_allocator& operator=(const dvd_eastl_allocator& EASTL_NAME(x)) = default;

        [[nodiscard]] void* allocate(const size_t n, int flags = 0)
        {
            (void)flags;
            return xmalloc(n);
        }

        [[nodiscard]] void* allocate(const size_t n, size_t alignment, size_t offset, int flags = 0)
        {
            (void)flags; (void)offset; (void)alignment;
            return xmalloc(n);
        }

        void deallocate(void* p, size_t n)
        {
            (void)n;
            //delete[](char*)p;
            xfree(p);
        }

        [[nodiscard]]
        const char* get_name()                  const noexcept { return "dvd custom eastl allocator"; }
              void  set_name(const char* pName)       noexcept { (void)pName; }
    };


    // All allocators are considered equal, as they merely use global new/delete.
    [[nodiscard]] inline bool operator==(const dvd_eastl_allocator& a, const dvd_eastl_allocator& b) noexcept
    {
        (void)a; (void)b;
        return true;
    }

    [[nodiscard]] inline bool operator!=(const dvd_eastl_allocator& a, const dvd_eastl_allocator& b) noexcept
    {
        (void)a; (void)b;
        return false;
    }
}
#endif //_TEMPLATE_ALLOCATOR_H_
