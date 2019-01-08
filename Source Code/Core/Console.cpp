#include "stdafx.h"

#include "Headers/Console.h"

#include "Core/Time/Headers/ApplicationTimer.h"
#include <iomanip>

#ifndef USE_BLOCKING_QUEUE
#define USE_BLOCKING_QUEUE
#endif //USE_BLOCKING_QUEUE

namespace Divide {

bool Console::_timestamps = false;
bool Console::_threadID = false;
bool Console::_enabled = true;
bool Console::_errorStreamEnabled = true;

std::thread Console::_printThread;
std::atomic_bool Console::_running = false;
vector<Console::ConsolePrintCallback> Console::_guiConsoleCallbacks;

//Use moodycamel's implementation of a concurent queue due to its "Knock-your-socks-off blazing fast performance."
//https://github.com/cameron314/concurrentqueue
namespace {
    constexpr bool g_useImmediatePrint = false;

    thread_local char textBuffer[CONSOLE_OUTPUT_BUFFER_SIZE + 1];

#if defined(USE_BLOCKING_QUEUE)
    moodycamel::BlockingConcurrentQueue<Console::OutputEntry>& outBuffer() {
        static moodycamel::BlockingConcurrentQueue<Console::OutputEntry> buf;
        return buf;
    }
#else
    moodycamel::ConcurrentQueue<Console::OutputEntry>& outBuffer() {
        static moodycamel::ConcurrentQueue<Console::OutputEntry> buf;
        return buf;
    }

    std::mutex& condMutex() noexcept {
        static std::mutex condMutex;
        return condMutex;
    }

    std::condition_variable& entryEnqueCV() {
        static std::condition_variable entryEnqueCV;
        return entryEnqueCV;
    }

    std::atomic_bool& entryAdded() noexcept {
        static std::atomic_bool entryAdded;
        return entryAdded;
    }

#endif
};

//! Do not remove the following license without express permission granted by DIVIDE-Studio
void Console::printCopyrightNotice() {
    std::cout << "------------------------------------------------------------------------------\n"
              << "Copyright (c) 2018 DIVIDE-Studio\n"
              << "Copyright (c) 2009 Ionut Cava\n\n"
              << "This file is part of DIVIDE Framework.\n\n"
              << "Permission is hereby granted, free of charge, to any person obtaining a copy of this software\n"
              << "and associated documentation files (the 'Software'), to deal in the Software without restriction,\n"
              << "including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,\n"
              << "and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,\n"
              << "subject to the following conditions:\n\n"
              << "The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.\n\n"
              << "THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,\n"
              << "INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.\n"
              << "IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,\n"
              << "WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE\n"
              << "OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\n"
              << "For any problems or licensing issues I may have overlooked, please contact: \n"
              << "E-mail: ionut.cava@divide-studio.com | Website: \n http://wwww.divide-studio.com\n"
              << "-------------------------------------------------------------------------------\n\n";
}

const char* Console::formatText(const char* format, ...) noexcept {
    va_list args;
    va_start(args, format);
    assert(_vscprintf(format, args) + 1 < CONSOLE_OUTPUT_BUFFER_SIZE);
    vsprintf(textBuffer, format, args);
    va_end(args);
    return textBuffer;
}

void Console::decorate(std::ostream& outStream, const char* text, const bool newline, const EntryType type) {
    if (_timestamps) {
        outStream << "[ " << std::internal
                          << std::setw(9)
                          << std::setprecision(3)
                          << std::setfill('0')
                          << std::fixed
                          << Time::ElapsedSeconds(true)
                  << " ] ";
    }
    if (_threadID) {
        outStream << "[ " << std::this_thread::get_id() << " ] ";
    }

    outStream << (type == EntryType::Error ? " Error: " : type == EntryType::Warning ? " Warning: " : "");
    outStream << text;
    outStream << (newline ? "\n" : "");
}

void Console::output(std::ostream& outStream, const char* text, const bool newline, const EntryType type) {
    if (_enabled) {
        decorate(outStream, text, newline, type);
    }
}

void Console::output(const char* text, const bool newline, const EntryType type) {
    if (_enabled) {
        stringstreamImplBest outStream;
        decorate(outStream, text, newline, type);

        OutputEntry entry;
        entry._text = outStream.str();
        entry._type = type;
        if (g_useImmediatePrint) {
            printToFile(entry);
        } else {
            //moodycamel::ProducerToken ptok(outBuffer());
            if (!outBuffer().enqueue(/*ptok, */entry)) {
                // ToDo: Something happened. Handle it, maybe? -Ionut
            }

#if !defined(USE_BLOCKING_QUEUE)
            Lock lk(condMutex());
            entryAdded() = true;
            entryEnqueCV().notify_one();
#endif
        }
    }
}

void Console::printToFile(const OutputEntry& entry) {
    ((entry._type == EntryType::Error && _errorStreamEnabled) ? std::cerr : std::cout) << entry._text.c_str();

    for (const Console::ConsolePrintCallback& cbk : _guiConsoleCallbacks) {
        if (!_running) {
            break;
        }
        cbk(entry);
    }
}

void Console::outThread() {

    constexpr U32 appUpdateRate = Config::TARGET_FRAME_RATE / Config::TICK_DIVISOR;
    constexpr U32 appUpdateRateInMS = 1000 / appUpdateRate;

    //moodycamel::ConsumerToken ctok(outBuffer());
    OutputEntry entry;
    while (_running) {
#if defined(USE_BLOCKING_QUEUE)

        if (outBuffer().wait_dequeue_timed(/*ctok, */entry, Time::Milliseconds(appUpdateRateInMS))) {
#else
        Lock lk(condMutex());
        entryEnqueCV().wait(lk, []() -> bool { return entryAdded(); });

        while (outBuffer().try_dequeue(/*ctok, */entry)) {
#endif
            printToFile(entry);
            if (!_running) {
                break;
            }
        } 
        //else {
            //std::this_thread::yield();
        //}
    }
    std::cout << "------------------------------------------" << std::endl
              << std::endl << std::endl << std::endl;

    std::cerr << std::flush;
    std::cout << std::flush;
}

void Console::start() {
    if (!_running) {
        _running = true;
        _printThread = std::thread(&Console::outThread);
        setThreadName(&_printThread, "CONSOLE_OUT_THREAD");
    }
}

void Console::stop() {
    if (_running) {
        _enabled = false;
        _running = false;
#if !defined(USE_BLOCKING_QUEUE)
        {
            Lock lk(condMutex());
            entryAdded() = true;
            entryEnqueCV().notify_one();
        }
#endif
        _printThread.join();
    }
}
};
