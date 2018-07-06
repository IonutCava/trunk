/*
   Copyright (c) 2017 DIVIDE-Studio
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

namespace Divide {

constexpr int CONSOLE_OUTPUT_BUFFER_SIZE = 4096 * 16;
constexpr int MAX_CONSOLE_ENTRIES = 128;

class Console : private NonCopyable {
    typedef std::function<void(const char*, bool)> ConsolePrintCallback;

   public:
    struct OutputEntry {
        OutputEntry() : _error(false)
        {
        }

        OutputEntry(const stringImplBest& text, bool error)
            : _text(text),
             _error(error)
        {
        }

        stringImplBest _text;
        bool _error;
    };

   public:
    static void start();
    static void stop();

    static void printCopyrightNotice();

    template <typename... T>
    inline static void printfn(const char* format, T&&... args);
    template <typename... T>
    inline static void printf(const char* format, T&&... args);
    template <typename... T>
    inline static void errorfn(const char* format, T&&... args);
    template <typename... T>
    inline static void errorf(const char* format, T&&... args);

    template <typename... T>
    inline static void d_printfn(const char* format, T&&... args);
    template <typename... T>
    inline static void d_printf(const char* format, T&&... args);
    template <typename... T>
    inline static void d_errorfn(const char* format, T&&... args);
    template <typename... T>
    inline static void d_errorf(const char* format, T&&... args);
    
    template <typename... T>
    inline static void printfn(std::ofstream& outStream, const char* format, T&&... args);
    template <typename... T>
    inline static void printf(std::ofstream& outStream, const char* format, T&&... args);
    template <typename... T>
    inline static void errorfn(std::ofstream& outStream, const char* format, T&&... args);
    template <typename... T>
    inline static void errorf(std::ofstream& outStream, const char* format, T&&... args);
    template <typename... T>
    inline static void d_printfn(std::ofstream& outStream, const char* format, T&&... args);
    template <typename... T>
    inline static void d_printf(std::ofstream& outStream, const char* format, T&&... args);
    template <typename... T>
    inline static void d_errorfn(std::ofstream& outStream, const char* format, T&&... args);
    template <typename... T>
    inline static void d_errorf(std::ofstream& outStream, const char* format, T&&... args);

    static bool timeStampsEnabled() { return _timestamps; }
    static void toggleTimeStamps(const bool state) { _timestamps = state; }

    static bool threadIDEnabled() { return _threadID; }
    static void togglethreadID(const bool state) { _threadID = state; }

    static bool enabled() { return _enabled; }
    static void toggle(const bool state) { _enabled = state; }

    static bool errorStreamEnabled() { return _errorStreamEnabled; }
    static void toggleErrorStream(const bool state) { _errorStreamEnabled = state; }
    static void bindConsoleOutput(const ConsolePrintCallback& guiConsoleCallback) {
        _guiConsoleCallback = guiConsoleCallback;
    }

   protected:
    static const char* formatText(const char* format, ...);
    static void output(const char* text, const bool newline, const bool error);
    static void output(std::ostream& outStream, const char* text, const bool newline, const bool error);
    static void decorate(std::ostream& outStream, const char* text, const bool newline, const bool error);
    static void outThread();

   private:
    static ConsolePrintCallback _guiConsoleCallback;
    static bool _timestamps;
    static bool _threadID;
    static bool _enabled;
    static bool _errorStreamEnabled;

    static std::atomic_bool _running;
    static std::thread _printThread;
};

};  // namespace Divide

#endif  //_CORE_CONSOLE_H_

#include "Console.inl"
