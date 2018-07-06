#include "Headers/GOAPContext.h"

#include "Core/Headers/Console.h"
#include <assert.h>

GOAPContext::GOAPContext() : Context()
{

}

GOAPContext::~GOAPContext()
{

}


void GOAPContext::logEvent(const char *fmt, ...){
#ifdef _DEBUG
    va_list args;
    char text[512] = {};
    va_start(args, fmt);
    assert(_vscprintf(fmt, args) + 3 < 512);
    vsprintf_s(text, sizeof(text), fmt, args);
    strcat(text, "\n");
    va_end(args);
    PRINT_FN(text);
#endif
}