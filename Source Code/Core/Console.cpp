#include "core.h"

#include "Headers/Console.h"
#include "Core/Headers/ApplicationTimer.h"
#include "config.h"
#include <iomanip>
#include <stdarg.h>

//! Do not remove the following license without express permission granted bu DIVIDE-Studio
void Console::printCopyrightNotice() const {
    std::cout << "------------------------------------------------------------------------------" << std::endl;
    std::cout << "Copyright (c) 2014 DIVIDE-Studio" << std::endl;
    std::cout << "Copyright (c) 2009 Ionut Cava" << std::endl;
    std::cout << std::endl;
    std::cout << "This file is part of DIVIDE Framework." << std::endl;
    std::cout << std::endl;
    std::cout << "Permission is hereby granted, free of charge, to any person obtaining a copy of this software" << std::endl;
    std::cout << "and associated documentation files (the 'Software'), to deal in the Software without restriction," << std::endl;
    std::cout << "including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense," << std::endl;
    std::cout << "and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so," << std::endl;
    std::cout << "subject to the following conditions:" << std::endl;
    std::cout << std::endl;
    std::cout << "The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software." << std::endl;
    std::cout << std::endl;
    std::cout << "THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED," << std::endl;
    std::cout << "INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT." << std::endl;
    std::cout << "IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY," << std::endl;
    std::cout << "WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE" << std::endl;
    std::cout << "OR THE USE OR OTHER DEALINGS IN THE SOFTWARE." << std::endl;
    std::cout << std::endl;
    std::cout << "For any problems or licensing issues I may have overlooked, please contact: " << std::endl;
    std::cout << "E-mail: ionut.cava@divide-studio.com | Website: http://wwww.divide-studio.com" << std::endl;
    std::cout << "-------------------------------------------------------------------------------" << std::endl;
    std::cout << std::endl;
}

#ifdef _DEBUG
void Console::d_printfn(const char* format, ...) const {
    va_list args;
    char text[CONSOLE_OUTPUT_BUFFER_SIZE] = {};
    va_start(args, format);
    assert(_vscprintf(format, args) - 1 < CONSOLE_OUTPUT_BUFFER_SIZE);
    vsprintf_s(text, sizeof(text), format, args);
    strcat(text, "\n");
    va_end(args);
    output(text);
}

void Console::d_printf(const char* format, ...) const {
    va_list args;
    char text[CONSOLE_OUTPUT_BUFFER_SIZE] = {};
    va_start(args, format);
    assert(_vscprintf(format, args) + 1 < CONSOLE_OUTPUT_BUFFER_SIZE);
    vsprintf_s(text, sizeof(text), format, args);
    va_end(args);
    output(text);
}

void Console::d_errorfn(const char* format, ...) const {
    va_list args;
    char text[CONSOLE_OUTPUT_BUFFER_SIZE] = {};
    va_start(args, format);
    assert(_vscprintf(format, args) + 3 < CONSOLE_OUTPUT_BUFFER_SIZE);
    vsprintf_s(text, sizeof(text), format, args);
    strcat(text, "\n");
    va_end(args);
    output(text,true);
}

void Console::d_errorf(const char* format, ...) const {
    va_list args;
    char text[CONSOLE_OUTPUT_BUFFER_SIZE] = {};
    va_start(args, format);
    assert(_vscprintf(format, args) + 1 < CONSOLE_OUTPUT_BUFFER_SIZE);
    vsprintf_s(text, sizeof(text), format, args);
    va_end(args);
    output(text,true);
}

#endif
void Console::printfn(const char* format, ...) const {
    va_list args;
    char text[CONSOLE_OUTPUT_BUFFER_SIZE] = {};
    va_start(args, format);
    assert(_vscprintf(format, args) + 3 < CONSOLE_OUTPUT_BUFFER_SIZE);
    vsprintf_s(text, sizeof(text), format, args);
    strcat(text, "\n");
    va_end(args);
    output(text);
}

void Console::printf(const char* format, ...) const {
    va_list args;
    char text[CONSOLE_OUTPUT_BUFFER_SIZE] = {};
    va_start(args, format);
    assert(_vscprintf(format, args) + 1 < CONSOLE_OUTPUT_BUFFER_SIZE);
    vsprintf_s(text, sizeof(text), format, args);
    va_end(args);
    output(text);
}

void Console::errorfn(const char* format, ...) const {
    va_list args;
    char text[CONSOLE_OUTPUT_BUFFER_SIZE] = {};
    va_start(args, format);
    assert(_vscprintf(format, args) + 3 < CONSOLE_OUTPUT_BUFFER_SIZE);
    vsprintf_s(text, sizeof(text), format, args);
    strcat(text, "\n");
    va_end(args);
    output(text,true);
}

void Console::errorf(const char* format, ...) const {
    va_list args;
    char text[CONSOLE_OUTPUT_BUFFER_SIZE] = {};
    va_start(args, format);
    assert(_vscprintf(format, args) + 1 < CONSOLE_OUTPUT_BUFFER_SIZE);
    vsprintf_s(text, sizeof(text), format, args);
    va_end(args);
    output(text,true);
}

void Console::output(const char* output, const bool error) const {
    if(!_guiConsoleCallback.empty()){
        if(error){
            std::string outputString("Error: ");
            outputString.append(output);
            _guiConsoleCallback(outputString.c_str(),error);
        }else{
            _guiConsoleCallback(output,error);
        }
    }

    boost::mutex::scoped_lock lock(io_mutex);

    std::ostream& outputStream = error ? std::cerr : std::cout;

    if(_timestamps)
        outputStream << "[ " << std::setprecision(2) << GETTIME(true) << " ] ";

    if(error)
        outputStream << " Error: ";

    outputStream << output << std::flush;
}