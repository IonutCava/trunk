#include "Headers/Script.h"
#include <chaiscript/chaiscript_stdlib.hpp>

namespace Divide {
Script::Script(const stringImpl& sourceCode)
    : _scriptSource(sourceCode.c_str()),
     _script(chaiscript::Std_Lib::library())
{
}

Script::Script(const stringImpl& scriptPath, FileType fileType)
    : _script(chaiscript::Std_Lib::library())
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