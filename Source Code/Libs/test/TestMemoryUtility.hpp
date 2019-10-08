/*
Copyright 2016 Colin Girling

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef OCL_GUARD_TEST_TESTMEMORYUTILITY_HPP
#define OCL_GUARD_TEST_TESTMEMORYUTILITY_HPP

#include <cstddef>
#include <cstdlib>
#include <cstring>

// temp includes
#include <iostream>
#include <cstdio>

#include "TestMemoryLeakCheck.hpp"

namespace ocl
{

template<typename Type, typename SizeType = size_t>
struct TestMemoryUtility
{
    static SizeType const SIZE_IN_BYTES = static_cast<SizeType>(sizeof(Type));

    static TestMemoryCounter& GetCounter()
    {
        return TestMemoryLeakCheck::GetCounter();
    }

    static Type* Allocate(SizeType elements)
    {
        GetCounter().Inc();
        SizeType const alloc_size_in_bytes = elements * sizeof(Type);
        Type* ptr = static_cast<Type*>(::malloc(static_cast<size_t>(alloc_size_in_bytes)));
        return ptr;
    }

    static void Allocate(Type*& dest, SizeType elements)
    {
        dest = Allocate(elements);
    }

    /// Free the memory but does not set ptr to NULL or return the pointer.
    /// Useful in destructor(s) or other out of scope situations.
    static void FastFree(Type* ptr)
    {
        if (ptr != NULL)
            GetCounter().Dec();
        ::free(ptr);
    }

    static void SafeFree(Type*& ptr)
    {
        FastFree(ptr);
        ptr = NULL;
    }

    /// Does not check ptr for NULL, but will return NULL if size is 0.
    static Type* UnsafeAllocateCopy(Type const* ptr, SizeType elements)
    {
        if (elements > 0)
        {
            Type* dest = Allocate(elements);
            if (dest != NULL)
                ::memcpy(dest, ptr, static_cast<size_t>(elements * SIZE_IN_BYTES));
            return dest;
        }

        return NULL;
    }

    static void UnsafeAllocateCopy(Type*& dest, Type const* src, SizeType size)
    {
        dest = UnsafeAllocateCopy(src, size);
    }

    static Type* SafeAllocateCopy(Type const* ptr, SizeType size)
    {
        return (ptr != NULL) ? UnsafeAllocateCopy(ptr, size) : NULL;
    }

    static void SafeAllocateCopy(Type*& dest, Type const* src, SizeType size)
    {
        dest = SafeAllocateCopy(src, size);
    }
};

} // namespace ocl

#endif // OCL_GUARD_TEST_TESTMEMORYUTILITY_HPP
