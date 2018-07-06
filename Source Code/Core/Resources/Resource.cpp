#include "Headers/Resource.h"
#include "Core/Headers/Application.h"

U32 maxAlloc = 0;
char* zMaxFile = "";
I16 nMaxLine = 0;

void* operator new(size_t t ,char* zFile, I32 nLine){
	if (t > maxAlloc)	{
		maxAlloc = t;
		zMaxFile = zFile;
		nMaxLine = nLine;
	}

	std::stringstream ss;
	ss << "[ "<< GETTIME() << " ] : New allocation: " << t << " IN: \""
	   << zFile << "\" at line: " << nLine << "." << std::endl
	   << "\t Max Allocation: ["  << maxAlloc << "] in file: \""
	   << zMaxFile << "\" at line: " << nMaxLine  << std::endl << std::endl;
	Application::getInstance().logMemoryAllocation(ss);
	return malloc(t);
}


void operator delete(void * pxData ,char* zFile, I32 nLine){
	free(pxData);
}
