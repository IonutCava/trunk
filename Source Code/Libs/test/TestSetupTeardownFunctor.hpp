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

#ifndef OCL_GUARD_TEST_TESTSETUPTEARDOWNFUNCTOR_HPP
#define OCL_GUARD_TEST_TESTSETUPTEARDOWNFUNCTOR_HPP

namespace ocl
{

class TestSetupTeardownFunctor
{
public:
    TestSetupTeardownFunctor(char const* class_name, bool is_setup)
        : m_class_name(class_name)
        , m_is_setup(is_setup)
    {
    }

    char const* GetClassName() const throw()
    {
        return m_class_name;
    }

    bool IsSetup() const throw()
    {
        return m_is_setup;
    }

    // Implemented by macro in derived class.
    virtual void Execute() = 0;

private:
    char const* m_class_name;
    bool m_is_setup;
};

}

#endif // OCL_GUARD_TEST_TESTSETUPTEARDOWNFUNCTOR_HPP
