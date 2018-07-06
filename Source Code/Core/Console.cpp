#include "core.h"

#include "Headers/Console.h"
#include "Rendering/Headers/Framerate.h"
#include "config.h"
#include <iomanip>
#include <stdarg.h>

//! Do not remove the following license without express permission granted bu DIVIDE-Studio
void Console::printCopyrightNotice() const {
	std::cout << "------------------------------------------------------------------------------" << std::endl;
    std::cout << "Copyright (c) 2013 DIVIDE-Studio" << std::endl;
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
	std::string fmt_text;
	va_start(args, format);
	I32 len = _vscprintf(format, args) + 1;
	char *text = new char[len];
	vsprintf_s(text, len, format, args);
	fmt_text.append(text);
	fmt_text.append("\n");
	delete[] text;
	text = NULL;
	va_end(args);
	output(fmt_text);
	fmt_text.empty();
}

void Console::d_printf(const char* format, ...) const {
	va_list args;
	std::string fmt_text;
	va_start(args, format);
	I32 len = _vscprintf(format, args) + 1;
	char *text = new char[len];
	vsprintf_s(text, len, format, args);
	fmt_text.append(text);
	delete[] text;
	text = NULL;
	va_end(args);
	output(fmt_text);
	fmt_text.empty();
}
#endif

void Console::printfn(const char* format, ...) const {
	va_list args;
	std::string fmt_text;
	va_start(args, format);
	I32 len = _vscprintf(format, args) + 1;
	char *text = new char[len];
	vsprintf_s(text, len, format, args);
	fmt_text.append(text);
	fmt_text.append("\n");
	delete[] text;
	text = NULL;
	va_end(args);
	output(fmt_text);
	fmt_text.empty();
}

void Console::printf(const char* format, ...) const {
	va_list args;
	std::string fmt_text;
	va_start(args, format);
	I32 len = _vscprintf(format, args) + 1;
	char *text = new char[len];
	vsprintf_s(text, len, format, args);
	fmt_text.append(text);
	delete[] text;
	text = NULL;
	va_end(args);
	output(fmt_text);
	fmt_text.empty();
}

#ifdef _DEBUG

void Console::d_errorfn(const char* format, ...) const {
	va_list args;
	std::string fmt_text;
	va_start(args, format);
	I32 len = _vscprintf(format, args) + 1;
	char *text = new char[len];
	vsprintf_s(text, len, format, args);
	fmt_text.append("Error: ");
	fmt_text.append(text);
	fmt_text.append("\n");
	delete[] text;
	text = NULL;
	va_end(args);
	output(fmt_text,true);
	fmt_text.empty();
}

void Console::d_errorf(const char* format, ...) const {
	va_list args;
	std::string fmt_text;
	va_start(args, format);
	I32 len = _vscprintf(format, args) + 1;
	char *text = new char[len];
	vsprintf_s(text, len, format, args);
	fmt_text.append("Error: ");
	fmt_text.append(text);
	delete[] text;
	text = NULL;
	va_end(args);
	output(fmt_text,true);
	fmt_text.empty();
}
#endif

void Console::errorfn(const char* format, ...) const {
	va_list args;
	std::string fmt_text;
	va_start(args, format);
	I32 len = _vscprintf(format, args) + 1;
	char *text = new char[len];
	vsprintf_s(text, len, format, args);
	fmt_text.append("Error: ");
	fmt_text.append(text);
	fmt_text.append("\n");
	delete[] text;
	text = NULL;
	va_end(args);
	output(fmt_text,true);
	fmt_text.empty();
}

void Console::errorf(const char* format, ...) const {
	va_list args;
	std::string fmt_text;
	va_start(args, format);
	I32 len = _vscprintf(format, args) + 1;
	char *text = new char[len];
	vsprintf_s(text, len, format, args);
	fmt_text.append("Error: ");
	fmt_text.append(text);
	delete[] text;
	text = NULL;
	va_end(args);
	output(fmt_text,true);
	fmt_text.empty();
}

void Console::output(const std::string& output,const bool error) const {
	boost::mutex::scoped_lock  lock(io_mutex);
	if(_timestamps){
		if(error){
			std::cerr << "[ " << std::setprecision(4) << GETTIME() << " ] " << output << std::flush;
		}else{
			std::cout << "[ " << std::setprecision(4) << GETTIME() << " ] " << output << std::flush;
		}
	}else{
		if(error){
			std::cerr << output << std::flush;
		}else{
			std::cout << output << std::flush;
		}
	}
	if(!_guiConsoleCallback.empty()){
		_guiConsoleCallback(output,error);
	}
}