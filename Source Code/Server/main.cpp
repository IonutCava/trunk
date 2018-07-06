#include "Headers/Server.h"

#include "Utility/Headers/OutputBuffer.h"

using namespace Divide;

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset,
                     const char* pName, int flags, unsigned int debugFlags,
                     const char* file, int line) {
    ACKNOWLEDGE_UNUSED(alignmentOffset);
    ACKNOWLEDGE_UNUSED(pName);
    ACKNOWLEDGE_UNUSED(flags);
    ACKNOWLEDGE_UNUSED(debugFlags);
    ACKNOWLEDGE_UNUSED(file);
    ACKNOWLEDGE_UNUSED(line);

    // this allocator doesn't support alignment
    assert(alignment <= 8);
    return malloc(size);
}
void* operator new[](size_t size, const char* pName, int flags,
                     unsigned int debugFlags, const char* file, int line) {
    ACKNOWLEDGE_UNUSED(pName);
    ACKNOWLEDGE_UNUSED(flags);
    ACKNOWLEDGE_UNUSED(debugFlags);
    ACKNOWLEDGE_UNUSED(file);
    ACKNOWLEDGE_UNUSED(line);

    return malloc(size);
}

int Vsnprintf8(char* pDestination, size_t n, const char* pFormat,
               va_list arguments) {
    return vsnprintf(pDestination, n, pFormat, arguments);
}

int main() {
    /*Console output redirect*/
    std::ofstream myfile("console_server.log");
    outbuf obuf(myfile.rdbuf(), std::cout.rdbuf());
    scoped_streambuf_assignment ssa(std::cout, &obuf);
    /*Console output redirect*/
    stringImpl address = "127.0.0.1";

    Server::getInstance().init(((U16)443), address, true);
    return 0;
}