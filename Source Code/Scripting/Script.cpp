#include "Headers/Script.h"
#include <chaiscript/chaiscript_stdlib.hpp>

namespace Divide {
Script::Script(const stringImpl& sourceCode)
    : _scriptSource(sourceCode.c_str())
{
}

Script::Script(const stringImpl& scriptPath, FileType fileType)
{
    if (!scriptPath.empty()) {
        stringImpl source;
        readFile(scriptPath, source, fileType);
        _scriptSource = source.c_str();
    }
}

Script::~Script()
{
}

}; //namespace Divide