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

*******************************************************************************

Extend class TestClass into easy to use MACROS which support testing functions
and member functions, as well as providing check functions.

E.g.

TEST_MEMBER_FUNCTION(MyString, SetSize, size_t)
{
    MyString str;
    CHECK_TRUE(str.SetSize(1));
}
*/

#ifndef OCL_GUARD_TEST_TESTMACROS_HPP
#define OCL_GUARD_TEST_TESTMACROS_HPP

/// NOTE: DO NOT include TestClass.hpp, as this is done as part of Test.hpp.

#include "TestString.hpp"
#include "FunctorTestLog.hpp"
#include "TestFileFunctor.hpp"
#include "TestStdioFileFunctor.hpp"
#include "TestSetupTeardownFunctor.hpp"

#define TEST_SETUP_TEARDOWN(Type, is_setup, class_name) \
    class TestSetupTeardownFunctor_##Type##_##class_name : public ocl::TestSetupTeardownFunctor \
    { \
        private: \
            TestSetupTeardownFunctor_##Type##_##class_name(TestSetupTeardownFunctor_##Type##_##class_name const&); \
            TestSetupTeardownFunctor_##Type##_##class_name& operator=(TestSetupTeardownFunctor_##Type##_##class_name const&); \
        public: \
            TestSetupTeardownFunctor_##Type##_##class_name(char const* name = #class_name) : \
                ocl::TestSetupTeardownFunctor(name, is_setup) \
            { \
                if (IsSetup()) \
                    ocl::TestClass::SetSetup(*this); \
                else \
                    ocl::TestClass::SetTeardown(*this); \
            } \
            void Execute(); \
    } TestSetupTeardownFunctor_##Type##_##class_name##_instance; \
    void TestSetupTeardownFunctor_##Type##_##class_name::Execute()

#define TEST_SETUP(class_name) TEST_SETUP_TEARDOWN(Setup, true, class_name)

#define TEST_TEARDOWN(class_name)  TEST_SETUP_TEARDOWN(Teardown, false, class_name)

#ifndef TEST
#define TEST(name) \
class TestGeneral_##name : public ocl::TestClass \
{ \
private: \
    TestGeneral_##name(TestGeneral_##name const&); \
    TestGeneral_##name& operator=(TestGeneral_##name const&); \
public: \
    TestGeneral_##name(); \
} g_TestGeneral_##name; \
TestGeneral_##name::TestGeneral_##name() \
    : ocl::TestClass(#name, "", "")
#else
#error Unit test conflict with other macro!
#endif // TEST_FUNCTION

#ifndef TEST_FUNCTION
#define TEST_FUNCTION(function_name, args) \
class Test_##function_name##_##args : public ocl::TestClass \
{ \
private: \
    Test_##function_name##_##args(Test_##function_name##_##args const&); \
    Test_##function_name##_##args& operator=(Test_##function_name##_##args const&); \
public: \
    Test_##function_name##_##args(); \
} g_Test_##function_name##_##args; \
Test_##function_name##_##args::Test_##function_name##_##args() \
    : ocl::TestClass("", #function_name, #args)
#else
#error Unit test conflict with other macro!
#endif // TEST_FUNCTION

#ifndef TEST_FUNCTION_TIME
#define TEST_FUNCTION_TIME(function_name, args, secs, millisecs) \
class TimedTest_##function_name##_##args : public ocl::TestClass \
{ \
private: \
    TimedTest_##function_name##_##args(TimedTest_##function_name##_##args const&); \
    TimedTest_##function_name##_##args& operator=(TimedTest_##function_name##_##args const&); \
public: \
    TimedTest_##function_name##_##args(); \
} g_TimedTest_##function_name##_##args; \
TimedTest_##function_name##_##args::TimedTest_##function_name##_##args() \
    : ocl::TestClass("", #function_name, #args, false, true, secs, millisecs * ocl::TestTime::MICROSECONDS_PER_SECOND)
#else
#error Unit test conflict with other macro!
#endif // TEST_FUNCTION_TIME

#ifndef TEST_MEMBER_FUNCTION
#define TEST_MEMBER_FUNCTION(class_name, function_name, args) \
class Test_##class_name##_##function_name##_##args : public ocl::TestClass \
{ \
private: \
    Test_##class_name##_##function_name##_##args(Test_##class_name##_##function_name##_##args const&); \
    Test_##class_name##_##function_name##_##args& operator=(Test_##class_name##_##function_name##_##args const&); \
public: \
    Test_##class_name##_##function_name##_##args(); \
} g_Test_##class_name##_##function_name##_##args; \
Test_##class_name##_##function_name##_##args::Test_##class_name##_##function_name##_##args() \
    : ocl::TestClass(#class_name, #function_name, #args)
#else
#error Unit test conflict with other macro!
#endif // TEST_MEMBER_FUNCTION

#ifndef TEST_MEMBER_FUNCTION_TIME
#define TEST_MEMBER_FUNCTION_TIME(class_name, function_name, args, secs, millisecs) \
class TimedTest_##class_name##_##function_name##_##args : public ocl::TestClass \
{ \
private: \
    TimedTest_##class_name##_##function_name##_##args(TimedTest_##class_name##_##function_name##_##args const&); \
    TimedTest_##class_name##_##function_name##_##args& operator=(TimedTest_##class_name##_##function_name##_##args const&); \
public: \
    TimedTest_##class_name##_##function_name##_##args(); \
} g_TimedTest_##class_name##_##function_name##_##args; \
TimedTest_##class_name##_##function_name##_##args::TimedTest_##class_name##_##function_name##_##args() \
    : ocl::TestClass(#class_name, #function_name, #args, false, true, secs, millisecs * ocl::TestTime::MICROSECONDS_PER_SECOND)
#else
#error Unit test conflict with other macro!
#endif // TEST_MEMBER_FUNCTION_TIME

#ifndef TEST_CONST_MEMBER_FUNCTION
#define TEST_CONST_MEMBER_FUNCTION(class_name, function_name, args) \
class Test_##class_name##_##function_name##_##args##_const : public ocl::TestClass \
{ \
private: \
    Test_##class_name##_##function_name##_##args##_const(Test_##class_name##_##function_name##_##args##_const const&); \
    Test_##class_name##_##function_name##_##args##_const& operator=(Test_##class_name##_##function_name##_##args##_const const&); \
public: \
    Test_##class_name##_##function_name##_##args##_const(); \
} g_Test_##class_name##_##function_name##_##args##_const; \
Test_##class_name##_##function_name##_##args##_const::Test_##class_name##_##function_name##_##args##_const() \
    : ocl::TestClass(#class_name, #function_name, #args, true)
#else
#error Unit test conflict with other macro!
#endif // TEST_CONST_MEMBER_FUNCTION

#ifndef TEST_CONST_MEMBER_FUNCTION_TIME
#define TEST_CONST_MEMBER_FUNCTION_TIME(class_name, function_name, args, secs, millisecs) \
class TimedTest_##class_name##_##function_name##_##args##_const : public ocl::TestClass \
{ \
private: \
    TimedTest_##class_name##_##function_name##_##args##_const(TimedTest_##class_name##_##function_name##_##args##_const const&); \
    TimedTest_##class_name##_##function_name##_##args##_const& operator=(TimedTest_##class_name##_##function_name##_##args##_const const&); \
public: \
    TimedTest_##class_name##_##function_name##_##args##_const(); \
} g_TimedTest_##class_name##_##function_name##_##args##_const; \
TimedTest_##class_name##_##function_name##_##args##_const::TimedTest_##class_name##_##function_name##_##args##_const() \
    : ocl::TestClass(#class_name, #function_name, #args, true, true, secs, millisecs * ocl::TestTime::MICROSECONDS_PER_SECOND)
#else
#error Unit test conflict with other macro!
#endif // TEST_CONST_MEMBER_FUNCTION_TIME

// Get true or false for any failed tests.
#define TEST_HAS_FAILED ocl::TestClass::HasSharedFailure()

// Cause debugger to break on a value (unique count for visual studio)
#define TEST_BREAK_ON_LEAK(value) ocl::TestClass::BreakOn(value)

/*
Override the function name passed into TEST_FUNCTION or TEST_MEMBER_FUNCTION.
This will update the function name that appears in the results output.
E.g.

TEST_MEMBER_FUNCTION(MyString, operator_plus_equal, char)
{
    TEST_OVERRIDE_FUNCTION_NAME("operator +=");

    MyString str;
    str += 'a';
    CHECK_EQUAL(str.GetLength(), 0U);
}
*/
#ifndef TEST_OVERRIDE_FUNCTION_NAME
#define TEST_OVERRIDE_FUNCTION_NAME(function_name) SetFunctionName(function_name)
#else
#error Unit test conflict with other macro!
#endif


/*
Override the function argument(s) passed into TEST_FUNCTION or TEST_MEMBER_FUNCTION.
This will update the function arguments that appears in the results output.
E.g.

TEST_MEMBER_FUNCTION(MyString, Append, char_const_ptr)
{
    TEST_OVERRIDE_ARGS("char const*");

    MyString str;
    CHECK_TRUE(str.Append("hello"));
}
*/
#ifndef TEST_OVERRIDE_ARGS
#define TEST_OVERRIDE_ARGS(args) SetArgs(args)
#else
#error Unit test conflict with other macro!
#endif


/*
Override the function name and arguments passed into TEST_FUNCTION or TEST_MEMBER_FUNCTION.
This will update the function name and arguments that appears in the results output.
E.g.

TEST_MEMBER_FUNCTION(MyString, operator_plus_equal, char)
{
    TEST_OVERRIDE_FUNCTION_NAME_ARGS("operator +=", "char");

    MyString str;
    str += 'a';
    CHECK_EQUAL(str.GetLength(), 0U);
}
*/
#ifndef TEST_OVERRIDE_FUNCTION_NAME_ARGS
#define TEST_OVERRIDE_FUNCTION_NAME_ARGS(function_name, args) SetFunctionName(function_name); SetArgs(args)
#else
#error Unit test conflict with other macro!
#endif


/*
Override default logger with a custom logger.
Functor class requires following following implementation:
struct MyFunctor
{
    void operator ()(char const* str) { my_log_code(str); }
};

Macros is used with this example like this:
TEST_OVERRIDE_LOG(MyFunctor, new MyFunctor());
*/

#ifndef TEST_OVERRIDE_LOG
#define TEST_OVERRIDE_LOG(functor_type, functor_ptr) ocl::TestClass::SetLogger(new ocl::FunctorTestLog<functor_type>(functor_ptr))
#endif

#ifndef TEST_OVERRIDE_STDIO_FILE_LOG
#define TEST_OVERRIDE_STDIO_FILE_LOG(file_name) \
    ocl::TestClass::SetLogger(new ocl::FunctorTestLog<ocl::TestStdioFileFunctor>(new ocl::TestStdioFileFunctor(file_name)))
#endif

#ifndef TEST_OVERRIDE_FILE_LOG
#define TEST_OVERRIDE_FILE_LOG(file_name) \
    ocl::TestClass::SetLogger(new ocl::FunctorTestLog<ocl::TestFileFunctor>(new ocl::TestFileFunctor(file_name)))
#endif

/**
    CHECK_* macros
 */

#ifndef CHECK_TRUE
#define CHECK_TRUE(expression) CheckTrue(#expression, __FILE__, (ocl::ocl_size_type)__LINE__, (expression))
#else
#error Unit test conflict with other macro!
#endif

#ifndef CHECK_FALSE
#define CHECK_FALSE(expression) CheckFalse(#expression, __FILE__, (ocl::ocl_size_type)__LINE__, (expression))
#else
#error Unit test conflict with other macro!
#endif

#ifndef CHECK_EQUAL
#define CHECK_EQUAL(value1, value2) CheckEqual(#value1 "==" #value2, __FILE__, (ocl::ocl_size_type)__LINE__, (value1), (value2))
#else
#error Unit test conflict with other macro!
#endif

#ifndef CHECK_NOT_EQUAL
#define CHECK_NOT_EQUAL(value1, value2) CheckNotEqual(#value1 "!=" #value2, __FILE__, (ocl::ocl_size_type)__LINE__, (value1), (value2))
#else
#error Unit test conflict with other macro!
#endif

#ifndef CHECK_GREATER
#define CHECK_GREATER(value1, value2) CheckGreater(#value1 ">=" #value2, __FILE__, (ocl::ocl_size_type)__LINE__, (value1), (value2))
#else
#error Unit test conflict with other macro!
#endif

#ifndef CHECK_GREATER_EQUAL
#define CHECK_GREATER_EQUAL(value1, value2) CheckGreaterEqual(#value1 ">" #value2, __FILE__, (ocl::ocl_size_type)__LINE__, (value1), (value2))
#else
#error Unit test conflict with other macro!
#endif

#ifndef CHECK_LESS
#define CHECK_LESS(value1, value2) CheckLess(#value1 "<" #value2, __FILE__, (ocl::ocl_size_type)__LINE__, (value1), (value2))
#else
#error Unit test conflict with other macro!
#endif

#ifndef CHECK_LESS_EQUAL
#define CHECK_LESS_EQUAL(value1, value2) CheckLessEqual(#value1 "<=" #value2, __FILE__, (ocl::ocl_size_type)__LINE__, (value1), (value2))
#else
#error Unit test conflict with other macro!
#endif

#ifndef CHECK_NULL
#define CHECK_NULL(value) CheckNull(#value "== NULL", __FILE__, (ocl::ocl_size_type)__LINE__, value)
#else
#error Unit test conflict with other macro!
#endif

#ifndef CHECK_NOT_NULL
#define CHECK_NOT_NULL(value) CheckNotNull(#value "!= NULL", __FILE__, (ocl::ocl_size_type)__LINE__, value)
#else
#error Unit test conflict with other macro!
#endif

#ifndef CHECK_ZERO
#define CHECK_ZERO(value) CheckZero(#value "== 0", __FILE__, (ocl::ocl_size_type)__LINE__, (value))
#endif

#ifndef CHECK_NOT_ZERO
#define CHECK_NOT_ZERO(value) CheckNotZero(#value "!= 0", __FILE__, (ocl::ocl_size_type)__LINE__, (value))
#endif

#ifndef CHECK_STRCMP
#define CHECK_STRCMP(str1, str2) CheckStrCmp("StrCmp(" #str1 "," #str2 ") == 0", __FILE__, (ocl::ocl_size_type)__LINE__, str1, str2)
#endif

#ifndef CHECK_NOT_STRCMP
#define CHECK_NOT_STRCMP(str1, str2) CheckNotStrCmp("StrCmp(" #str1 "," #str2 ") != 0", __FILE__, (ocl::ocl_size_type)__LINE__, str1, str2)
#endif

#ifndef CHECK_COMPARE
#define CHECK_COMPARE(value1, value2) CheckCompare("compare " #value1 " = " #value2, __FILE__, (ocl::ocl_size_type)__LINE__, value1, value2)
#endif

#ifndef CHECK_NOT_COMPARE
#define CHECK_NOT_COMPARE(value1, value2) CheckNotCompare("compare " #value1 " != " #value2, __FILE__, (ocl::ocl_size_type)__LINE__, (value1), (value2))
#endif

#define CHECK_EXCEPTION(expression, exception_type, expect_exception) \
{ \
    bool found_exception = false; \
    try \
    { \
        expression; \
    } \
    catch (exception_type&) \
    { \
        found_exception = true; \
    } \
    CheckException(#expression, __FILE__, (ocl::ocl_size_type)__LINE__, found_exception, expect_exception); \
}

#define CHECK_ALL_EXCEPTIONS(expression, expect_exception) \
{ \
    bool found_exception = false; \
    try \
    { \
        expression; \
    } \
    catch (...) \
    { \
        found_exception = true; \
    } \
    CheckException(#expression, __FILE__, (ocl::ocl_size_type)__LINE__, found_exception, expect_exception); \
}

#ifndef CHECK_TIME
#define CHECK_TIME(func) for (; !CheckTime(); ) func
#endif

#ifndef CHECK_PERFORMANCE
#define CHECK_PERFORMANCE(func, min_iterations) for (ocl::TestString filename(__FILE__); !CheckTime((ocl::ocl_size_type)min_iterations, filename, (ocl::ocl_size_type)__LINE__); ) func
#endif

#endif // OCL_GUARD_TEST_TESTMACROS_HPP
