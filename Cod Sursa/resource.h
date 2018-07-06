#ifndef RESOURCE_H_
#define RESOURCE_H_

#ifdef HIDE_DEBUG_CONSOLE
	#pragma comment( linker,"/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )
#endif

#pragma warning(disable:4244)
 
#define GETTIME()   GFXDevice::getInstance().getTime()
#define GETMSTIME() GFXDevice::getInstance().getMSTime()

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
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
#include "Utility/Headers/DataTypes.h"

#define NEW_PARAM (__FILE__, __LINE__)
#define PLACEMENTNEW_PARAM ,__FILE__, __LINE__
#define NEW_DECL , char* zFile, int nLine

void* operator new(size_t t ,char* zFile, int nLine);
void* operator new(size_t t);
void operator delete(void * pxData ,char* zFile, int nLine);
void operator delete(void *pxData);

#define New new NEW_PARAM
#define PNew(macroparam) new (macroparam PLACEMENTNEW_PARAM)

using namespace std;
using namespace tr1;

#endif
