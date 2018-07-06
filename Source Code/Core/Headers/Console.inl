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
template <typename... T>
const char* Console::d_printfn(const char* format, T&&... args) {
#ifdef _DEBUG
    return printfn(format, std::forward<T>(args)...);
#else
    return "";
#endif
}

template <typename... T>
const char* Console::d_printf(const char* format, T&&... args) {
#ifdef _DEBUG
    return printf(format, std::forward<T>(args)...);
#else
    return "";
#endif
}

template <typename... T>
const char* Console::d_errorfn(const char* format, T&&... args) {
#ifdef _DEBUG
    return errorfn(format, std::forward<T>(args)...);
#else
    return "";
#endif
}

template <typename... T>
const char* Console::d_errorf(const char* format, T&&... args) {
#ifdef _DEBUG
    return errorf(format, std::forward<T>(args)...);
#else
    return "";
#endif
}

template <typename... T>
const char* Console::printfn(const char* format, T&&... args) {
    return output(formatText(format, std::forward<T>(args)...), true, false);
}

template <typename... T>
const char* Console::printf(const char* format, T&&... args) {
    return output(formatText(format, std::forward<T>(args)...), false, false);
}

template <typename... T>
const char* Console::errorfn(const char* format, T&&... args) {
    return output(formatText(format, std::forward<T>(args)...), true, true);
}

template <typename... T>
const char* Console::errorf(const char* format, T&&... args) {
    return output(formatText(format, std::forward<T>(args)...), false, true);
}

template <typename... T>
const char* Console::printfn(std::ofstream& outStream, const char* format,
                             T&&... args) {
    return output(outStream, formatText(format, std::forward<T>(args)...), true, false);
}

template <typename... T>
const char* Console::printf(std::ofstream& outStream, const char* format,
                            T&&... args) {
    return output(outStream, formatText(format, std::forward<T>(args)...),
                  false, false);
}

template <typename... T>
const char* Console::errorfn(std::ofstream& outStream, const char* format,
                             T&&... args) {
    return output(formatText(format, std::forward<T>(args)...), true, true);
}

template <typename... T>
const char* Console::errorf(std::ofstream& outStream, const char* format,
                            T&&... args) {
    return output(outStream, formatText(format, std::forward<T>(args)...),
                  false, true);
}

template <typename... T>
const char* Console::d_printfn(std::ofstream& outStream, const char* format,
                               T&&... args) {
#ifdef _DEBUG
    return printfn(outStream, format, std::forward<T>(args)...);
#else
    return "";
#endif
}

template <typename... T>
const char* Console::d_printf(std::ofstream& outStream, const char* format,
                              T&&... args) {
#ifdef _DEBUG
    return printf(outStream, format, std::forward<T>(args)...);
#else
    return "";
#endif
}

template <typename... T>
const char* Console::d_errorfn(std::ofstream& outStream, const char* format,
                               T&&... args) {
#ifdef _DEBUG
    return errorfn(outStream, format, std::forward<T>(args)...);
#else
    return "";
#endif
}

template <typename... T>
const char* Console::d_errorf(std::ofstream& outStream, const char* format,
                              T&&... args) {
#ifdef _DEBUG
    return errorf(outStream, format, std::forward<T>(args)...);
#else
    return "";
#endif
}
};

#endif  //_CORE_CONSOLE_INL_