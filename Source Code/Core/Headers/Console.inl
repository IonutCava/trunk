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

#ifndef _CORE_CONSOLE_INL_
#define _CORE_CONSOLE_INL_

namespace Divide {
template <typename... Args>
void Console::d_printfn(const char* format, Args&&... args) {
    if (Config::Build::IS_DEBUG_BUILD) {
        printfn(format, std::forward<Args>(args)...);
    }
}

template <typename... Args>
void Console::d_printf(const char* format, Args&&... args) {
    if (Config::Build::IS_DEBUG_BUILD) {
        printf(format, std::forward<Args>(args)...);
    }
}

template <typename... Args>
void Console::d_errorfn(const char* format, Args&&... args) {
    if (Config::Build::IS_DEBUG_BUILD) {
        errorfn(format, std::forward<Args>(args)...);
    }
}

template <typename... Args>
void Console::d_errorf(const char* format, Args&&... args) {
    if (Config::Build::IS_DEBUG_BUILD) {
        errorf(format, std::forward<Args>(args)...);
    }
}

template <typename... Args>
void Console::printfn(const char* format, Args&&... args) {
    output(formatText(format, std::forward<Args>(args)...), true, false);
}

template <typename... Args>
void Console::printf(const char* format, Args&&... args) {
    output(formatText(format, std::forward<Args>(args)...), false, false);
}

template <typename... Args>
void Console::errorfn(const char* format, Args&&... args) {
    output(formatText(format, std::forward<Args>(args)...), true, true);
}

template <typename... Args>
void Console::errorf(const char* format, Args&&... args) {
    return output(formatText(format, std::forward<Args>(args)...), false, true);
}
template <typename... Args>
void Console::printfn(std::ofstream& outStream, const char* format, Args&&... args) {
    output(outStream, formatText(format, std::forward<Args>(args)...), true, false);
}

template <typename... Args>
void Console::printf(std::ofstream& outStream, const char* format, Args&&... args) {
    output(outStream, formatText(format, std::forward<Args>(args)...), false, false);
}

template <typename... Args>
void Console::errorfn(std::ofstream& outStream, const char* format, Args&&... args) {
    output(formatText(format, std::forward<Args>(args)...), true, true);
}

template <typename... Args>
void Console::errorf(std::ofstream& outStream, const char* format, Args&&... args) {
    output(outStream, formatText(format, std::forward<Args>(args)...), false, true);
}

template <typename... Args>
void Console::d_printfn(std::ofstream& outStream, const char* format, Args&&... args) {
    if (Config::Build::IS_DEBUG_BUILD) {
        printfn(outStream, format, std::forward<Args>(args)...);
    }
}

template <typename... Args>
void Console::d_printf(std::ofstream& outStream, const char* format, Args&&... args) {
    if (Config::Build::IS_DEBUG_BUILD) {
        printf(outStream, format, std::forward<Args>(args)...);
    }
}

template <typename... Args>
void Console::d_errorfn(std::ofstream& outStream, const char* format, Args&&... args) {
    if (Config::Build::IS_DEBUG_BUILD) {
        errorfn(outStream, format, std::forward<Args>(args)...);
    }
}

template <typename... Args>
void Console::d_errorf(std::ofstream& outStream, const char* format, Args&&... args) {
    if (Config::Build::IS_DEBUG_BUILD) {
        errorf(outStream, format, std::forward<Args>(args)...);
    }
}

};

#endif  //_CORE_CONSOLE_INL_