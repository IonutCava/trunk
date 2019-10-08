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

#ifndef OCL_GUARD_TEST_TESTCLASSSHAREDDATA_HPP
#define OCL_GUARD_TEST_TESTCLASSSHAREDDATA_HPP

#include "StdioTestLog.hpp"
#include "TestString.hpp"
#include <cstddef>

namespace ocl
{

/**
 * All shared data for every test which is derived from TestClass
 * is in TestClassSharedData class.
 */
template<typename SizeType>
class TestClassSharedData
{
public:
    TestClassSharedData()
        : m_logger(NULL)
        , m_constructions(0)
        , m_logged_line(0)
        , m_total_checks(0)
        , m_total_not_tested(0)
        , m_total_failed_tests(0)
        , m_total_leaked_tests(0)
        , m_total_functions_tested(0)
        , m_total_timed_functions(0)
        , m_total_tests(0)

        // Padded to be lined up for output as columns.
        , m_success_message("SUCCESS - ")
        , m_failed_message (" FAILED - ")
        , m_not_run_message("NOT RUN - ")
        , m_timed_message  ("  TIMED - ")
    {
    }

    ~TestClassSharedData()
    {
        delete m_logger;
    }

    TestLog* GetLogger()
    {
        if (m_logger == NULL)
            m_logger = new StdioTestLog;
        return m_logger;
    }

    void SetLogger(TestLog* logger)
    {
        delete m_logger;
        m_logger = logger;
    }

    void Clear()
    {
        SetLogger(NULL);
    }

    bool IsLast() const
    {
        return m_destructions >= m_constructions;
    }

    SizeType GetConstructions() const throw()
    {
        return m_constructions;
    }

    void SetConstructions(SizeType constructions) throw()
    {
        m_constructions = constructions;
    }

    void IncConstructions()
    {
        ++m_constructions;
    }

    SizeType GetDestructions() const throw()
    {
        return m_destructions;
    }

    void SetDestructions(SizeType destructions) throw()
    {
        m_destructions = destructions;
    }

    void IncDestructions()
    {
        ++m_destructions;
    }

    SizeType GetLoggedLine() const throw()
    {
        return m_logged_line;
    }

    void SetLoggedLine(SizeType logged_line) throw()
    {
        m_logged_line = logged_line;
    }

    void IncLoggedLine()
    {
        ++m_logged_line;
    }

    SizeType GetTotalChecks() const throw()
    {
        return m_total_checks;
    }

    void SetTotalChecks(SizeType total_checks) throw()
    {
        m_total_checks = total_checks;
    }

    void IncTotalChecks()
    {
        ++m_total_checks;
    }

    SizeType GetTotalNotTested() const throw()
    {
        return m_total_not_tested;
    }

    void SetTotalNotTested(SizeType total_not_tested) throw()
    {
        m_total_not_tested = total_not_tested;
    }

    void IncTotalNotTested()
    {
        ++m_total_not_tested;
    }

    SizeType GetTotalFailedTests() const throw()
    {
        return m_total_failed_tests;
    }

    void SetTotalFailedTests(SizeType total_failed_tests) throw()
    {
        m_total_failed_tests = total_failed_tests;
    }

    void IncTotalFailedTests()
    {
        ++m_total_failed_tests;
    }

    SizeType GetTotalLeakedTests() const throw()
    {
        return m_total_leaked_tests;
    }

    void SetTotalLeakedTests(SizeType total_leaked_tests) throw()
    {
        m_total_leaked_tests = total_leaked_tests;
    }

    void IncTotalLeakedTests()
    {
        ++m_total_leaked_tests;
    }

    SizeType GetTotalFunctionsTested() const throw()
    {
        return m_total_functions_tested;
    }

    void SetTotalFunctionsTested(SizeType total_functions_tested) throw()
    {
        m_total_functions_tested = total_functions_tested;
    }

    void IncTotalFunctionsTested()
    {
        ++m_total_functions_tested;
    }

    SizeType GetTotalTimedFunctions() const throw()
    {
        return m_total_timed_functions;
    }

    void SetTotalTimedFunctions(SizeType total_timed_functions) throw()
    {
        m_total_timed_functions = total_timed_functions;
    }

    void IncTotalTimedFunctions()
    {
        ++m_total_timed_functions;
    }

    SizeType GetTotalTests() const throw()
    {
        return m_total_tests;
    }

    void SetTotalTests(SizeType total_tests) throw()
    {
        m_total_tests = total_tests;
    }

    char const* GetClassName() const throw()
    {
        return m_class_name;
    }

    void SetClassName(char const* class_name) throw()
    {
        m_class_name = class_name;
    }

    bool IsClassNameSame(char const* class_name) const
    {
        return TestString(m_class_name) == class_name;
    }

    void IncTotalTests()
    {
        ++m_total_tests;
    }

    const char* GetSuccessMessage() const throw()
    {
        return m_success_message;
    }

    const char* GetFailedMessage() const throw()
    {
        return m_failed_message;
    }

    const char* GetNotRunMessage() const throw()
    {
        return m_not_run_message;
    }

    const char* GetTimedMessage() const throw()
    {
        return m_timed_message;
    }

private:
    TestClassSharedData(TestClassSharedData const&);
    TestClassSharedData& operator=(TestClassSharedData const&);

// Data only available to TestClass.
private:
    // Default logger when one is not provided.
    TestLog* m_logger;

    // Count all constructions.
    SizeType m_constructions;

    // Count all the destructions, and check constructions match destructions.
    SizeType m_destructions;

    // Currently logged line number for the run test.
    SizeType m_logged_line;

    // Counts for all tests.
    SizeType m_total_checks;
    SizeType m_total_not_tested;
    SizeType m_total_failed_tests;
    SizeType m_total_leaked_tests;
    SizeType m_total_functions_tested;
    SizeType m_total_timed_functions;
    SizeType m_total_tests;

    // Last class name set for TestClass.
    char const* m_class_name;

    // message for start of each test.
    const char* m_success_message;
    const char* m_failed_message;
    const char* m_not_run_message;
    const char* m_timed_message;
};

} // namespace ocl

#endif // OCL_GUARD_TEST_TESTCLASSSHAREDDATA_HPP
