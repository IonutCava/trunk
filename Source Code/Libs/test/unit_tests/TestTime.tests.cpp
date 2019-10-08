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

#include "../Test.hpp"
#include "../TestTime.hpp"

#include "EnabledTests.h"

#if defined(TESTTIME_TESTS_ENABLED) && (TESTTIME_TESTS_ENABLED != 0)

TEST_MEMBER_FUNCTION(TestTime, TestTime, bool)
{
    using ocl::TestTime;
    bool is_set;

    {
        TestTime tt(false);
        is_set = tt.IsSet();
        CHECK_FALSE(is_set);
    }

    {
        // Pause slightly to ensure this test isn't so fast
        // that nanoseconds is still 0.
        // NOTE: Use 10 milliseconds for CPUs that don't have precise timers.
        Sleep(10);

        TestTime tt(true);
        is_set = tt.IsSet();
        CHECK_TRUE(is_set);
    }

    {
        // Ensure default behaviour doesn't set the time.
        // NOTE: This is tested last to make use of earlier Sleep call,
        //       and verify time has moved on.
        TestTime tt;
        is_set = tt.IsSet();
        CHECK_FALSE(is_set);
    }
}

TEST_MEMBER_FUNCTION(TestTime, TestTime, unsigned_long_unsigned_long)
{
    TEST_OVERRIDE_ARGS("unsigned long,unsigned long");

    using ocl::TestTime;

    {
        TestTime tt(0, 0);
        CHECK_FALSE(tt.IsSet());
    }

    {
        TestTime tt(1, 0);
        CHECK_TRUE(tt.IsSet());
    }

    {
        TestTime tt(0, 1);
        CHECK_TRUE(tt.IsSet());
    }

    {
        TestTime tt(1, 1);
        CHECK_TRUE(tt.IsSet());
    }
}

TEST_MEMBER_FUNCTION(TestTime, TestTime, TestTime_const_ref)
{
    TEST_OVERRIDE_ARGS("TestTime const&");

    using ocl::TestTime;

    {
        TestTime other(0, 0);
        TestTime tt(other);
        CHECK_FALSE(tt.IsSet());
    }

    {
        TestTime other(1, 0);
        TestTime tt(other);
        CHECK_TRUE(tt.IsSet());
    }

    {
        TestTime other(0, 1);
        TestTime tt(other);
        CHECK_TRUE(tt.IsSet());
    }

    {
        TestTime other(1, 1);
        TestTime tt(other);
        CHECK_TRUE(tt.IsSet());
    }
}

TEST_CONST_MEMBER_FUNCTION(TestTime, operator_equal, TestTime_const_ref)
{
    TEST_OVERRIDE_FUNCTION_NAME_ARGS("operator ==", "TestTime const&");

    using ocl::TestTime;

    TestTime tt1(0, 0);
    TestTime tt2(0, 0);

    CHECK_EQUAL(tt1, tt2);

    tt1 += TestTime(0, 1);
    CHECK_FALSE(tt1 == tt2);

    tt2 += TestTime(0, 1);

    CHECK_EQUAL(tt1, tt2);
}

TEST_CONST_MEMBER_FUNCTION(TestTime, operator_not_equal, TestTime_const_ref)
{
    TEST_OVERRIDE_FUNCTION_NAME_ARGS("operator !=", "TestTime const&");

    using ocl::TestTime;

    TestTime tt1(0, 0);
    TestTime tt2(0, 0);

    CHECK_FALSE(tt1 != tt2);

    tt1 += TestTime(0, 1);
    CHECK_NOT_EQUAL(tt1, tt2);

    tt2 += TestTime(0, 1);
    CHECK_FALSE(tt1 != tt2);
}

TEST_CONST_MEMBER_FUNCTION(TestTime, operator_less, TestTime_const_ref)
{
    TEST_OVERRIDE_FUNCTION_NAME_ARGS("operator <", "TestTime const&");

    using ocl::TestTime;

    TestTime tt1(0, 0);
    TestTime tt2(0, 0);
    tt2 += TestTime(0, 1);

    CHECK_LESS(tt1, tt2);

    tt1 += TestTime(0, 1);
    CHECK_FALSE(tt1 < tt2);

    tt1 += TestTime(0, 1);
    CHECK_FALSE(tt1 < tt2);
}

TEST_CONST_MEMBER_FUNCTION(TestTime, operator_less_equal, TestTime_const_ref)
{
    TEST_OVERRIDE_FUNCTION_NAME_ARGS("operator <=", "TestTime const&");

    using ocl::TestTime;

    TestTime tt1(0, 0);
    TestTime tt2(0, 0);
    tt2 += TestTime(0, 1);

    CHECK_LESS_EQUAL(tt1, tt2);

    tt1 += TestTime(0, 1);
    CHECK_LESS_EQUAL(tt1, tt2);

    tt1 += TestTime(0, 1);
    CHECK_FALSE(tt1 <= tt2);
}

TEST_CONST_MEMBER_FUNCTION(TestTime, operator_more, TestTime_const_ref)
{
    TEST_OVERRIDE_FUNCTION_NAME_ARGS("operator >", "TestTime const&");

    using ocl::TestTime;

    TestTime tt1(0, 0);
    TestTime tt2(0, 0);
    tt1 += TestTime(0, 1);

    CHECK_GREATER(tt1, tt2);

    tt2 += TestTime(0, 1);
    CHECK_FALSE(tt1 > tt2);

    tt2 += TestTime(0, 1);
    CHECK_FALSE(tt1 > tt2);
}

TEST_MEMBER_FUNCTION(TestTime, operator_plus_eq, TestTime_const_ref)
{
    TEST_OVERRIDE_FUNCTION_NAME_ARGS("operator +=", "TestTime const&");

    using ocl::TestTime;

    TestTime tt(0, 0);
    tt += TestTime(0, 1);

    CHECK_EQUAL(tt.GetNanoseconds(), 1U);

    tt += TestTime(1, 0);
    CHECK_EQUAL(tt.GetSeconds(), 1U);
    CHECK_EQUAL(tt.GetNanoseconds(), 1U);

    tt += TestTime(1, 1);
    CHECK_EQUAL(tt.GetSeconds(), 2U);
    CHECK_EQUAL(tt.GetNanoseconds(), 2U);
}

TEST_MEMBER_FUNCTION(TestTime, operator_div_eq, TestTime_const_ref)
{
    TEST_OVERRIDE_FUNCTION_NAME_ARGS("operator /=", "TestTime const&");

    using ocl::TestTime;

    TestTime tt(0, 0);
    tt += TestTime(2, 0);

    tt /= 2;
    CHECK_EQUAL(tt.GetSeconds(), 1U);

    tt /= 2;
    CHECK_EQUAL(tt.GetSeconds(), 0U);
    CHECK_EQUAL(tt.GetMilliseconds(), 500U);
}

TEST_MEMBER_FUNCTION(TestTime, IsSet, NA)
{
    using ocl::TestTime;

    {
        TestTime tt;
        CHECK_FALSE(tt.IsSet());
    }

    {
        TestTime tt(1, 0);
        CHECK_TRUE(tt.IsSet());
    }

    {
        TestTime tt(0, 1);
        CHECK_TRUE(tt.IsSet());
    }

    {
        TestTime tt(1, 1);
        CHECK_TRUE(tt.IsSet());
        tt.Clear();
        CHECK_FALSE(tt.IsSet());
    }
}

TEST_MEMBER_FUNCTION(TestTime, Clear, NA)
{
    using ocl::TestTime;

    {
        TestTime tt(1, 0);
        tt.Clear();
        CHECK_FALSE(tt.IsSet());
    }

    {
        TestTime tt(0, 1);
        tt.Clear();
        CHECK_FALSE(tt.IsSet());
    }

    {
        TestTime tt(1, 1);
        tt.Clear();
        CHECK_FALSE(tt.IsSet());
    }
}

TEST_MEMBER_FUNCTION(TestTime, ClearSeconds, NA)
{
    using ocl::TestTime;

    {
        TestTime tt(1, 0);
        tt.ClearSeconds();
        CHECK_FALSE(tt.IsSet());
    }

    {
        TestTime tt(0, 1);
        tt.ClearSeconds();
        CHECK_TRUE(tt.IsSet());
        CHECK_EQUAL(tt.GetNanoseconds(), 1U);
    }

    {
        TestTime tt(1, 1);
        tt.ClearSeconds();
        CHECK_TRUE(tt.IsSet());
        CHECK_EQUAL(tt.GetNanoseconds(), 1U);
    }
}

TEST_MEMBER_FUNCTION(TestTime, ClearNanoseconds, NA)
{
    using ocl::TestTime;

    {
        TestTime tt(1, 0);
        tt.ClearNanoseconds();
        CHECK_TRUE(tt.IsSet());
        CHECK_EQUAL(tt.GetSeconds(), 1U);
    }

    {
        TestTime tt(0, 1);
        tt.ClearNanoseconds();
        CHECK_FALSE(tt.IsSet());
    }

    {
        TestTime tt(1, 1);
        tt.ClearNanoseconds();
        CHECK_TRUE(tt.IsSet());
        CHECK_EQUAL(tt.GetSeconds(), 1U);
    }
}

TEST_MEMBER_FUNCTION(TestTime, Refresh, NA)
{
    using ocl::TestTime;

    TestTime tt;
    tt.Refresh();
    CHECK_TRUE(tt.IsSet());
}

TEST_MEMBER_FUNCTION(TestTime, GetSeconds, NA)
{
    using ocl::TestTime;

    TestTime tt;
    CHECK_EQUAL(tt.GetSeconds(), 0U);

    tt.SetTime(0U, 1U);
    CHECK_EQUAL(tt.GetSeconds(), 0U);
    CHECK_EQUAL(tt.GetNanoseconds(), 1U);

    tt.SetTime(1U, 0U);
    CHECK_EQUAL(tt.GetSeconds(), 1U);
    CHECK_EQUAL(tt.GetNanoseconds(), 0U);

    tt.SetTime(2U, 1U);
    CHECK_EQUAL(tt.GetSeconds(), 2U);
    CHECK_EQUAL(tt.GetNanoseconds(), 1U);

    tt.SetTime(0U, 1U);
    CHECK_EQUAL(tt.GetSeconds(), 0U);
    CHECK_EQUAL(tt.GetNanoseconds(), 1U);

    tt.Clear();
    tt.SetSeconds(1U);
    CHECK_EQUAL(tt.GetSeconds(), 1U);

    tt.SetSeconds(2U);
    CHECK_EQUAL(tt.GetSeconds(), 2U);
}

TEST_MEMBER_FUNCTION(TestTime, GetSecondsInNanoseconds, NA)
{
    using ocl::TestTime;
    unsigned long NANOSECONDS_PER_SECOND = TestTime::NANOSECONDS_PER_SECOND;

    CHECK_EQUAL(NANOSECONDS_PER_SECOND, 1000000000U);

    TestTime tt;
    CHECK_EQUAL(tt.GetSecondsInNanoseconds(), 0U);

    tt.SetTime(0U, 1U);
    CHECK_EQUAL(tt.GetSecondsInNanoseconds(), 0U);
    CHECK_EQUAL(tt.GetNanoseconds(), 1U);

    tt.SetTime(1U, 0U);
    CHECK_EQUAL(tt.GetSecondsInNanoseconds(), 1U * NANOSECONDS_PER_SECOND);
    CHECK_EQUAL(tt.GetNanoseconds(), 0U);

    tt.SetTime(2U, 1U);
    CHECK_EQUAL(tt.GetSecondsInNanoseconds(), 2U * NANOSECONDS_PER_SECOND);
    CHECK_EQUAL(tt.GetNanoseconds(), 1U);

    tt.SetTime(0U, 1U);
    CHECK_EQUAL(tt.GetSecondsInNanoseconds(), 0U);
    CHECK_EQUAL(tt.GetNanoseconds(), 1U);

    tt.Clear();
    tt.SetSeconds(1U);
    CHECK_EQUAL(tt.GetSecondsInNanoseconds(), 1U * NANOSECONDS_PER_SECOND);

    tt.SetSeconds(2U);
    CHECK_EQUAL(tt.GetSecondsInNanoseconds(), 2U * NANOSECONDS_PER_SECOND);
}

TEST_MEMBER_FUNCTION(TestTime, GetNanoseconds, NA)
{
    using ocl::TestTime;

    TestTime tt;
    CHECK_EQUAL(tt.GetNanoseconds(), 0U);

    tt.SetTime(0U, 1U);
    CHECK_EQUAL(tt.GetNanoseconds(), 1U);

    tt.SetTime(1U, 0U);
    CHECK_EQUAL(tt.GetNanoseconds(), 0U);

    tt.SetTime(2U, 1U);
    CHECK_EQUAL(tt.GetNanoseconds(), 1U);

    tt.SetTime(0U, 1U);
    CHECK_EQUAL(tt.GetNanoseconds(), 1U);

    tt.Clear();
    tt.SetNanoseconds(1U);
    CHECK_EQUAL(tt.GetNanoseconds(), 1U);

    tt.SetNanoseconds(2U);
    CHECK_EQUAL(tt.GetNanoseconds(), 2U);
}

TEST_MEMBER_FUNCTION(TestTime, GetMilliseconds, NA)
{
    using ocl::TestTime;
    unsigned long NANOSECONDS_PER_MILLISECOND = TestTime::NANOSECONDS_PER_MILLISECOND;

    CHECK_EQUAL(NANOSECONDS_PER_MILLISECOND, 1000000U);

    TestTime tt;
    CHECK_EQUAL(tt.GetMilliseconds(), 0U);

    tt.SetTime(0U, 1U * NANOSECONDS_PER_MILLISECOND);
    CHECK_EQUAL(tt.GetMilliseconds(), 1U);

    tt.SetTime(1U, 0U);
    CHECK_EQUAL(tt.GetMilliseconds(), 0U);

    tt.SetTime(2U, 1U * NANOSECONDS_PER_MILLISECOND);
    CHECK_EQUAL(tt.GetMilliseconds(), 1U);

    tt.SetTime(0U, 1U * NANOSECONDS_PER_MILLISECOND);
    CHECK_EQUAL(tt.GetMilliseconds(), 1U);

    tt.Clear();
    tt.SetNanoseconds(1U * NANOSECONDS_PER_MILLISECOND);
    CHECK_EQUAL(tt.GetMilliseconds(), 1U);

    tt.SetNanoseconds(2U * NANOSECONDS_PER_MILLISECOND);
    CHECK_EQUAL(tt.GetMilliseconds(), 2U);
}

TEST_MEMBER_FUNCTION(TestTime, SetSeconds, unsigned_long)
{
    TEST_OVERRIDE_ARGS("unsigned long");

    using ocl::TestTime;

    TestTime tt;
    tt.SetSeconds(1U);
    CHECK_EQUAL(tt.GetSeconds(), 1U);

    tt.Clear();
    CHECK_EQUAL(tt.GetSeconds(), 0U);

    tt.SetSeconds(1U);
    CHECK_EQUAL(tt.GetSeconds(), 1U);

    tt.SetSeconds(2U);
    CHECK_EQUAL(tt.GetSeconds(), 2U);

    tt.SetSeconds(0U);
    CHECK_EQUAL(tt.GetSeconds(), 0U);
}

TEST_MEMBER_FUNCTION(TestTime, SetNanoseconds, unsigned_long)
{
    TEST_OVERRIDE_ARGS("unsigned long");

    using ocl::TestTime;

    TestTime tt;
    tt.SetNanoseconds(1U);
    CHECK_EQUAL(tt.GetNanoseconds(), 1U);

    tt.Clear();
    CHECK_EQUAL(tt.GetNanoseconds(), 0U);

    tt.SetNanoseconds(1U);
    CHECK_EQUAL(tt.GetNanoseconds(), 1U);

    tt.SetNanoseconds(2U);
    CHECK_EQUAL(tt.GetNanoseconds(), 2U);

    tt.SetNanoseconds(0U);
    CHECK_EQUAL(tt.GetNanoseconds(), 0U);
}

TEST_MEMBER_FUNCTION(TestTime, SetMilliseconds, unsigned_long)
{
    TEST_OVERRIDE_ARGS("unsigned long");

    using ocl::TestTime;

    unsigned long NANOSECONDS_PER_MILLISECOND = TestTime::NANOSECONDS_PER_MILLISECOND;
    CHECK_EQUAL(NANOSECONDS_PER_MILLISECOND, 1000000U);

    TestTime tt;
    tt.SetMilliseconds(1U);
    CHECK_EQUAL(tt.GetMilliseconds(), 1U);
    CHECK_EQUAL(tt.GetNanoseconds(), 1U * NANOSECONDS_PER_MILLISECOND);

    tt.Clear();
    CHECK_EQUAL(tt.GetMilliseconds(), 0U);
    CHECK_EQUAL(tt.GetNanoseconds(), 0U);

    tt.SetMilliseconds(1U);
    CHECK_EQUAL(tt.GetMilliseconds(), 1U);
    CHECK_EQUAL(tt.GetNanoseconds(), 1U * NANOSECONDS_PER_MILLISECOND);

    tt.SetMilliseconds(2U);
    CHECK_EQUAL(tt.GetMilliseconds(), 2U);
    CHECK_EQUAL(tt.GetNanoseconds(), 2U * NANOSECONDS_PER_MILLISECOND);

    tt.SetMilliseconds(0U);
    CHECK_EQUAL(tt.GetMilliseconds(), 0U);
    CHECK_EQUAL(tt.GetNanoseconds(), 0U);
}

TEST_MEMBER_FUNCTION(TestTime, SetMicroseconds, unsigned_long)
{
    TEST_OVERRIDE_ARGS("unsigned long");

    using ocl::TestTime;

    unsigned long NANOSECONDS_PER_MICROSECOND = TestTime::NANOSECONDS_PER_MICROSECOND;
    CHECK_EQUAL(NANOSECONDS_PER_MICROSECOND, 1000U);

    TestTime tt;
    tt.SetMilliseconds(1U);
    CHECK_EQUAL(tt.GetMilliseconds(), 1U);
    CHECK_EQUAL(tt.GetMicroseconds(), 1U * NANOSECONDS_PER_MICROSECOND);

    tt.Clear();
    CHECK_EQUAL(tt.GetMilliseconds(), 0U);
    CHECK_EQUAL(tt.GetMicroseconds(), 0U);

    tt.SetMilliseconds(1U);
    CHECK_EQUAL(tt.GetMilliseconds(), 1U);
    CHECK_EQUAL(tt.GetMicroseconds(), 1U * NANOSECONDS_PER_MICROSECOND);

    tt.SetMilliseconds(2U);
    CHECK_EQUAL(tt.GetMilliseconds(), 2U);
    CHECK_EQUAL(tt.GetMicroseconds(), 2U * NANOSECONDS_PER_MICROSECOND);

    tt.SetMilliseconds(0U);
    CHECK_EQUAL(tt.GetMilliseconds(), 0U);
    CHECK_EQUAL(tt.GetMicroseconds(), 0U);
}

TEST_MEMBER_FUNCTION(TestTime, GetTimeInNanoseconds, NA)
{
    using ocl::TestTime;

    TestTime tt;
    CHECK_EQUAL(tt.GetTimeInNanoseconds(), 0U);

    tt.SetTimeInNanoseconds(1U);
    CHECK_EQUAL(tt.GetTimeInNanoseconds(), 1U);

    tt.SetTime(1U, 1U);
    CHECK_EQUAL(tt.GetTimeInNanoseconds(), 1000000001U);

    tt.SetTime(0U, 2U);
    CHECK_EQUAL(tt.GetTimeInNanoseconds(), 2U);

    tt.SetTime(2U, 0U);
    CHECK_EQUAL(tt.GetTimeInNanoseconds(), 2000000000U);
}

TEST_MEMBER_FUNCTION(TestTime, SetTime, unsigned_long_unsigned_long)
{
    TEST_OVERRIDE_ARGS("unsigned long,unsigned long");

    using ocl::TestTime;

    TestTime tt;
    tt.SetTime(1U, 2U);
    CHECK_EQUAL(tt.GetSeconds(), 1U);
    CHECK_EQUAL(tt.GetNanoseconds(), 2U);

    tt.SetTime(0U, 0U);
    CHECK_EQUAL(tt.GetSeconds(), 0U);
    CHECK_EQUAL(tt.GetNanoseconds(), 0U);

    tt.SetTime(0U, 1U);
    CHECK_EQUAL(tt.GetSeconds(), 0U);
    CHECK_EQUAL(tt.GetNanoseconds(), 1U);

    tt.SetTime(1U, 0U);
    CHECK_EQUAL(tt.GetSeconds(), 1U);
    CHECK_EQUAL(tt.GetNanoseconds(), 0U);
}

TEST_MEMBER_FUNCTION(TestTime, SetTimeInNanoseconds, ocl_uint64)
{
    using ocl::TestTime;

    TestTime tt;
    tt.SetTimeInNanoseconds(1000000002U);
    CHECK_EQUAL(tt.GetSeconds(), 1U);
    CHECK_EQUAL(tt.GetNanoseconds(), 2U);

    tt.SetTimeInNanoseconds(0U);
    CHECK_EQUAL(tt.GetSeconds(), 0U);
    CHECK_EQUAL(tt.GetNanoseconds(), 0U);

    tt.SetTimeInNanoseconds(1U);
    CHECK_EQUAL(tt.GetSeconds(), 0U);
    CHECK_EQUAL(tt.GetNanoseconds(), 1U);

    tt.SetTimeInNanoseconds(1000000000U);
    CHECK_EQUAL(tt.GetSeconds(), 1U);
    CHECK_EQUAL(tt.GetNanoseconds(), 0U);
}

TEST_MEMBER_FUNCTION(TestTime, GetDiffTime, TestTime_const_ref_TestTime_ref)
{
    TEST_OVERRIDE_ARGS("TestTime const&, TestTime&");

    using ocl::TestTime;

    TestTime tm1(1, 0);
    TestTime tm1b(1, 0);
    TestTime tm2(2, 0);
    TestTime tm3(1, TestTime::MillisecondsToNanoseconds(500));
    TestTime tm4(2, TestTime::MillisecondsToNanoseconds(500));

    TestTime diff;

    diff.SetSeconds(10);
    diff.SetMilliseconds(10);
    tm1.GetDiffTime(tm2, diff);
    unsigned long seconds = diff.GetSeconds();
    unsigned long milliseconds = diff.GetMilliseconds();
    unsigned long nanoseconds = diff.GetNanoseconds();
    CHECK_EQUAL(seconds, 1U);
    CHECK_EQUAL(milliseconds, 0U);
    CHECK_EQUAL(nanoseconds, 0U);

    diff.SetSeconds(10);
    diff.SetMilliseconds(10);
    tm2.GetDiffTime(tm1, diff);
    seconds = diff.GetSeconds();
    milliseconds = diff.GetMilliseconds();
    nanoseconds = diff.GetNanoseconds();
    CHECK_EQUAL(seconds, 1U);
    CHECK_EQUAL(nanoseconds, 0U);
    CHECK_EQUAL(milliseconds, 0U);

    diff.SetSeconds(10);
    diff.SetMilliseconds(10);
    tm1.GetDiffTime(tm3, diff);
    seconds = diff.GetSeconds();
    milliseconds = diff.GetMilliseconds();
    nanoseconds = diff.GetNanoseconds();
    CHECK_EQUAL(seconds, 0U);
    CHECK_EQUAL(nanoseconds, TestTime::MillisecondsToNanoseconds(500));
    CHECK_EQUAL(milliseconds, 500U);

    diff.SetSeconds(10);
    diff.SetMilliseconds(10);
    tm3.GetDiffTime(tm1, diff);
    seconds = diff.GetSeconds();
    milliseconds = diff.GetMilliseconds();
    nanoseconds = diff.GetNanoseconds();
    CHECK_EQUAL(seconds, 0U);
    CHECK_EQUAL(nanoseconds, TestTime::MillisecondsToNanoseconds(500));
    CHECK_EQUAL(milliseconds, 500U);

    diff.SetSeconds(10);
    diff.SetMilliseconds(10);
    tm1.GetDiffTime(tm4, diff);
    seconds = diff.GetSeconds();
    milliseconds = diff.GetMilliseconds();
    nanoseconds = diff.GetNanoseconds();
    CHECK_EQUAL(seconds, 1U);
    CHECK_EQUAL(nanoseconds, TestTime::MillisecondsToNanoseconds(500));
    CHECK_EQUAL(milliseconds, 500U);

    diff.SetSeconds(10);
    diff.SetMilliseconds(10);
    tm4.GetDiffTime(tm1, diff);
    seconds = diff.GetSeconds();
    milliseconds = diff.GetMilliseconds();
    nanoseconds = diff.GetNanoseconds();
    CHECK_EQUAL(seconds, 1U);
    CHECK_EQUAL(nanoseconds, TestTime::MillisecondsToNanoseconds(500));
    CHECK_EQUAL(milliseconds, 500U);

    diff.SetSeconds(10);
    diff.SetMilliseconds(10);
    tm1.GetDiffTime(tm1b, diff);
    seconds = diff.GetSeconds();
    milliseconds = diff.GetMilliseconds();
    nanoseconds = diff.GetNanoseconds();
    CHECK_EQUAL(seconds, 0U);
    CHECK_EQUAL(nanoseconds, TestTime::MillisecondsToNanoseconds(0));
    CHECK_EQUAL(milliseconds, 0U);

    diff.SetSeconds(10);
    diff.SetMilliseconds(10);
    tm1b.GetDiffTime(tm1, diff);
    seconds = diff.GetSeconds();
    milliseconds = diff.GetMilliseconds();
    nanoseconds = diff.GetNanoseconds();
    CHECK_EQUAL(seconds, 0U);
    CHECK_EQUAL(nanoseconds, TestTime::MillisecondsToNanoseconds(0));
    CHECK_EQUAL(milliseconds, 0U);
}

TEST_MEMBER_FUNCTION(TestTime, MillisecondsToNanoseconds, unsigned_long)
{
    TEST_OVERRIDE_ARGS("unsigned long");

    using ocl::TestTime;

    unsigned long NANOSECONDS_PER_MILLISECOND = TestTime::NANOSECONDS_PER_MILLISECOND;
    CHECK_EQUAL(NANOSECONDS_PER_MILLISECOND, 1000000U);

    unsigned long ns = TestTime::MillisecondsToNanoseconds(0U);
    CHECK_TRUE(ns == 0U);

    ns = TestTime::MillisecondsToNanoseconds(1U);
    CHECK_EQUAL(ns, NANOSECONDS_PER_MILLISECOND);

    ns = TestTime::MillisecondsToNanoseconds(10U);
    CHECK_EQUAL(ns, NANOSECONDS_PER_MILLISECOND * 10U);
}

TEST_MEMBER_FUNCTION(TestTime, MicrosecondsToNanoseconds, unsigned_long)
{
    TEST_OVERRIDE_ARGS("unsigned long");

    using ocl::TestTime;

    unsigned long NANOSECONDS_PER_MICROSECOND = TestTime::NANOSECONDS_PER_MICROSECOND;
    CHECK_EQUAL(NANOSECONDS_PER_MICROSECOND, 1000U);

    unsigned long ns = TestTime::MicrosecondsToNanoseconds(0U);
    CHECK_TRUE(ns == 0U);

    ns = TestTime::MicrosecondsToNanoseconds(1U);
    CHECK_EQUAL(ns, NANOSECONDS_PER_MICROSECOND);

    ns = TestTime::MicrosecondsToNanoseconds(10U);
    CHECK_EQUAL(ns, NANOSECONDS_PER_MICROSECOND * 10U);
}

TEST_MEMBER_FUNCTION(TestTime, NanosecondsToMilliseconds, unsigned_long)
{
    TEST_OVERRIDE_ARGS("unsigned long");

    using ocl::TestTime;

    unsigned long NANOSECONDS_PER_MILLISECOND = TestTime::NANOSECONDS_PER_MILLISECOND;
    CHECK_EQUAL(NANOSECONDS_PER_MILLISECOND, 1000000U);

    unsigned long ms = TestTime::NanosecondsToMilliseconds(0U);
    CHECK_TRUE(ms == 0U);

    ms = TestTime::NanosecondsToMilliseconds(1000000U);
    CHECK_EQUAL(ms, 1U);

    ms = TestTime::NanosecondsToMilliseconds(10000000U);
    CHECK_EQUAL(ms, 10U);

    ms = TestTime::NanosecondsToMilliseconds(999999U);
    CHECK_EQUAL(ms, 0U);
}

TEST_MEMBER_FUNCTION(TestTime, NanosecondsToMicroseconds, unsigned_long)
{
    TEST_OVERRIDE_ARGS("unsigned long");

    using ocl::TestTime;

    unsigned long NANOSECONDS_PER_MICROSECOND = TestTime::NANOSECONDS_PER_MICROSECOND;
    CHECK_EQUAL(NANOSECONDS_PER_MICROSECOND, 1000U);

    unsigned long ms = TestTime::NanosecondsToMicroseconds(0U);
    CHECK_TRUE(ms == 0U);

    ms = TestTime::NanosecondsToMicroseconds(1000U);
    CHECK_EQUAL(ms, 1U);

    ms = TestTime::NanosecondsToMicroseconds(10000U);
    CHECK_EQUAL(ms, 10U);

    ms = TestTime::NanosecondsToMicroseconds(999U);
    CHECK_EQUAL(ms, 0U);
}

TEST_MEMBER_FUNCTION(TestTime, MillisecondsToNanoseconds64, unsigned_long)
{
    using ocl::TestTime;
    using ocl::ocl_uint64;

    ocl_uint64 NANOSECONDS_PER_MILLISECOND = TestTime::NANOSECONDS_PER_MILLISECOND;
    CHECK_EQUAL(NANOSECONDS_PER_MILLISECOND, 1000000U);

    ocl_uint64 ns = TestTime::MillisecondsToNanoseconds64(0U);
    CHECK_TRUE(ns == 0U);

    ns = TestTime::MillisecondsToNanoseconds64(1U);
    CHECK_EQUAL(ns, NANOSECONDS_PER_MILLISECOND);

    ns = TestTime::MillisecondsToNanoseconds64(10U);
    CHECK_EQUAL(ns, NANOSECONDS_PER_MILLISECOND * 10U);
}

TEST_MEMBER_FUNCTION(TestTime, MicrosecondsToNanoseconds64, unsigned_long)
{
    using ocl::TestTime;
    using ocl::ocl_uint64;

    ocl_uint64 NANOSECONDS_PER_MICROSECOND = TestTime::NANOSECONDS_PER_MICROSECOND;
    CHECK_EQUAL(NANOSECONDS_PER_MICROSECOND, 1000U);

    ocl_uint64 ns = TestTime::MicrosecondsToNanoseconds64(0U);
    CHECK_TRUE(ns == 0U);

    ns = TestTime::MicrosecondsToNanoseconds64(1U);
    CHECK_EQUAL(ns, NANOSECONDS_PER_MICROSECOND);

    ns = TestTime::MicrosecondsToNanoseconds64(10U);
    CHECK_EQUAL(ns, NANOSECONDS_PER_MICROSECOND * 10U);
}

TEST_MEMBER_FUNCTION(TestTime, NanosecondsToMilliseconds64, unsigned_long)
{
    using ocl::TestTime;
    using ocl::ocl_uint64;

    ocl_uint64 NANOSECONDS_PER_MILLISECOND = TestTime::NANOSECONDS_PER_MILLISECOND;
    CHECK_EQUAL(NANOSECONDS_PER_MILLISECOND, 1000000U);

    ocl_uint64 ms = TestTime::NanosecondsToMilliseconds64(0U);
    CHECK_TRUE(ms == 0U);

    ms = TestTime::NanosecondsToMilliseconds64(1000000U);
    CHECK_EQUAL(ms, 1U);

    ms = TestTime::NanosecondsToMilliseconds64(10000000U);
    CHECK_EQUAL(ms, 10U);

    ms = TestTime::NanosecondsToMilliseconds64(999999U);
    CHECK_EQUAL(ms, 0U);
}

TEST_MEMBER_FUNCTION(TestTime, NanosecondsToMicroseconds64, ocl_uint64)
{
    using ocl::TestTime;
    using ocl::ocl_uint64;

    ocl_uint64 NANOSECONDS_PER_MICROSECOND = TestTime::NANOSECONDS_PER_MICROSECOND;
    CHECK_EQUAL(NANOSECONDS_PER_MICROSECOND, 1000U);

    ocl_uint64 ms = TestTime::NanosecondsToMicroseconds64(0U);
    CHECK_TRUE(ms == 0U);

    ms = TestTime::NanosecondsToMicroseconds64(1000U);
    CHECK_EQUAL(ms, 1U);

    ms = TestTime::NanosecondsToMicroseconds64(10000U);
    CHECK_EQUAL(ms, 10U);

    ms = TestTime::NanosecondsToMicroseconds64(999U);
    CHECK_EQUAL(ms, 0U);
}

#endif // #if defined(TESTTIME_TESTS_ENABLED) && (TESTTIME_TESTS_ENABLED != 0)
