/* Copyright (c) 2018 DIVIDE-Studio
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

#ifndef _PLATFORM_VIDEO_OPENGLS_PROGRAM_INL_
#define _PLATFORM_VIDEO_OPENGLS_PROGRAM_INL_

#include "Core/Headers/Console.h"

namespace Divide {
    template<typename T_out, size_t T_out_count, typename T_in>
    const T_out* glShaderProgram::castData(const vectorEASTL<char>& values) const {
        static_assert(sizeof(T_out) * T_out_count == sizeof(T_in), "Invalid cast data");

        return reinterpret_cast<const T_out*>(values.data());
    }
}; //namespace Divide

#endif //_PLATFORM_VIDEO_OPENGLS_PROGRAM_INL_
