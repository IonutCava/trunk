#include "Headers/Script.h"
#include "Headers/ScriptBindings.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {
Script::Script(const stringImpl& sourceCode)
    : _scriptSource(sourceCode.c_str()),
      _script(create_chaiscript_stdlib())
{
    bootstrap();
}

Script::Script(const stringImpl& scriptPath, FileType fileType)
    : _script(create_chaiscript_stdlib())
{
    if (!scriptPath.empty()) {
        readFile(scriptPath, _scriptSource, fileType);
    }

    bootstrap();
}

Script::~Script()
{
}

void Script::bootstrap() {
    const SysInfo& systemInfo = const_sysInfo();
    _script.use(systemInfo._pathAndFilename._path + Paths::g_scriptsLocation + "utility.chai");
    _script.add(create_chaiscript_stl_extra());
    _script.add(chaiscript::fun(&Script::handleOutput), "handle_output");
}

void Script::handleOutput(const std::string &msg) {
    Console::printfn(Locale::get(_ID("CONSOLE_SCRIPT_OUTPUT")), msg.c_str());
}

}; //namespace Divide