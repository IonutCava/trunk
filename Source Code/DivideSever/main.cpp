#include "Server.h"
#include "Utility/Headers/OutputBuffer.h"
#include <EASTL/vector.h>
#include <assert.cpp>
#include <string.cpp>

using namespace Divide;

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned int debugFlags, const char* file, int line)
{
    // this allocator doesn't support alignment
    EASTL_ASSERT(alignment <= 8);
    return malloc(size);
}
void* operator new[](size_t size, const char* pName, int flags, unsigned int debugFlags, const char* file, int line)
{
  return malloc(size);
}
// E
int Vsnprintf8(char* pDestination, size_t n, const char* pFormat, va_list arguments)
{
    #ifdef _MSC_VER
    return _vsnprintf(pDestination, n, pFormat, arguments);
    #else
    return vsnprintf(pDestination, n, pFormat, arguments);
    #endif
}

int main()
{
    /*Console output redirect*/
    std::ofstream myfile("console_server.log");
    outbuf obuf(myfile.rdbuf(), std::cout.rdbuf());
    scoped_streambuf_assignment ssa(std::cout, &obuf);
    /*Console output redirect*/
    stringImpl address = "127.0.0.1";

    Server::getInstance().init(((U16)443),address,true);
    return 0;
}