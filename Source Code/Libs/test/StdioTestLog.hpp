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

#ifndef OCL_GUARD_TEST_STDIOTESTLOG_HPP
#define OCL_GUARD_TEST_STDIOTESTLOG_HPP

#include "TestLog.hpp"
#include <iostream>

namespace ocl
{

class StdioTestLog : public TestLog
{
public:
    virtual void Write(TestString const& str)
    {
        TestString::size_type len = str.GetLength();
        bool eol = (len > 0) && (str[len-1] == '\n');
        if (eol)
        {
            if (len > 1)
            {
                TestString sub_str = str.GetSubString(0, len - 1);
                std::cout << sub_str.Ptr();
            }
            WriteEOL();
        }
        else
            std::cout << str.Ptr();
    }

    virtual void WriteEOL()
    {
        std::cout << std::endl;
    }
};

}

#endif // OCL_GUARD_TEST_STDIOTESTLOG_HPP
