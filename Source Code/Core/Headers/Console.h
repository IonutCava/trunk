#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "Core/Headers/Singleton.h"
#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>

#ifndef I32
#define I32 int
#endif

DEFINE_SINGLETON(Console)
	typedef boost::function2<void, const char*, bool > consolePrintCallback;

public:
	void printCopyrightNotice() const;
	void printfn(const char* format, ...) const;
	void printf(const char* format, ...) const;
	void errorfn(const char* format, ...) const;
	void errorf(const char* format, ...) const;
#ifdef _DEBUG
	void d_printfn(const char* format, ...) const;
	void d_printf(const char* format, ...) const;
	void d_errorfn(const char* format, ...) const;
	void d_errorf(const char* format, ...) const;
#endif

	inline void toggleTimeStamps(const bool state)                                {_timestamps = state;}
	inline void bindConsoleOutput(const consolePrintCallback& guiConsoleCallback) {_guiConsoleCallback = guiConsoleCallback;}

protected:
    Console();
    ~Console();
	void output(const char* text,const bool error = false) const;

private:
	mutable boost::mutex io_mutex;
	consolePrintCallback _guiConsoleCallback;
	bool _timestamps;
    char* _textBuffer;

END_SINGLETON

/// General use
#define PRINT_F(x, ...) Console::getInstance().printf(x, __VA_ARGS__);
#define PRINT_FN(x, ...) Console::getInstance().printfn(x, __VA_ARGS__);
#define ERROR_F(x, ...) Console::getInstance().errorf(x, __VA_ARGS__);
#define ERROR_FN(x, ...) Console::getInstance().errorfn(x, __VA_ARGS__);

#ifdef _DEBUG
/// Debug only
#define D_PRINT_F(x, ...) Console::getInstance().d_printf(x, __VA_ARGS__);
#define D_PRINT_FN(x, ...) Console::getInstance().d_printfn(x, __VA_ARGS__);
#define D_ERROR_F(x, ...) Console::getInstance().d_errorf(x, __VA_ARGS__);
#define D_ERROR_FN(x, ...) Console::getInstance().d_errorfn(x, __VA_ARGS__);
#else
#define D_PRINT_F(x, ...) ;
#define D_PRINT_FN(x, ...) ;
#define D_ERROR_F(x, ...) ;
#define D_ERROR_FN(x, ...) ;
#endif
/// Misc
#define CONSOLE_TIMESTAMP_OFF() Console::getInstance().toggleTimeStamps(false)
#define CONSOLE_TIMESTAMP_ON() Console::getInstance().toggleTimeStamps(true)

#endif