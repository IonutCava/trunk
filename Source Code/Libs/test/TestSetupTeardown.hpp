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

#ifndef OCL_GUARD_TEST_TESTSETUPTEARDOWN_HPP
#define OCL_GUARD_TEST_TESTSETUPTEARDOWN_HPP

#include "TestSetupTeardownFunctor.hpp"
#include "TestString.hpp"
#include <cstddef>

namespace ocl
{

class TestSetupTeardown
{
public:
    TestSetupTeardown()
        : m_setup_functor(NULL)
        , m_teardown_functor(NULL)
        , m_setup_run(false)
    {
    }

    bool IsSetupSet() const throw()
    {
        return m_setup_functor != NULL;
    }

    bool IsTeardownSet() const throw()
    {
        return m_teardown_functor != NULL;
    }

    bool IsSetupTeardownSet() const throw()
    {
        return (m_setup_functor != NULL) && (m_teardown_functor != NULL);
    }

    bool IsSetupRun() const throw()
    {
        return m_setup_run;
    }

    TestString GetSetupClassName() const
    {
        TestString class_name(m_setup_functor != NULL ?
                              m_setup_functor->GetClassName() : "");
        return class_name;
    }

    TestString GetTeardownClassName() const
    {
        TestString class_name(m_teardown_functor != NULL ?
                              m_teardown_functor->GetClassName() : "");
        return class_name;
    }

    template<bool const is_setup>
    char const* GetClassName() const throw()
    {
        return is_setup ? GetSetupClassName() : GetTeardownClassName();
    }

    TestSetupTeardownFunctor* GetSetupFunctor()
    {
        return m_setup_functor;
    }

    void SetSetupFunctor(TestSetupTeardownFunctor& setup_functor)
    {
        m_setup_functor = &setup_functor;
    }

    TestSetupTeardownFunctor* GetTeardownFunctor()
    {
        return m_teardown_functor;
    }

    void SetTeardownFunctor(TestSetupTeardownFunctor& teardown_functor)
    {
        m_teardown_functor = &teardown_functor;
    }

    template<bool const is_setup>
    TestSetupTeardownFunctor* GetSetupTeardownFunctor()
    {
        return is_setup ? GetSetupFunctor() : GetTeardownFunctor();
    }

    template<bool const is_setup>
    TestSetupTeardownFunctor*& SetupOrTeardownFunctor()
    {
        return is_setup ? m_setup_functor : m_teardown_functor;
    }

    template<bool const is_setup>
    void Execute()
    {
        TestSetupTeardownFunctor* functor = SetupOrTeardownFunctor<is_setup>();
        if (functor != NULL)
        {
            m_setup_run = is_setup;
            functor->Execute();
        }
    }

    void Clear()
    {
        m_setup_functor = NULL;
        m_teardown_functor = NULL;
    }

private:
    TestSetupTeardownFunctor* m_setup_functor;
    TestSetupTeardownFunctor* m_teardown_functor;
    bool m_setup_run;
};

}

#endif // OCL_GUARD_TEST_TESTSETUPTEARDOWN_HPP
