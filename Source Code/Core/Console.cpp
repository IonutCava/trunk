#include "Headers/Console.h"

#include "Core/Headers/ApplicationTimer.h"
#include "config.h"
#include <iomanip>
#include <stdarg.h>
#include <thread>
#include <iostream>

namespace Divide {

bool Console::_timestamps = false;
bool Console::_threadID = false;
std::thread Console::_printThread;

std::atomic_bool Console::_running;
Console::consolePrintCallback Console::_guiConsoleCallback;

boost::lockfree::stack<Console::OutputEntry> Console::_outputBuffer(MAX_CONSOLE_ENTRIES);

//! Do not remove the following license without express permission granted by DIVIDE-Studio
void Console::printCopyrightNotice() {
    std::cout << "-------------------------------------------------------------"
                 "-----------------\n";
    std::cout << "Copyright (c) 2016 DIVIDE-Studio\n";
    std::cout << "Copyright (c) 2009 Ionut Cava\n\n";
    std::cout << "This file is part of DIVIDE Framework.\n\n";
    std::cout << "Permission is hereby granted, free of charge, to any person "
                 "obtaining a copy of this software\n";
    std::cout << "and associated documentation files (the 'Software'), to deal "
                 "in the Software without restriction,\n";
    std::cout << "including without limitation the rights to use, copy, "
                 "modify, merge, publish, distribute, sublicense,\n";
    std::cout << "and/or sell copies of the Software, and to permit persons to "
                 "whom the Software is furnished to do so,\n";
    std::cout << "subject to the following conditions:\n\n";
    std::cout << "The above copyright notice and this permission notice shall "
                 "be included in all copies or substantial portions of the "
                 "Software.\n\n";
    std::cout << "THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY "
                 "KIND, EXPRESS OR IMPLIED,\n";
    std::cout << "INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF "
                 "MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND "
                 "NONINFRINGEMENT.\n";
    std::cout << "IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE "
                 "FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,\n";
    std::cout << "WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING "
                 "FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE\n";
    std::cout << "OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\n";
    std::cout << "For any problems or licensing issues I may have overlooked, "
                 "please contact: \n";
    std::cout << "E-mail: ionut.cava@divide-studio.com | Website: \n"
                 "http://wwww.divide-studio.com\n";
    std::cout << "-------------------------------------------------------------"
                 "------------------\n\n";
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

const char* Console::output(const char* text, const bool newline, const bool error) {
    if (_guiConsoleCallback) {
        _guiConsoleCallback(text, error);
    }

    OutputEntry entry;
    entry._error = error;

    std::stringstream outStream;
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

    entry._text.append(outStream.str());
    entry._text = (error ? " Error: " : "");
    entry._text.append(text);
    entry._text.append(newline ? "\n" : "");

    WAIT_FOR_CONDITION(_outputBuffer.push(entry));

    return entry._text.c_str();
}

void Console::outThread() {
    while (_running) {
        OutputEntry entry;
        while (_outputBuffer.pop(entry)) {
            std::ostream& outStream = entry._error ? std::cerr : std::cout;
            outStream << entry._text.c_str();
        }
    }
}

void Console::start() {
    _running = true;
    _printThread = std::thread(&Console::outThread);
}

void Console::stop() {
    _running = false;
    _printThread.join();

    std::cerr << std::flush;
    std::cout << std::flush;
}
};
