/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _CORE_CONSOLE_H_
#define _CORE_CONSOLE_H_

#include "Core/Headers/NonCopyable.h"

#include <boost/thread/mutex.hpp>
#include <functional>

namespace Divide {

static const int CONSOLE_OUTPUT_BUFFER_SIZE = 4096 * 16;

class Console : private NonCopyable {
    typedef std::function<void(const char*, bool)> consolePrintCallback;

   public:
    static void flush();
    static void printCopyrightNotice();
    static const char* printfn(const char* format, ...);
    static const char* printf(const char* format, ...);
    static const char* errorfn(const char* format, ...);
    static const char* errorf(const char* format, ...);

    static const char* d_printfn(const char* format, ...);
    static const char* d_printf(const char* format, ...);
    static const char* d_errorfn(const char* format, ...);
    static const char* d_errorf(const char* format, ...);

    static void toggleTimeStamps(const bool state) { _timestamps = state; }
    static void bindConsoleOutput(
        const consolePrintCallback& guiConsoleCallback) {
        _guiConsoleCallback = guiConsoleCallback;
    }

   protected:
    static const char* output(const char* text, const bool error = false);

   private:
    static boost::mutex io_mutex;
    static consolePrintCallback _guiConsoleCallback;
    static bool _timestamps;
    static char _textBuffer[CONSOLE_OUTPUT_BUFFER_SIZE];
};

};  // namespace Divide

#endif  //_CORE_CONSOLE_H_