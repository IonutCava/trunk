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

#ifndef OCL_GUARD_TEST_TESTSTDIOFILEFUNCTOR_HPP
#define OCL_GUARD_TEST_TESTSTDIOFILEFUNCTOR_HPP

#include <fstream>
#include <iostream>

namespace ocl
{

struct TestStdioFileFunctor
{
    TestStdioFileFunctor(char const* file_name)
        : m_ostream(file_name, std::ofstream::out | std::ofstream::app)
    {
    }

    ~TestStdioFileFunctor()
    {
        m_ostream.close();
    }

    /// This functor is called from FunctorTestLog::Write.
    void operator()(char const* str)
    {
        m_ostream << str;
        if (*str != '\n')
            std::cout << str;
        else
            std::cout << std::endl;
    }

private:
    TestStdioFileFunctor(TestStdioFileFunctor const&);
    TestStdioFileFunctor& operator=(TestStdioFileFunctor const&);

private:
    std::ofstream m_ostream;
};

} // namespace ocl

#endif // OCL_GUARD_TEST_TESTSTDIOFILEFUNCTOR_HPP
