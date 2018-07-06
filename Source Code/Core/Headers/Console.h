#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "resource.h"
#include <boost/thread.hpp>

DEFINE_SINGLETON(Console)

public:
	void printCopyrightNotice();
	void printfn(char* format, ...);
	void printf(char* format, ...);
	void errorfn(char* format, ...);
	void errorf(char* format, ...);
	void d_printfn(char* format, ...);
	void d_printf(char* format, ...);
	void d_errorfn(char* format, ...);
	void d_errorf(char* format, ...);
	void toggleTimeStamps(bool state){_timestamps = state;}
private:
	void output(const std::string& text,bool error = false);
	boost::mutex io_mutex;
	bool _timestamps;

END_SINGLETON

/// General use
#define PRINT_F(x, ...) Console::getInstance().printf(x, __VA_ARGS__);
#define PRINT_FN(x, ...) Console::getInstance().printfn(x, __VA_ARGS__);
#define ERROR_F(x, ...) Console::getInstance().errorf(x, __VA_ARGS__);
#define ERROR_FN(x, ...) Console::getInstance().errorfn(x, __VA_ARGS__);
/// Debug only
#define D_PRINT_F(x, ...) Console::getInstance().d_printf(x, __VA_ARGS__);
#define D_PRINT_FN(x, ...) Console::getInstance().d_printfn(x, __VA_ARGS__);
#define D_ERROR_F(x, ...) Console::getInstance().d_errorf(x, __VA_ARGS__);
#define D_ERROR_FN(x, ...) Console::getInstance().d_errorfn(x, __VA_ARGS__);


#endif