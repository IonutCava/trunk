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

#ifndef OCL_GUARD_TEST_TESTMEMORYCOUNTER_HPP
#define OCL_GUARD_TEST_TESTMEMORYCOUNTER_HPP

#include <cstddef>

namespace ocl
{

/// Count allocation and de-allocation of pointers,
/// with support for enabling or disabling tracking.
class TestMemoryCounter
{
public:
    TestMemoryCounter()
        : m_count(0)
        , m_max_count(0)
        , m_count_enabled(false)
        , m_global_count(0)
        , m_max_global_count(0)
    {
    }

    void Enable(bool enabled = true)
    {
        m_count_enabled = enabled;
        if (!enabled)
            m_count = m_max_count = 0;
    }

    bool IsEnabled() const
    {
        return m_count_enabled;
    }

    void Inc()
    {
        if (m_count_enabled)
        {
            ++m_count;
            if (m_count > m_max_count)
                m_max_count = m_count;
        }
        ++m_global_count;
        if (m_global_count > m_max_global_count)
            m_max_global_count = m_global_count;
    }

    // return false if the count is already zero.
    bool Dec()
    {
        if (m_count_enabled)
        {
            if (m_count == 0)
                return false; // Too many frees detected!
            --m_count;
        }
        if (m_global_count == 0)
            return false; // Too many frees detected!
        --m_global_count;
        
        return true; // Allocations and frees match.
    }

    unsigned long GetCount() const
    {
        return m_count;
    }

    unsigned long GetMaxCount() const
    {
        return m_max_count;
    }

    unsigned long GetGlobalCount() const
    {
        return m_global_count;
    }

    unsigned long GetMaxGlobalCount() const
    {
        return m_max_global_count;
    }

    bool IsCountEnabled() const
    {
        return m_count_enabled;
    }

private:
    // Increase for allocations and decrease for deallocations, when tracking enabled.
    unsigned long m_count;
    unsigned long m_max_count;
    bool m_count_enabled;

    // count any allocation or deallocation.
    unsigned long m_global_count;
    unsigned long m_max_global_count;
};

} // namespace ocl

#endif // OCL_GUARD_TEST_TESTMEMORYCOUNTER_HPP

