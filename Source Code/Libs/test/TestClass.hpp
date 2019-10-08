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

#ifndef OCL_GUARD_TEST_TESTCLASS_HPP
#define OCL_GUARD_TEST_TESTCLASS_HPP

#include "TestLog.hpp"
#include "TestTime.hpp"
#include "TestTypes.hpp"
#include "TestString.hpp"
#include "TestCompare.hpp"
#include "TestTypeInfo.hpp"
#include "TestIntegerUtility.hpp"
#include "TestStringUtility.hpp"
#include "TestClassSharedData.hpp"
#include "TestStdioFileFunctor.hpp"
#include "TestMemoryLeakCheck.hpp"
#include "TestMemoryCounter.hpp"
#include "TestSetupTeardown.hpp"

#include <cstring>
#include <climits>
#include <cstddef>

namespace ocl
{

/// Record and report on a unit test,
/// as well as updating all shared unit test results.
class TestClass
{
public:
    // Maximum padded digits for number of tests.
    static TestString::size_type const max_digits = 6;

    // Pad characters for error lines, which should like up with test counter.
    static TestString::size_type const error_padding = max_digits + 2;

    // Construct the base class for a unit test,
    // optionally tracking the start and end of any leaks.
    //
    // NOTE: Do not pass objects that allocate memory into the constructor,
    //       as these would not get tracked on allocation, but cause leak detector
    //       to detect the free of the memory when exiting constructor.
    TestClass(char const* class_name,
              char const* function_name,
              char const* args,
              bool is_const = false,
              bool is_timed = false,
              unsigned long secs = 0,
              unsigned long nanosecs = 0)
        : m_is_const(is_const)
        , m_is_timed(is_timed)
        , m_start_time(is_timed)
        , m_current_time(m_start_time)
        , m_timed_function_calls(0)
        , m_sample_time(secs, nanosecs)
        , m_check_count(0)
        , m_failure_check_count(0)
        , m_test_number(0)
        , m_logged(false)
    {
        GetSharedData().SetClassName(class_name);

        // Ensure that the next constructor can complete
        // the snapshot memory leak checking.
        TestClass*& prev_test = GetPreviousTest();
        if (prev_test != NULL)
        {
            // This is not the first test, so log results from previous test.
            prev_test->LogTests();
        }
        else if (GetSharedData().GetLogger() == NULL)
        {
            // Force logger to be created before memory leak checking starts,
            // so that this does not get recorded in the start/end memory checking.
            LogWriteLine("Unable to create logging!");
        }

        prev_test = this;

        if (is_timed && !m_sample_time.IsSet())
            m_sample_time.SetSeconds(1);

        // Start leak checking before any members are dynamically allocated,
        // but after the previous test has the memory leak checking stopped.
        StartLeakChecking();

        privateSetupPostLeakCheckingData(class_name, function_name, args);
        privateIncrementCounters();

        // Start the setup part of a fixture, after leak detection starts.
        Setup();
    }

    virtual ~TestClass()
    {
        LogTests();

        GetSharedData().IncDestructions();
        if (GetSharedData().GetDestructions() > GetSharedData().GetConstructions())
            LogWriteLine("Error matching start and end of tests!");

        // All tests are complete so dump out the summary and
        // the full memory leak report.
        if (IsLast())
        {
            // Output the global data for all tests.
            privateLogSharedData();

            // Clear the logger to ensure the allocation
            // for the first test is freed.
            GetSharedData().Clear();

            // Any memory leaks will get reported by dumping
            // all information supported for the platform,
            // which might include filename and line number.
            m_leak_check.DumpAll();
        }
    }

// Functions for managing the function or member function test.
public:
    TestString const& GetClassName() const throw()
    {
        return m_class_name;
    }

    void SetClassName(TestString const& class_name)
    {
        m_class_name = class_name;
    }

    TestString const& GetFunctionName() const throw()
    {
        return m_function_name;
    }

    void SetFunctionName(TestString const& function_name)
    {
        m_function_name = function_name;
    }

    TestString const& GetArgs() const throw()
    {
        return m_args;
    }

    void SetArgs(TestString const& args)
    {
        if (args != "NA")
            m_args = args;
        else
            m_args.Clear();
    }

    /// return string as class name::function name([args]) [const]
    /// , function name([args]) or general test name.
    TestString GetTestName() const
    {
        TestString const& function_name = GetFunctionName();

        TestString str = GetClassName();

        // If function_name is empty then this is a general test.
        if (!function_name.IsEmpty())
        {
            if (!str.IsEmpty())
                str += "::";
            str += function_name;
            str += "(";
            str += GetArgs();
            str += ")";
            if (m_is_const)
                str += " const";
        }

        return str;
    }

// Call the user defined setup and tear down functions via TestClass::Execute.
public:
    void Setup()
    {
        TestSetupTeardown& setup_teardown = privateSetupTeardown();
        TestString const& this_class_name = GetClassName();
        TestString class_name = setup_teardown.GetClassName<true>();
        if (setup_teardown.IsSetupSet() && this_class_name == class_name)
            setup_teardown.Execute<true>();
    }

    void Teardown()
    {
        TestSetupTeardown& setup_teardown = privateSetupTeardown();
        TestString const& this_class_name = GetClassName();
        TestString class_name = setup_teardown.GetClassName<false>();
        if (setup_teardown.IsTeardownSet() &&
            setup_teardown.IsSetupRun() &&
            this_class_name == class_name)
            setup_teardown.Execute<false>();
    }

// Set the appropriate class objects via using the TEST_SETUP and TEST_TEARDOWN macros.
public:
    static void SetSetup(TestSetupTeardownFunctor& setup_functor)
    {
        TestSetupTeardown& setup_teardown = privateSetupTeardown();

        if (setup_teardown.IsSetupRun() &&
            (setup_teardown.GetSetupClassName() != setup_functor.GetClassName()))
        {
            // If tear down execute is not run before setting new setup functor,
            // Then the class has changed and we need to complete the previous
            // tear down before starting a new class fixture.
            setup_teardown.Execute<false>();
        }

        setup_teardown.SetSetupFunctor(setup_functor);
    }

    static void SetTeardown(TestSetupTeardownFunctor& teardown_functor)
    {
        TestSetupTeardown& setup_teardown = privateSetupTeardown();
        setup_teardown.SetTeardownFunctor(teardown_functor);
    }

    static void ClearSetupTeardown()
    {
        TestSetupTeardown& setup_teardown = privateSetupTeardown();
        setup_teardown.Clear();
    }

// General helper functions.
public:
    void StartLeakChecking()
    {
        // Need to make sure all dynamically allocated memory within this test class
        // is freed before the leak checking starts.
        Clear();

        m_leak_check.Start();
    }

    /// Get status for tests and failures.
    bool HasTests() const throw()
    {
        return m_check_count > 0;
    }

    bool HasFailed() const throw()
    {
        return m_failure_check_count > 0;
    }

    bool IsGeneralTest() const throw()
    {
        return GetFunctionName().IsEmpty();
    }

    bool IsLast() const throw()
    {
        return GetSharedData().IsLast();
    }

    static void LogWrite(TestString const& msg)
    {
        if (GetLogger() != NULL)
            GetLogger()->Write(msg);
    }

    static void LogWriteLine()
    {
        if (GetLogger() != NULL)
            GetLogger()->WriteLine("");
    }


    static void LogWriteLine(TestString const& msg)
    {
        if (GetLogger() != NULL)
            GetLogger()->WriteLine(msg);
    }

    static TestLog* GetLogger()
    {
        return GetSharedData().GetLogger();
    }

    static void SetLogger(TestLog* logger)
    {
        GetSharedData().SetLogger(logger);
    }

    TestTime const& GetStartTime() const throw()
    {
        return m_start_time;
    }

    TestTime const& GetSampleTime() const throw()
    {
        return m_sample_time;
    }

    TestTime const& GetCurrentTime() const throw()
    {
        return m_current_time;
    }

    void LogTests()
    {
        if (m_logged)
            return;

        m_logged = true;

        privateLogFunction();

        // Ensure tear down happens before leak checking stops.
        Teardown();

        LogLeaks();
    }

    void LogLeaks()
    {
        privateStopLeakChecking();
        privateLogLeaks();
    }

    template<typename T>
    static void BreakOn(T value)
    {
        TestMemoryLeakCheck::BreakOn<T>(value);
    }

// Check functions
    void CheckTrue(TestString const& expression,
                   TestString const& filename,
                   ocl_size_type line_number,
                   bool value)
    {
        LogCheck(expression, filename, line_number, !value);
    }

    void CheckFalse(TestString const& expression,
                    TestString const& filename,
                    ocl_size_type line_number,
                    bool value)
    {
        LogCheck(expression, filename, line_number, value);
    }

    void CheckNull(TestString const& expression,
                   TestString const& filename,
                   ocl_size_type line_number,
                   void const* ptr)
    {
        LogCheck(expression, filename, line_number, ptr != NULL);
    }

    void CheckNotNull(TestString const& expression,
                      TestString const& filename,
                      ocl_size_type line_number,
                      void const* ptr)
    {
        LogCheck(expression, filename, line_number, ptr == NULL);
    }

    void CheckException(TestString const& expression,
                        TestString const& filename,
                        ocl_size_type line_number,
                        bool found_exception,
                        bool expect_exception)
    {
        bool error = (expect_exception && !found_exception) || (!expect_exception && found_exception);
        LogCheck(expression, filename, line_number, error);
    }

    template<typename CharType>
    void CheckStrCmp(TestString const& expression,
                    TestString const& filename,
                    ocl_size_type line_number,
                    CharType const* str1,
                    CharType const* str2)
    {
        bool error = StrCmp(str1, str2) != 0;
        LogCheck(expression, filename, line_number, error);
    }

    template<typename CharType>
    void CheckNotStrCmp(TestString const& expression,
                    TestString const& filename,
                    ocl_size_type line_number,
                    CharType const* str1,
                    CharType const* str2)
    {
        bool error = StrCmp(str1, str2) == 0;
        LogCheck(expression, filename, line_number, error);
    }

    template<typename T1, typename T2>
    void CheckEqual(TestString const& expression,
                    TestString const& filename,
                    ocl_size_type line_number,
                    T1 const& value1,
                    T2 const& value2)
    {
        bool is_equal = value1 == value2;
        LogCheck(expression, filename, line_number, !is_equal);
    }

    template<typename T1, typename T2>
    void CheckNotEqual(TestString const& expression,
                       TestString const& filename,
                       ocl_size_type line_number,
                       T1 const& value1,
                       T2 const& value2)
    {
        bool is_not_equal = value1 != value2;
        LogCheck(expression, filename, line_number, !is_not_equal);
    }

    template<typename T1, typename T2>
    void CheckGreater(TestString const& expression,
                      TestString const& filename,
                      ocl_size_type line_number,
                      T1 const& value1,
                      T2 const& value2)
    {
        bool is_greater = value1 > value2;
        LogCheck(expression, filename, line_number, !is_greater);
    }

    template<typename T1, typename T2>
    void CheckGreaterEqual(TestString const& expression,
                           TestString const& filename,
                           ocl_size_type line_number,
                           T1 const& value1,
                           T2 const& value2)
    {
        bool is_greater_equal = value1>= value2;
        LogCheck(expression, filename, line_number, !is_greater_equal);
    }

    template<typename T1, typename T2>
    void CheckLess(TestString const& expression,
                   TestString const& filename,
                   ocl_size_type line_number,
                   T1 const& value1,
                   T2 const& value2)
    {
        bool is_less = value1 < value2;
        LogCheck(expression, filename, line_number, !is_less);
    }

    template<typename T1, typename T2>
    void CheckLessEqual(TestString const& expression,
                        TestString const& filename,
                        ocl_size_type line_number,
                        T1 const& value1,
                        T2 const& value2)
    {
        bool is_less_equal = value1 <= value2;
        LogCheck(expression, filename, line_number, !is_less_equal);
    }

    template<typename T>
    void CheckZero(TestString const& expression,
                   TestString const& filename,
                   ocl_size_type line_number,
                   T value)
    {
        bool is_equal = value == static_cast<T>(0);
        LogCheck(expression, filename, line_number, !is_equal);
    }

    template<typename T>
    void CheckNotZero(TestString const& expression,
                      TestString const& filename,
                      ocl_size_type line_number,
                      T value)
    {
        bool is_not_equal = value != static_cast<T>(0);
        LogCheck(expression, filename, line_number, !is_not_equal);
    }

    /// Compare contents of char*, wchar_t* or primitive values.
    template<typename T1, typename T2>
    void CheckCompare(TestString const& expression,
                      TestString const& filename,
                      ocl_size_type line_number,
                      T1 const& value1,
                      T2 const& value2)
    {
        int compare = TestCompare<T1, T2>::Compare(value1, value2);
        LogCheck(expression, filename, line_number, compare == 0);
    }

    /// Compare contents of char*, wchar_t* or primitive values.
    template<typename T1, typename T2>
    void CheckNotCompare(TestString const& expression,
                         TestString const& filename,
                         ocl_size_type line_number,
                         T1 const& value1,
                         T2 const& value2)
    {
        int compare = TestCompare<T1, T2>::Compare(value1, value2);
        LogCheck(expression, filename, line_number, compare != 0);
    }

    /// While the current time has not reached the start time + sample time,
    /// keep returning false.
    /// @note This function also refreshes the current time.
    bool CheckTime()
    {
        TestTime stop_time(m_start_time);
        stop_time += m_sample_time;

        m_current_time.Refresh();

        ++m_timed_function_calls;

        return m_current_time > stop_time;
    }

    /// While the current time has not reached the start time + sample time,
    /// keep returning false, or return false when the performance has been
    /// detected as too slow.
    /// @note This function also refreshes the current time.
    bool CheckTime(ocl_size_type min_iterations, TestString const& filename, ocl_size_type line_number)
    {
        TestTime stop_time(m_start_time);
        stop_time += m_sample_time;

        m_current_time.Refresh();

        ++m_timed_function_calls;

        bool time_complete = m_current_time > stop_time;

        if (time_complete && (m_timed_function_calls < min_iterations))
        {
            ++m_failure_check_count;
            TestString error;
            error.Append(m_timed_function_calls);
            error.Append(" iterations less than expected ");
            error.Append(min_iterations);
            error.Append(" iterations");
            privateSetFilename(filename);
            privateCheckFailed(error, line_number);
            return true;
        }

        return time_complete;
    }

// Shared data helper functions
    static bool HasSharedFailure() throw()
    {
        return (GetSharedData().GetTotalFailedTests() > 0) ||
               (GetSharedData().GetTotalLeakedTests() > 0);
    }

// Unit test helper functions.
// NOTE: These are protected.
#ifndef OCL_TEST_HELPERS_DISABLED
protected:
    template<typename CharType>
    static ocl_size_type StrLen(CharType const* str)
    {
        return TestStringUtility::UnsafeLength(str);
    }

    template<typename CharType>
    static CharType const* StrEnd(CharType const* str)
    {
        return TestStringUtility::StrEnd(str);
    }

    template<typename CharType>
    static CharType* StrCpy(CharType* dest, CharType const* src)
    {
        return TestStringUtility::UnsafeCopy(dest, src);
    }

    template<typename CharType>
    static int StrCmp(CharType const* str1, CharType const* str2)
    {
        return TestStringUtility::UnsafeCompare(str1, str2);
    }

    template<typename CharType>
    static ocl_size_type CharCount(CharType const* str, CharType char_to_find)
    {
        return TestStringUtility::UnsafeCharCount(str, char_to_find);
    }

    template<typename CharType>
    static ocl_size_type CharCount(CharType const* str, CharType const* chars_to_find)
    {
        return TestStringUtility::UnsafeCharCount(str, chars_to_find);
    }

    template<typename SizeType>
    static int MemCmp(void const* ptr1, void const* ptr2, SizeType size)
    {
        return ::memcmp(ptr1, ptr2, static_cast<ocl_size_type>(size));
    }

    template<typename Type, typename SizeType>
    static void MemSet(Type* ptr, SizeType count, Type value)
    {
        for (Type* ptr_end = ptr + count; ptr < ptr_end; ++ptr)
            *ptr = value;
    }

    template<typename CharType>
    static bool IsDigit(CharType ch)
    {
        return TestIntegerUtility::IsDigit(ch);
    }

    template<typename CharType>
    static bool IsMinus(CharType ch)
    {
        return TestIntegerUtility::IsMinus(ch);
    }

    template<typename IntType>
    static IntType GetMinusSign(bool want_negative)
    {
        return TestIntegerUtility::GetMinusSign<IntType>(want_negative);
    }

    /// @note The first template type is int as this cannot be deduced for the return,
    /// but the char type can be deduced so can be omitted.
    template<typename IntType, typename CharType>
    static IntType ToInt(CharType ch)
    {
        return TestIntegerUtility::ToInt<IntType>(ch);
    }

    template<typename CharType, typename IntType, typename SizeType>
    static bool ToInt(CharType const* str, IntType& value, SizeType& error_pos)
    {
        return TestIntegerUtility::ToInt<CharType, IntType, SizeType>(str, value, error_pos);
    }

    template<typename CharType, typename IntType>
    static bool ToInt(CharType const* str, IntType& value)
    {
        return TestIntegerUtility::ToInt<CharType, IntType>(str, value);
    }

    template<typename CharType, typename IntType>
    static IntType ToInt(CharType const* str)
    {
        return TestIntegerUtility::ToInt<CharType, IntType>(str);
    }

    static void Sleep(unsigned long milliseconds)
    {
        ocl::TestTime::Sleep(milliseconds);
    }
#endif // ifmdef OCL_TEST_HELPERS_DISABLED

    template<typename T>
    static T Max(T value1, T value2)
    {
        return value1 > value2 ? value1 : value2;
    }

protected:
    // Log check function used by all check macros.
    void LogCheck(TestString const& expression,
                  TestString const& filename,
                  ocl_size_type line_number,
                  bool failed)
    {
        privateSetFilename(filename);

        if (failed)
        {
            ++m_failure_check_count;
            privateCheckFailed(expression, line_number);
        }

        ++m_check_count;
        GetSharedData().IncTotalChecks();
    }

    void Clear()
    {
        // Clear any data that is dynamically allocated so the leak detection
        // does not consider this as a memory leak within the test.
        m_filename.Clear();
        m_class_name.Clear();
        m_function_name.Clear();
        m_args.Clear();
        m_check_failures.Clear();
    }

// Memory leak private helpers.
private:
    void privateStopLeakChecking()
    {
        // Clear any dynamically allocated memory within this test before
        // identifying any leaks in unit tests.
        Clear();

        m_leak_check.Stop();
    }

    void privateLogLeaks()
    {
        if (m_leak_check.IsLeaking())
        {
            GetSharedData().IncTotalLeakedTests();
            TestString msg(' ', error_padding);
            msg.Append("Memory leak detected!");
            LogWriteLine(msg);
        }
    }

// Log reporting private helpers.
private:
    // Log the whole line for a function or member function.
    void privateLogFunction()
    {
        privateLogFunctionLineNumber();

        privateLogTestStatus();

        // Output summary after function name.
        if (m_is_timed)
            privateLogTime();
        else
            privateLogNumberOfRunTests();

        // Output function or member function name.
        LogWriteLine(GetTestName());

        // Output any error lines and expressions.
        if (HasFailed())
            privateLogFailures();
    }

    // Log the current tested function with a line number.
    void privateLogFunctionLineNumber()
    {
        TestString str, line_number_str;

        GetSharedData().IncLoggedLine();
        line_number_str.Append(static_cast<ocl_size_type>(GetSharedData().GetLoggedLine()));
        if (line_number_str.GetLength() < max_digits)
            str = TestString(' ', max_digits - line_number_str.GetLength());
        str += "(";
        str += line_number_str;
        str += ") ";

        LogWrite(str);
    }

    // Output SUCCESS, FAILED, NOT RUN or TIMED message.
    void privateLogTestStatus()
    {
        // Output the first part of the log line.
        if (HasTests())
        {
            if (!IsGeneralTest())
                GetSharedData().IncTotalFunctionsTested();
            if (HasFailed() || m_leak_check.IsLeaking())
                LogWrite(GetSharedData().GetFailedMessage());
            else
                LogWrite(GetSharedData().GetSuccessMessage());
        }
        else if (m_is_timed)
        {
            GetSharedData().IncTotalTimedFunctions();
            LogWrite(GetSharedData().GetTimedMessage());
        }
        else
        {
            GetSharedData().IncTotalNotTested();
            LogWrite(GetSharedData().GetNotRunMessage());
        }
    }

    // Appends a time to a string in seconds and milliseconds or nanoseconds.
    static void privateAppendTime(TestString& str, TestTime const& ttm, bool milliseconds)
    {
        str.Append(ttm.GetSeconds());
        str.Append(".");
        if (milliseconds)
            str.Append(ttm.GetMilliseconds(), TestTime::MILLISECONDS_CHARS);
        else
            str.Append(ttm.GetNanoseconds(), TestTime::NANOSECONDS_CHARS);
    }

    // Extra helper function for more timing info.
    void privateAppendExtraTimings(TestString& str, TestTime& diff_time)
    {
        str.Append("elapsed ");
        privateAppendTime(str, diff_time, true);
        str.Append(". calls ");
        str.Append(static_cast<unsigned long>(m_timed_function_calls));
        str.Append(". ");
    }

    // The three variations that can be logged for a test.
    void privateLogTime()
    {
        TestTime diff_time;
        m_current_time.GetDiffTime(m_start_time, diff_time);

        TestString msg;

        // Enable if more statistics is required.
        // privateAppendExtraTimings(msg, diff_time);

        msg.Append("time ");
        if (m_timed_function_calls > 0)
        {
            diff_time /= static_cast<size_t>(m_timed_function_calls);
            privateAppendTime(msg, diff_time, false);
        }
        else
            msg.Append("?"); // Too fast too get accurate timing.

        LogWrite(msg);
    }

    // Log the part that follows the function name.
    void privateLogNumberOfRunTests()
    {
        char const* str_test     = "TEST  ";
        char const* str_tests    = "TESTS ";
        char const* str_failed   = "FAILED";
        char const* str_no_tests = "* NO TESTS *";

        TestString::size_type const max_chars = 
            Max(max_digits + TestStringUtility::UnsafeLength(str_test) + 1,
                TestStringUtility::UnsafeLength(str_test)) + 1;

        // Build up the string with number of tests or failed tests, then appended message.
        TestString whole_str;
        TestString msg_str;

        if (m_check_count > 0)
        {
            if (m_failure_check_count > 0)
            {
                whole_str.Append(static_cast<unsigned long>(m_failure_check_count));
                msg_str.Append(str_failed);
            }
            else
            {
                whole_str.Append(static_cast<unsigned long>(m_check_count));
                msg_str.Append((m_check_count > 1) ? str_tests : str_test);
            }
            whole_str.PadLeft(' ', max_digits);
            whole_str.Append(" ");
        }
        else
            msg_str.Append(str_no_tests);

        whole_str.Append(msg_str);
        whole_str.PadRight(' ', max_chars);

        LogWrite(whole_str);
    }

    // Log the output for a failed check macro.
    void privateLogFailures()
    {
        TestString const indent(' ', error_padding);

        TestString msg;

        if (!m_filename.IsEmpty())
        {
            msg.Append(indent);
            msg.Append("FILE: ");
            msg.Append(m_filename);
            LogWriteLine(msg);
            msg.Clear();
        }

        // Extract each line from the string until there are no more failed checks.
        while (!m_check_failures.IsEmpty())
        {
            ocl_size_type pos = 0;
            if (m_check_failures.Find('\n', pos))
            {
                m_check_failures.GetSubString(msg, 0, pos + 1, true);
                msg.Prepend(indent);
                LogWrite(msg);
            }
            else
            {
                m_check_failures.Prepend(indent);
                LogWrite(m_check_failures);
                m_check_failures.Clear();
            }
        }
    }

    // Log the number of checks for a tested function, e.g. 5 TESTS or 1 TEST.
    static void privateLogCount(TestString const& msg, ocl_size_type count)
    {
        TestString count_msg(msg);
        count_msg.Append(" = ");
        count_msg.Append(static_cast<unsigned long>(count));
        if (GetLogger() != NULL)
            GetLogger()->WriteLine(count_msg);
    }

    // Output the final summary report.
    static void privateLogSharedData()
    {
        if (GetLogger() != NULL)
        {
            GetLogger()->WriteEOL();
            privateLogCount("Total checks", GetSharedData().GetTotalChecks());
            if (GetSharedData().GetTotalNotTested() > 0)
                privateLogCount("Total not tested", GetSharedData().GetTotalNotTested());
            if (HasSharedFailure())
                privateLogCount("Total failed tests", GetSharedData().GetTotalFailedTests());
            privateLogCount("Total functions tested", GetSharedData().GetTotalFunctionsTested());
            if (GetSharedData().GetTotalTimedFunctions() > 0)
                privateLogCount("Total functions timed", GetSharedData().GetTotalTimedFunctions());
            privateLogCount("Total tests", GetSharedData().GetTotalTests());
            if (GetSharedData().GetTotalLeakedTests() > 0)
                privateLogCount("Total memory leaks", GetSharedData().GetTotalLeakedTests());
        }
    }

    // This data needs to be set after leak checking so that
    // dynamic data is not falsely reported as a memory leak.
    void privateSetupPostLeakCheckingData(char const* class_name,
                                          char const* function_name,
                                          char const* args)
    {
        SetClassName(class_name);
        SetFunctionName(function_name);
        SetArgs(args);
    }

    // Keep track of constructions,
    // totals and current test number for every test.
    void privateIncrementCounters()
    {
        GetSharedData().IncConstructions();
        GetSharedData().IncTotalTests();

        m_test_number = GetSharedData().GetConstructions();
    }

    void privateSetFilename(TestString const& filename)
    {
        m_filename = filename;
    }

    void privateCheckFailed(TestString const& expression,
                            ocl_size_type line_number)
    {
        if (line_number > 0)
        {
            GetSharedData().IncTotalFailedTests();
            m_check_failures.Append("LINE: ");
            m_check_failures.Append(static_cast<unsigned long>(line_number));
            m_check_failures.Append("\n");
        }
        if ((expression != NULL) && (*expression != '\0'))
        {
            m_check_failures.Append("EXPRESSION: ");
            m_check_failures.Append(expression);
            m_check_failures.Append("\n");
        }
    }

private:
    static TestClassSharedData<ocl_size_type>& GetSharedData()
    {
        // Ensure shared data is available for first use.
        static TestClassSharedData<ocl_size_type> shared_data;
        return shared_data;
    }

    // Because all constructors are called first before main,
    // then all destructor(s) get called after main,
    // the snapshot needs to be completed for the previous constructor.
    TestClass*& GetPreviousTest()
    {
        static TestClass* prev_test = NULL;
        return prev_test;
    }

    static TestSetupTeardown& privateSetupTeardown()
    {
        static TestSetupTeardown setup_teardown;
        return setup_teardown;
    }

// Data for this test.
private:
    TestMemoryLeakCheck m_leak_check;

    TestString m_filename;
    TestString m_class_name;
    TestString m_function_name;
    TestString m_args;
    bool m_is_const;
    bool m_is_timed;

    // When this test is timed, record the start time.
    TestTime m_start_time;

    // Timer used to check progress in the sample time period.
    TestTime m_current_time;
    ocl_size_type m_timed_function_calls;

    // The amount of time to run the performance test.
    TestTime m_sample_time;

    // Total checks for this function.
    ocl_size_type m_check_count;

    // Test failures for this function.
    // Only record check or timing failures as these have specific tests.
    // Memory leaks do not affect this failure count.
    ocl_size_type m_failure_check_count;
    TestString m_check_failures;

    // Debug test number helper for identifying when a test crashes.
    ocl_size_type m_test_number;

    // Only log test results once.
    bool m_logged;

private:
    TestClass(TestClass const&);
    TestClass& operator =(TestClass const&);
};

} // namespace ocl

#endif // OCL_GUARD_TEST_TESTCLASS_HPP
