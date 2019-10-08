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

#ifndef OCL_GUARD_TEST_TESTMEMORYLEAKCHECK_STUB_HPP
#define OCL_GUARD_TEST_TESTMEMORYLEAKCHECK_STUB_HPP

#include <cstddef>
#include "TestMemoryCounter.hpp"

namespace ocl
{

// Implementation for non-Windows operating systems and compilers.
struct TestMemoryLeakCheck
{
    TestMemoryLeakCheck() {}
    ~TestMemoryLeakCheck() {}

    static TestMemoryCounter& GetCounter()
    {
        static TestMemoryCounter counter;
        return counter;
    }

    static void EnableCounting(bool enabled)
    {
        // Track when allocating and deallocating to detect too many deallocations
        // within a start and end checkpoint.
        GetCounter().Enable(enabled);
    }

    // Start or stop snapshot memory leak detection.
    bool Start() { return true; }
    bool Stop() { return true; }

    bool IsLeaking() const { return false; }

    // Dump memory leak state for start/stop snapshot.
    void Dump() {}

    // Dump a summary of all memory leaks for all tests.
    void DumpAll() {}

    template<typename T>
    void BreakOn(T /*break_value*/) {}
};

} // namespace ocl

#endif // OCL_GUARD_TEST_TESTMEMORYLEAKCHECK_STUB_HPP
