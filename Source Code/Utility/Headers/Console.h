#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "resource.h"
#include "Utility/Headers/Singleton.h" 
#include <stdarg.h>

SINGLETON_BEGIN(Console)

public:
	void printCopyrightNotice();
	void printfn(char* format, ...);
	void printf(char* format, ...);
	void errorfn(char* format, ...);
	void errorf(char* format, ...);

private:
	void output(const std::string& text);

SINGLETON_END()

#endif