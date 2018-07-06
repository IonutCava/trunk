#ifndef RESOURCE_H_
#define RESOURCE_H_

#ifdef HIDE_DEBUG_CONSOLE
	#pragma comment( linker,"/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )
#endif

#pragma warning(disable:4244)
#pragma warning(disable:4996) //strcpy
 
#define GETTIME()   Framerate::getInstance().getElapsedTime()/1000
#define GETMSTIME() Framerate::getInstance().getElapsedTime()

#define CLAMP(n, min, max) (((n)<(min))?(min):(((n)>(max))?(max):(n)))
#define BIT(x) (1 << (x))

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifdef WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <assert.h>
#include <memory.h>
#include <string.h>
#include <memory.h>
#include <malloc.h>
#include <map>
#include <math.h>
#include <vector>
#include <deque>
#include <list>
#include <time.h>
#include <unordered_map>
#include "Utility/Headers/MathClasses.h"
#include "Utility/Headers/Console.h"
#include "Rendering/Framerate.h" //For time management

#define NEW_PARAM (__FILE__, __LINE__)
#define PLACEMENTNEW_PARAM ,__FILE__, __LINE__
#define NEW_DECL , char* zFile, int nLine

void* operator new(size_t t ,char* zFile, int nLine);
void operator delete(void * pxData ,char* zFile, int nLine);

#define New new NEW_PARAM
#define PNew(macroparam) new (macroparam PLACEMENTNEW_PARAM)

#endif
