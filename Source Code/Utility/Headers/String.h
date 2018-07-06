/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef STRING_H_
#define STRING_H_

#include "config.h"

/// Although the string implementation replaces most of the string instances in the code,
/// some are left as std::string for compatibility reasons (mainly with algorithms and external libraries) -Ionut

#if defined(STRING_IMP) && STRING_IMP == 0
    #include <EASTL/string.h>
    namespace stringAlg = eastl;

    typedef eastl::string stringImpl;

namespace eastl {
    typedef eastl_size_t stringSize;
    inline const char* toBase(const std::string& input) {
        return input.c_str();
    }

    inline const char* fromBase(const eastl::string& input) {
        return input.c_str();
    }

};
    
#else //defined(STRING_IMP) && STRING_IMP == 1
    #include <string>
    namespace stringAlg = std;

    typedef std::string stringImpl;

namespace std {
    typedef size_t stringSize;
    inline const std::string& toBase(const std::string& input) {
        return input;
    }
   
    inline const char* fromBase(const std::string& input) {
        return input.c_str();
    }
};
#endif //defined(VECTOR_IMP)

#endif