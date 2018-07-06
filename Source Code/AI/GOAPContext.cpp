#include "Headers/GOAPContext.h"

#include "Core/Headers/Console.h"
#include <assert.h>

using namespace AI;

GOAPContext::GOAPContext() : Context()
{
    _logLevel = LOG_LEVEL_NORMAL;
}

GOAPContext::~GOAPContext()
{

}


void GOAPContext::logEvent(const char *fmt, ...){
    if (_logLevel == LOG_LEVEL_NONE) {
        return;
    }
#ifdef _DEBUG
    va_list args;
    char text[4096 * 16] = {};
    va_start(args, fmt);
    assert(_vscprintf(fmt, args) + 3 < 4096 * 16);
    vsprintf_s(text, sizeof(text), fmt, args);
    strcat(text, "\n");
    va_end(args);
    PRINT_FN(text);
#endif
}