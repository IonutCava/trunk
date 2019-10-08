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

#ifndef OCL_GUARD_TEST_TESTMEMORYLEAKCHECK_HPP
#define OCL_GUARD_TEST_TESTMEMORYLEAKCHECK_HPP

/*
// Expected interface:
struct TestMemoryLeakCheck
{
    // Detect leaks or too many deletes.
    static TestMemoryCounter& GetCounter();
    static void EnableCounting(bool enabled);

    bool Start();           // return false if already started.
    bool Stop();            // return false if not started.
    bool IsLeaking() const;
    void Dump();            // dump memory leaks for individual unit test.
    void DumpAll();         // dump all memory leaks.

    template<typename T>
    void BreakOn(T break_value); // break for specific compiler and value.
};
*/

#if defined(_MSC_VER)

// Microsoft compiler functions.
#include "TestMemoryLeakCheck_msvc.hpp"

#else // not _MSC_VER

// Non-Microsoft implementation. (stub does nothing)
#include "TestMemoryLeakCheck_stub.hpp"

#endif // defined(_MSC_VER)

#endif // OCL_GUARD_TEST_TESTMEMORYLEAKCHECK_HPP
