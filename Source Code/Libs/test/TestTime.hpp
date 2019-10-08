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

#ifndef OCL_GUARD_TEST_TESTTIME_HPP
#define OCL_GUARD_TEST_TESTTIME_HPP

#include <time.h>
#include <limits.h>
#include "TestTypes.hpp"

namespace ocl
{

/**
 * Store an elapsed time value and be able to
 * extract seconds, milliseconds and nanoseconds.
 * This is used for the timing functions and member functions.
 *
 * @note although nanosecond timing is not performed, this class
 * is designed to be upgraded later with a minimal of effort.
 */

class TestTime
{
public:
    static const unsigned long HOURS_PER_DAY                = 24;
    static const unsigned long MINUTES_PER_HOUR             = 60;
    static const unsigned long SECONDS_PER_MINUTE           = 60;
    static const unsigned long MILLISECONDS_PER_SECOND      = 1000;
    static const unsigned long MICROSECONDS_PER_SECOND      = 1000000;
    static const unsigned long NANOSECONDS_PER_SECOND       = 1000000000;
    static const unsigned long MILLISECONDS_PER_MICROSECOND = 1000;
    static const unsigned long NANOSECONDS_PER_MILLISECOND  = 1000000;
    static const unsigned long NANOSECONDS_PER_MICROSECOND  = 1000;

    // Maximum number of characters required to display value as a string.
    static const unsigned long SECONDS_CHARS      = 2;
    static const unsigned long MILLISECONDS_CHARS = 3;
    static const unsigned long MICROSECONDS_CHARS = 6;
    static const unsigned long NANOSECONDS_CHARS  = 9;

public:
    TestTime(bool refresh = false) throw()
        : m_nanoseconds(0)
    {
        if (refresh)
            Refresh();
    }

    TestTime(unsigned long seconds, unsigned long nanoseconds) throw()
        : m_nanoseconds(0)
    {
        SetTime(seconds, nanoseconds);
    }

    TestTime(TestTime const& other_time) throw()
        : m_nanoseconds(other_time.m_nanoseconds)
    {
    }

    bool operator ==(TestTime const& other_time) const throw()
    {
        return m_nanoseconds == other_time.m_nanoseconds;
    }

    bool operator !=(TestTime const& other_time) const throw()
    {
        return m_nanoseconds != other_time.m_nanoseconds;
    }

    bool operator <(TestTime const& other_time) const throw()
    {
        return m_nanoseconds < other_time.m_nanoseconds;
    }

    bool operator <=(TestTime const& other_time) const throw()
    {
        return m_nanoseconds <= other_time.m_nanoseconds;
    }

    bool operator >(TestTime const& other_time) const throw()
    {
        return m_nanoseconds > other_time.m_nanoseconds;
    }

    bool operator >=(TestTime const& other_time) const throw()
    {
        return m_nanoseconds >= other_time.m_nanoseconds;
    }

    TestTime& operator +=(TestTime const& time_to_add) throw()
    {
        m_nanoseconds += time_to_add.m_nanoseconds;
        return *this;
    }

    TestTime& operator /=(size_t div_by) throw()
    {
        m_nanoseconds /= static_cast<ocl_uint64>(div_by);
        return *this;
    }

public:
    bool IsSet() const throw()
    {
        return m_nanoseconds > 0;
    }

    void Clear() throw()
    {
        m_nanoseconds = 0;
    }

    void ClearSeconds() throw()
    {
        m_nanoseconds -= GetSecondsInNanoseconds();
    }

    void ClearNanoseconds()
    {
        ocl_uint64 nanoseconds = m_nanoseconds - GetSecondsInNanoseconds();
        m_nanoseconds -= nanoseconds;
    }

    /// Update with the current system time.
    void Refresh()
    {
        clock_t ticks = clock();
        privateFromClock(ticks, CLOCKS_PER_SEC, m_nanoseconds);
    }

    unsigned long GetSeconds() const throw()
    {
        return static_cast<unsigned long>(m_nanoseconds / NANOSECONDS_PER_SECOND);
    }

    // Return the number of seconds, converted to Nanoseconds.
    // NOTE: This does not include the nanoseconds returned by GetNanoseconds.
    ocl_uint64 GetSecondsInNanoseconds() const throw()
    {
        ocl_uint64 seconds = GetSeconds();
        return seconds * NANOSECONDS_PER_SECOND;
    }

    unsigned long GetNanoseconds() const throw()
    {
        // Remove the seconds and return the remaining part.
        ocl_uint64 seconds_in_nanoseconds = GetSecondsInNanoseconds();
        return static_cast<unsigned long>(m_nanoseconds - seconds_in_nanoseconds);
    }

    unsigned long GetMicroseconds() const throw()
    {
        return static_cast<unsigned long>(NanosecondsToMicroseconds(GetNanoseconds()));
    }

    unsigned long GetMilliseconds() const throw()
    {
        return static_cast<unsigned long>(NanosecondsToMilliseconds(GetNanoseconds()));
    }

    void SetSeconds(unsigned long seconds) throw()
    {
        ClearSeconds();
        ocl_uint64 seconds_in_nanoseconds = seconds * NANOSECONDS_PER_SECOND;
        m_nanoseconds += seconds_in_nanoseconds;
    }

    /// Sets the nanoseconds part of the time.
    /// @note if this is equal or greater than NANOSECONDS_PER_SECOND
    /// then the behaviour is undefined.
    void SetNanoseconds(unsigned long nanoseconds) throw()
    {
        ClearNanoseconds();
        m_nanoseconds += nanoseconds;
    }

    /// Sets the nanoseconds part of the time.
    /// @note if this is equal or greater than MILLISECONDS_PER_SECOND
    /// then the behaviour is undefined.
    void SetMilliseconds(unsigned long milliseconds) throw()
    {
        ClearNanoseconds();
        m_nanoseconds += MillisecondsToNanoseconds(milliseconds);
    }

    /// Sets the nanoseconds part of the time.
    /// @note if this is equal or greater than NANOSECONDS_PER_MILLISECOND
    /// then the behaviour is undefined.
    void SetMicroseconds(unsigned long microseconds) throw()
    {
        ClearNanoseconds();
        m_nanoseconds += MicrosecondsToNanoseconds(microseconds);
    }

    ocl_uint64 GetTimeInNanoseconds() const throw()
    {
        return m_nanoseconds;
    }

    void SetTime(unsigned long seconds, unsigned long nanoseconds)
    {
        m_nanoseconds = static_cast<ocl_uint64>(seconds) * NANOSECONDS_PER_SECOND;
        m_nanoseconds += nanoseconds;
    }

    void SetTimeInNanoseconds(ocl_uint64 nanoseconds)
    {
        m_nanoseconds = nanoseconds;
    }

    void GetDiffTime(TestTime const& other_time,
                     TestTime& diff_time) const throw()
    {
        if (m_nanoseconds > other_time.m_nanoseconds)
            diff_time.m_nanoseconds = m_nanoseconds - other_time.m_nanoseconds;
        else
            diff_time.m_nanoseconds = other_time.m_nanoseconds - m_nanoseconds;
    }

// Static helper member functions,
// with conversion functions added to aid readability of code.
public:
    static void Sleep(unsigned long milliseconds)
    {
        unsigned long expected_seconds = milliseconds / MILLISECONDS_PER_SECOND;
        milliseconds -= expected_seconds * MILLISECONDS_PER_SECOND;

        TestTime tt;
        for (;;)
        {
            tt.Refresh();
            unsigned long seconds = tt.GetSeconds();
            if (seconds > expected_seconds)
                break;
            if ((seconds == expected_seconds) && (tt.GetMilliseconds() >= milliseconds))
                break;
        }
    }

    static unsigned long MillisecondsToNanoseconds(unsigned long milliseconds)
    {
        return milliseconds * NANOSECONDS_PER_MILLISECOND;
    }

    static unsigned long MicrosecondsToNanoseconds(unsigned long microseconds)
    {
        return microseconds * MILLISECONDS_PER_SECOND;
    }

    static unsigned long NanosecondsToMilliseconds(unsigned long nanoseconds)
    {
        return nanoseconds / NANOSECONDS_PER_MILLISECOND;
    }

    static unsigned long NanosecondsToMicroseconds(unsigned long nanoseconds)
    {
        return nanoseconds / MILLISECONDS_PER_SECOND;
    }

    static ocl_uint64 MillisecondsToNanoseconds64(ocl_uint64 milliseconds)
    {
        return milliseconds * NANOSECONDS_PER_MILLISECOND;
    }

    static ocl_uint64 MicrosecondsToNanoseconds64(ocl_uint64 microseconds)
    {
        return microseconds * MILLISECONDS_PER_SECOND;
    }

    static ocl_uint64 NanosecondsToMilliseconds64(ocl_uint64 nanoseconds)
    {
        return nanoseconds / NANOSECONDS_PER_MILLISECOND;
    }

    static ocl_uint64 NanosecondsToMicroseconds64(ocl_uint64 nanoseconds)
    {
        return nanoseconds / MILLISECONDS_PER_SECOND;
    }

private:
    // Cannot #if CLOCKS_PER_SEC != 1000 on gcc compiler,
    // so ensure its run-time checked.
    static void privateFromClock(clock_t ticks,
                                 clock_t clocks_per_sec,
                                 ocl_uint64& time_in_nanoseconds)
    {
        static const clock_t ms = MILLISECONDS_PER_SECOND;

        if (clocks_per_sec > 0)
        {
            clock_t seconds = ticks / clocks_per_sec;

            clock_t milliseconds;
            if (clocks_per_sec != ms)
                milliseconds = (seconds * ms) / clocks_per_sec;
            else
                milliseconds = ticks;

            time_in_nanoseconds = static_cast<ocl_uint64>(milliseconds) *
                                  NANOSECONDS_PER_MILLISECOND;
        }
        else
            time_in_nanoseconds = 0;
    }

private:
    // Due to lack of support for int64_t or long long for different platforms,
    // this is implemented as two values.

    ocl_uint64 m_nanoseconds;
};

} // namespace ocl

#endif // OCL_GUARD_TEST_TESTTIME_HPP
