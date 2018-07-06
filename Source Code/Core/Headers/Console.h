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

#include <mutex>
#include <functional>
#include <atomic>
#include <fstream>

namespace Divide {

static const int CONSOLE_OUTPUT_BUFFER_SIZE = 4096 * 16;
static const int MAX_CONSOLE_ENTRIES = 5;

class Console : private NonCopyable {
    typedef std::function<void(const char*, bool)> consolePrintCallback;

   public:
    static void flush();
    static void printCopyrightNotice();

    template <typename... T>
    inline static const char* printfn(const char* format, T&&... args);
    template <typename... T>
    inline static const char* printf(const char* format, T&&... args);
    template <typename... T>
    inline static const char* errorfn(const char* format, T&&... args);
    template <typename... T>
    inline static const char* errorf(const char* format, T&&... args);

    template <typename... T>
    inline static const char* d_printfn(const char* format, T&&... args);
    template <typename... T>
    inline static const char* d_printf(const char* format, T&&... args);
    template <typename... T>
    inline static const char* d_errorfn(const char* format, T&&... args);
    template <typename... T>
    inline static const char* d_errorf(const char* format, T&&... args);
    template <typename... T>
    inline static const char* printfn(std::ofstream& outStream,
                                      const char* format, T&&... args);
    template <typename... T>
    inline static const char* printf(std::ofstream& outStream,
                                     const char* format, T&&... args);
    template <typename... T>
    inline static const char* errorfn(std::ofstream& outStream,
                                      const char* format, T&&... args);
    template <typename... T>
    inline static const char* errorf(std::ofstream& outStream,
                                     const char* format, T&&... args);
    template <typename... T>
    inline static const char* d_printfn(std::ofstream& outStream,
                                        const char* format, T&&... args);
    template <typename... T>
    inline static const char* d_printf(std::ofstream& outStream,
                                       const char* format, T&&... args);
    template <typename... T>
    inline static const char* d_errorfn(std::ofstream& outStream,
                                        const char* format, T&&... args);
    template <typename... T>
    inline static const char* d_errorf(std::ofstream& outStream,
                                       const char* format, T&&... args);

    static void toggleTimeStamps(const bool state) { _timestamps = state; }
    static void bindConsoleOutput(
        const consolePrintCallback& guiConsoleCallback) {
        _guiConsoleCallback = guiConsoleCallback;
    }

   protected:
    static const char* formatText(const char* format, ...);
    static const char* output(const char* text, const bool newline,
                              const bool error);
    static const char* output(std::ostream& outStream, const char* text,
                              const bool newline, const bool error);

   private:
    static std::atomic<int> _bufferEntryCount;
    static std::mutex io_mutex;
    static consolePrintCallback _guiConsoleCallback;
    static bool _timestamps;
};

};  // namespace Divide

#endif  //_CORE_CONSOLE_H_

#include "Console.inl"
