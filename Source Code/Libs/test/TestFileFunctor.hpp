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

#ifndef OCL_GUARD_TEST_TESTFILEFUNCTOR_HPP
#define OCL_GUARD_TEST_TESTFILEFUNCTOR_HPP

#include <fstream>

namespace ocl
{

struct TestFileFunctor
{
    TestFileFunctor(char const* file_name)
    {
        m_ostream.open(file_name, std::ofstream::out | std::ofstream::app);
    }

    ~TestFileFunctor()
    {
        m_ostream.close();
    }

    /// This functor is called from FunctorTestLog::Write.
    void operator()(char const* str)
    {
        m_ostream << str;
    }

private:
    TestFileFunctor(TestFileFunctor const&);
    TestFileFunctor& operator=(TestFileFunctor const&);

private:
    std::ofstream m_ostream;
};

} // namespace ocl

#endif // OCL_GUARD_TEST_TESTFILEFUNCTOR_HPP
