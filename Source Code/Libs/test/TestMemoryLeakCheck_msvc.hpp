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

#ifndef OCL_GUARD_TEST_TESTMEMORYLEAKCHECK_MSVC_HPP
#define OCL_GUARD_TEST_TESTMEMORYLEAKCHECK_MSVC_HPP

// Member functions for class TestMemoryLeakCheck.
// This is specific for Microsoft compilers.

#ifdef TEST_LEAK_DETECT

#ifdef _DEBUG

#ifndef _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#endif

#include <stdlib.h>
#include <crtdbg.h>
#include <cstddef>
#include "TestMemoryCounter.hpp"

// Need to replace new for Microsoft leak detection to list filenames leaking memory.
#ifndef OCL_TEST_DBG_NEW
    #define OCL_TEST_DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
    #define new OCL_TEST_DBG_NEW
#endif

namespace ocl
{

struct TestMemoryLeakCheck
{
    TestMemoryLeakCheck()
        : m_is_leaking(false)
        , m_started(false)
        , m_stopped(false)
    {
    }

    ~TestMemoryLeakCheck()
    {
        if (m_started && !m_stopped)
            Stop(); // Always ensure to match a start with a stop.
    }

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

    bool Start()
    {
        // Only allow start if it's not been started,
        // or both start and stop have been called.
        if (!m_started || m_stopped)
        {
            EnableCounting(true);

            m_started = true;
            m_stopped = false;
            m_is_leaking = false;

            if (!InitialisedState())
            {
                // Only need to do this once for all unit tests.
                InitialisedState() = true;
                _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);

                _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE|_CRTDBG_MODE_DEBUG);
                _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
                _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE|_CRTDBG_MODE_DEBUG);
                _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
                _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE|_CRTDBG_MODE_DEBUG);
                _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
            }

            _CrtMemCheckpoint(&m_state_start);
            return true;
        }
        return false;
    }

    // return true if a memory leak is detected.
    bool Stop()
    {
        if (m_started)
        {
            EnableCounting(false);

            m_stopped = true;

            _CrtMemState state_now;
            _CrtMemCheckpoint(&state_now);
            m_is_leaking = _CrtMemDifference(&m_state_diff, &m_state_start, &state_now) != 0;
            return true;
        }
        return false;
    }

    bool IsLeaking() const
    {
        return m_is_leaking;
    }

    // Dump memory leak state for start/stop snapshot.
    void Dump()
    {
        _CrtMemDumpStatistics(&m_state_diff);
    }

    // Dump a summary of all memory leaks for all tests.
    void DumpAll()
    {
        _CrtDumpMemoryLeaks();

        // Already reported leaks, so prevent Visual Studio reporting them again.
        _CrtSetDbgFlag(0);
    }

    template<typename T>
    static void BreakOn(T break_value)
    {
    // NOTE: If _CrtSetBreakAlloc does not work place a break point in file:
    //       C:\Program Files (x86)\Windows Kits\10\Source\10.0.10240.0\ucrt\heap\debug_heap.cpp
    // Look for line containing:
    //       if (_crtBreakAlloc != -1 && request_number == _crtBreakAlloc)
    // Older versions might have variable called lRequest instead of request_number.
        _CrtSetBreakAlloc(break_value);
    }

private:
    bool& InitialisedState()
    {
        static bool is_initialised = false;
        return is_initialised;
    }

    _CrtMemState m_state_start;
    _CrtMemState m_state_diff;
    bool m_is_leaking;
    bool m_started;
    bool m_stopped;
};

} // namespace ocl

#else
#include "TestMemoryLeakCheck_stub.hpp"
#endif // _DEBUG

#else // TEST_LEAK_DETECT

#ifdef _DEBUG

// If macro TEST_LEAK_DETECT has been manually undefined for a source file
// then restore the original global new operator.
#ifdef OCL_TEST_DBG_NEW
#undef new
#define new OCL_TEST_DBG_NEW
#undef OCL_TEST_DBG_NEW
#endif // OCL_TEST_DBG_NEW

#endif // _DEBUG

// Need stubs for release as TestClass doesn't check for TEST_LEAK_DETECT define.
#include "TestMemoryLeakCheck_stub.hpp"

#endif // TEST_LEAK_DETECT

#endif // OCL_GUARD_TEST_TESTMEMORYLEAKCHECK_MSVC_HPP
