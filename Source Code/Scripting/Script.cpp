#include "Headers/Script.h"
#include "Headers/ScriptBindings.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {
Script::Script(const stringImpl& sourceCode)
    : _scriptSource(sourceCode.c_str()),
      _script(create_chaiscript_stdlib())
{
    _script.use(Paths::g_scriptsLocation + "utility.chai");
    _script.add(create_chaiscript_stl_extra());
    _script.add(chaiscript::fun(&Script::handleOutput), "handle_output");
}

Script::Script(const stringImpl& scriptPath, FileType fileType)
    : _script(create_chaiscript_stdlib())
{
    if (!scriptPath.empty()) {
        stringImpl source;
        readFile(scriptPath, source, fileType);
        _scriptSource = "use(utility.chai);\r\n" + source;
    }

    _script.add(create_chaiscript_stl_extra());
    _script.add(chaiscript::fun(&Script::handleOutput), "handle_output");
}

Script::~Script()
{
}


void Script::handleOutput(const std::string &msg) {
    Console::printfn(Locale::get(_ID("CONSOLE_SCRIPT_OUTPUT")), msg.c_str());
}

}; //namespace Divide