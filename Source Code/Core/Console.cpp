#include "Headers/Console.h"
#include "Rendering/Headers/Framerate.h"
#include "config.h"
#include <iomanip>
#include <stdarg.h>

//! Do not remove the following license without express permission granted bu DIVIDE-Studio
void Console::printCopyrightNotice(){
	std::cout << "------------------------------------------------------------------------------" << std::endl;
	std::cout << "“Copyright 2009-2013 DIVIDE-Studio”" << std::endl << std::endl;
	std::cout << "This file is part of DIVIDE Framework." << std::endl;
	std::cout << "DIVIDE Framework is free software: you can redistribute it and/or modify" << std::endl;
	std::cout << "it under the terms of the GNU Lesser General Public License as published by" << std::endl;
	std::cout << "the Free Software Foundation, either version 3 of the License, or" << std::endl;
	std::cout << "(at your option) any later version." << std::endl << std::endl;
	std::cout << "   DIVIDE Framework is distributed in the hope that it will be useful," << std::endl;
	std::cout << "but WITHOUT ANY WARRANTY; without even the implied warranty of" << std::endl;
	std::cout << "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the" << std::endl;
	std::cout << "GNU Lesser General Public License for more details." << std::endl;
	std::cout << "You should have received a copy of the GNU Lesser General Public License" << std::endl;
	std::cout << "along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>." << std::endl << std::endl;
	std::cout << "For any problems or licensing issues I may have overlooked, please contact: " << std::endl;
	std::cout << "E-mail: ionut.cava@divide-studio.com | Website: http://wwww.divide-studio.com" << std::endl;
	std::cout << "-------------------------------------------------------------------------------" << std::endl;
	std::cout << std::endl;
	_timestamps = true;
}
void Console::d_printfn(char* format, ...){
#ifdef _DEBUG
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
#endif
}

void Console::d_printf(char* format, ...){
#ifdef _DEBUG
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
#endif
}

void Console::printfn(char* format, ...){
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

void Console::printf(char* format, ...){
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
void Console::d_errorfn(char* format, ...){
#ifdef _DEBUG
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
#endif
}

void Console::d_errorf(char* format, ...){
#ifdef _DEBUG
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
#endif
}

void Console::errorfn(char* format, ...){
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

void Console::errorf(char* format, ...){
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

void Console::output(const std::string& output,bool error){
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
	//GUI::getInstance().printConsole(output);
}