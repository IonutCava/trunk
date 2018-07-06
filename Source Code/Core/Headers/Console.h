/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */


#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "Core/Headers/Singleton.h"
#include <boost/thread/mutex.hpp>
#include <functional>

namespace Divide {

DEFINE_SINGLETON(Console)
	typedef std::function<void (const char*, bool)> consolePrintCallback;

public:
	void printCopyrightNotice() const;
	const char* printfn(const char* format, ...) const;
	const char* printf(const char* format, ...) const;
	const char* errorfn(const char* format, ...) const;
	const char* errorf(const char* format, ...) const;
#ifdef _DEBUG
	const char* d_printfn(const char* format, ...) const;
	const char* d_printf(const char* format, ...) const;
	const char* d_errorfn(const char* format, ...) const;
	const char* d_errorf(const char* format, ...) const;
#endif

	inline void toggleTimeStamps(const bool state)  {_timestamps = state;}
	inline void bindConsoleOutput(const consolePrintCallback& guiConsoleCallback) {
        _guiConsoleCallback = guiConsoleCallback;
    }

protected:
    Console();
    ~Console();
	const char* output(const char* text,const bool error = false) const;

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

}; //namespace Divide
#endif