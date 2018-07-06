#ifndef RESOURCE_H_
#define RESOURCE_H_

#ifdef HIDE_DEBUG_CONSOLE
	#pragma comment( linker,"/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )
#endif
 
#define GETTIME() ((F32)glutGet(GLUT_ELAPSED_TIME)/1000.0f)

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
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

#define GLUT_ICON                       101

#define NEW_PARAM (__FILE__, __LINE__)
#define PLACEMENTNEW_PARAM ,__FILE__, __LINE__
#define NEW_DECL , char* zFile, int nLine

void* operator new(size_t t ,char* zFile, int nLine);
void* operator new(size_t t);
void operator delete(void * pxData ,char* zFile, int nLine);
void operator delete(void *pxData);

#define New new NEW_PARAM
#define PNew(macroparam) new (macroparam PLACEMENTNEW_PARAM)

#define STEP	1
#define SIN		2
#define COS		3
#define CIRCLE  4
#define MPD		5
#define RandomDirection 6
#define string_length sizeof(GLUI_String)

//GLEWContext* glewGetContext();

#endif
