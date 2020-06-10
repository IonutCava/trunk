#include "stdafx.h"

#include "Headers/StringHelper.h"

namespace Divide {
namespace Util {

bool FindCommandLineArgument(const int argc, char** argv, const char* target_arg, const char* arg_prefix) {
    stringImpl tempArg(arg_prefix);
    tempArg += target_arg;
    const char* target = tempArg.c_str();

    for (int i = 0; i < argc; ++i) {
        if (_strcmpi(argv[i], target) == 0) {
            return true;
        }
    }
    return false;
}

bool IsNumber(const char* s) {
    F32 number = 0.0f;
    if (istringstreamImpl(s) >> number) {
        return !(IS_ZERO(number) && s[0] != 0);
    }
    return false;
}

void CStringRemoveChar(char* str, const char charToRemove) noexcept {
    char *pr = str, *pw = str;
    while (*pr) {
        *pw = *pr++;
        pw += (*pw != charToRemove);
    }
    *pw = '\0';
}

bool IsEmptyOrNull(const char* str) noexcept {
    return str == nullptr || (str[0] == '\0');
}
}; //namespace Util

}; //namespace Divide