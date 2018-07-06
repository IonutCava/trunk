#ifndef RESOURCE_H_
#define RESOURCE_H_

#ifdef HIDE_DEBUG_CONSOLE
	#pragma comment( linker,"/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )
#endif
 
#define GETTIME() ((F32)glutGet(GLUT_ELAPSED_TIME)/1000.0f)
#define GETMSTIME() ((F32)glutGet(GLUT_ELAPSED_TIME))
#ifndef NOMINMAX
#define NOMINMAX
#endif

#define _WIN32_WINNT 0x0501
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
#include <gl/glew.h>
#include <gl/freeglut.h> 
#include <math.h>
#include <vector>
#include <list>
#include <time.h>
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


#endif
