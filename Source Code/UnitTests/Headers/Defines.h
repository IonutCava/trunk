/*
Copyright (c) 2016 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef _DEFINES_H_
#define _DEFINES_H_

//Using: https://github.com/cppocl/cppocl
#include "Platform/Headers/PlatformDefines.h"

#ifdef NO_ERROR
#undef NO_ERROR
#endif

namespace Divide {
    const U32 NO_ERROR = 0;
    const U32 GENERAL_ERROR = 1;
    const U32 EXCEPTION_ERROR = 2;

    I32 Error();

    // Test a function that can throw an exception and see if we were expecting it.
    #define EXCEPTION_CHECK(func, exception_type, expect_exception) \
    { \
        bool found_exception = false; \
        try \
        { \
            func; \
        } \
        catch (exception_type&) \
        { \
            found_exception = true; \
        } \
        if ((expect_exception && !found_exception) || (!expect_exception && found_exception)) \
            return Error(); \
    }

    #define EXCEPTION_RANGE_CHECK(func, want_exception) \
        EXCEPTION_CHECK(func, std::out_of_range, want_exception)

    #define CHECK_TRUE(exp) if (!(exp)) return Error()
    #define CHECK_FALSE(exp) if (exp) return Error()
    #define CHECK_EQUAL(exp, value) if (exp != value) return Error()
    #define CHECK_NOT_EQUAL(exp, value) if (exp == value) return Error()

}; //namespace Divide
#endif //_DEFINES_H_
