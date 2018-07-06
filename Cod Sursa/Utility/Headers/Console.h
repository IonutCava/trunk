#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "resource.h"
#include "Utility/Headers/Singleton.h" 
#include <stdarg.h>

SINGLETON_BEGIN(Con)

public:
	void printfn(char* format, ...)
	{
		va_list args;
		string fmt_text;
		va_start(args, format);
		int len = _vscprintf(format, args) + 1;
	    char *text = new char[len];
		vsprintf_s(text, len, format, args);
		fmt_text.append(text);
		delete[] text;
		text = NULL;
		va_end(args);

		cout << fmt_text << endl;
		//GUI::getInstance().printConsole();
		fmt_text.empty();
	}

	void printf(char* format, ...)
	{
		va_list args;
		string fmt_text;
		va_start(args, format);
		int len = _vscprintf(format, args) + 1;
	    char *text = new char[len];
		vsprintf_s(text, len, format, args);
		fmt_text.append(text);
		delete[] text;
		text = NULL;
		va_end(args);

		cout << fmt_text;
		//GUI::getInstance().printConsole();
		fmt_text.empty();
	}

	void errorfn(char* format, ...)
	{
		va_list args;
		string fmt_text;
		va_start(args, format);
		int len = _vscprintf(format, args) + 1;
	    char *text = new char[len];
		vsprintf_s(text, len, format, args);
		fmt_text.append(text);
		delete[] text;
		text = NULL;
		va_end(args);

		cout << "Error: " << fmt_text << endl;
		//GUI::getInstance().printErrorConsole(fmt_text);
		fmt_text.empty();
	}

	void errorf(char* format, ...)
	{
		va_list args;
		string fmt_text;
		va_start(args, format);
		int len = _vscprintf(format, args) + 1;
	    char *text = new char[len];
		vsprintf_s(text, len, format, args);
		fmt_text.append(text);
		delete[] text;
		text = NULL;
		va_end(args);

		cout << "Error: " << fmt_text;
		//GUI::getInstance().printErrorConsole(fmt_text);
		fmt_text.empty();
	}

SINGLETON_END()

#endif