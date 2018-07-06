#include "stdafx.h"

#include "Headers/Console.h"

#include "Core/Time/Headers/ApplicationTimer.h"
#include <iomanip>
#include <stdarg.h>
#include <thread>
#include <iostream>

namespace Divide {

bool Console::_timestamps = false;
bool Console::_threadID = false;
bool Console::_enabled = true;

std::thread Console::_printThread;
//std::condition_variable Console::_entryEnqueCV;
//std::mutex Console::_entryEnqueMutex;
//std::atomic_bool Console::_entryAdded;
std::atomic_bool Console::_running = false;
Console::ConsolePrintCallback Console::_guiConsoleCallback;

//Use moodycamel's implementation of a concurent queue due to its "Knock-your-socks-off blazing fast performance."
//https://github.com/cameron314/concurrentqueue
moodycamel::ConcurrentQueue<Console::OutputEntry> Console::_outputBuffer(MAX_CONSOLE_ENTRIES);

//! Do not remove the following license without express permission granted by DIVIDE-Studio
void Console::printCopyrightNotice() {
    std::cout << "------------------------------------------------------------------------------\n"
              << "Copyright (c) 2017 DIVIDE-Studio\n"
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

const char* Console::formatText(const char* format, ...) {
    static thread_local char textBuffer[CONSOLE_OUTPUT_BUFFER_SIZE + 1];
    va_list args;
    va_start(args, format);
    assert(_vscprintf(format, args) + 1 < CONSOLE_OUTPUT_BUFFER_SIZE);
    vsprintf(textBuffer, format, args);
    va_end(args);
    return textBuffer;
}

void Console::decorate(std::ostream& outStream, const char* text, const bool newline, const bool error) {
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

    outStream << (error ? " Error: " : "");
    outStream << text;
    outStream << (newline ? "\n" : "");
}

void Console::output(std::ostream& outStream, const char* text, const bool newline, const bool error) {
    if (_enabled) {
        decorate(outStream, text, newline, error);
    }
}

void Console::output(const char* text, const bool newline, const bool error) {
    if (_enabled) {
        stringstreamImplBest outStream;
        decorate(outStream, text, newline, error);

        OutputEntry entry;
        entry._error = error;
        entry._text = outStream.str();

        //std::unique_lock<std::mutex> lk(_entryEnqueMutex);
        //moodycamel::ProducerToken ptok(_outputBuffer);
        WAIT_FOR_CONDITION_TIMEOUT(_outputBuffer.enqueue(/*ptok, */entry),
                                   Time::SecondsToMilliseconds(1.0));
        //_entryAdded = true;
        //_entryEnqueCV.notify_one();
    }
}

void Console::outThread() {
    //moodycamel::ConsumerToken ctok(_outputBuffer);
    while (_running) {
        {
            //std::unique_lock<std::mutex> lk(_entryEnqueMutex);
            //_entryEnqueCV.wait(lk, [] {return _entryAdded == true; });
        }

        OutputEntry entry;
        if (_outputBuffer.try_dequeue(/*ctok, */entry)) {
            (entry._error ? std::cerr : std::cout) << entry._text.c_str();

            if (_guiConsoleCallback) {
                _guiConsoleCallback(entry._text.c_str(), entry._error);
            }
        } else {
            std::this_thread::yield();
        }
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
        _printThread.join();
    }
}
};
