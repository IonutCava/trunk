#include "Headers/Resource.h"

#include "core.h"
#include "Core/Headers/Application.h"

U32 maxAlloc = 0;
char* zMaxFile = "";
I16 nMaxLine = 0;

void* operator new(size_t t ,char* zFile, I32 nLine){
#ifdef _DEBUG
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
#endif
	return malloc(t);
}

void operator delete(void * pxData ,char* zFile, I32 nLine){
	 free(pxData);
}

void * malloc_simd(const size_t bytes) {
#if defined(HAVE_ALIGNED_MALLOC)
#   if defined(__GNUC__)
        return aligned_malloc(bytes,ALIGNED_BYTES);
#   else
        return _aligned_malloc(bytes,ALIGNED_BYTES);
#   endif
#elif defined(HAVE_POSIX_MEMALIGN)
	void *ptr=NULL;
	I32 ret = posix_memalign(&ptr,ALIGNED_BYTES,bytes);
	if (ret) THROW_EXCEPTION("posix_memalign returned an error.");
	return ptr;
#else
	// We don't have aligned memory:
	return ::malloc(bytes);
#endif
}

void free_simd(void * pxData){
#if defined(HAVE_ALIGNED_MALLOC)
#   if defined(__GNUC__)
	aligned_free(pxData);
#   else
	_aligned_free(pxData);
#   endif
#else
	free(p);
#endif
}