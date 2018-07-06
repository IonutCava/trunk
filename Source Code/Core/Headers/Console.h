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
	void toggleTimeStamps(bool state){_timestamps = state;}
private:
	void output(const std::string& text);
	boost::mutex io_mutex;
	bool _timestamps;
END_SINGLETON

#endif