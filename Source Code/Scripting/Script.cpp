#include "stdafx.h"

#include "Headers/Script.h"
#include "Headers/ScriptBindings.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/StringHelper.h"
#include "Utility/Headers/Localization.h"
#include "Platform/File/Headers/FileManagement.h"
#include "Platform/File/Headers/FileUpdateMonitor.h"
#include "Platform/File/Headers/FileWatcherManager.h"

namespace Divide {

namespace {
    UpdateListener s_fileWatcherListener([](const char* atomName, FileUpdateEvent evt) {
        Script::onScriptModify(atomName, evt);
    });
};

I64 Script::s_scriptFileWatcher = -1;

Script::ScriptMap Script::s_scripts;
bool Script::s_scriptsReady = false;

Script::Script(const stringImpl& scriptPathOrCode, FileType fileType)
    : GUIDWrapper(),
      _script(nullptr),
      _scriptFileType(fileType)
{
    if (!scriptPathOrCode.empty()) {
        if (hasExtension(scriptPathOrCode, "chai")) {
            _scriptFile = splitPathToNameAndLocation(scriptPathOrCode);
        } else {
            _scriptSource = scriptPathOrCode;
        }
    }

    compile();
    bootstrap();
    extractAtoms();
    if (s_scriptsReady) {
        hashAlg::insert(s_scripts, std::make_pair(getGUID(), this));
    }
}

Script::~Script()
{
    if (s_scriptsReady) {
        ScriptMap::iterator it = s_scripts.find(getGUID());
        if (it != std::cend(s_scripts)) {
            s_scripts.erase(it);
        }
    }
}

void Script::idle() {
}

bool Script::onStartup() {
    s_scripts.reserve(100);
    s_scriptsReady = true;

    if (!Config::Build::IS_SHIPPING_BUILD) {
        FileWatcher& scriptFileWatcher = FileWatcherManager::allocateWatcher();
        s_scriptFileWatcher = scriptFileWatcher.getGUID();

        s_fileWatcherListener.addIgnoredEndCharacter('~');
        s_fileWatcherListener.addIgnoredExtension("tmp");
        scriptFileWatcher().addWatch(Paths::Scripts::g_scriptsLocation, &s_fileWatcherListener);
        scriptFileWatcher().addWatch(Paths::Scripts::g_scriptsAtomsLocation, &s_fileWatcherListener);
    }

    return true;
}

bool Script::onShutdown() {
    s_scriptsReady = false;

    if (!Config::Build::IS_SHIPPING_BUILD) {
        FileWatcherManager::deallocateWatcher(s_scriptFileWatcher);
        s_scriptFileWatcher = -1;
    }

    s_scripts.clear();

    return true;
}

void Script::compile() {
    if (!_scriptFile._fileName.empty()) {
        readFile(_scriptFile._path + _scriptFile._fileName, _scriptSource, _scriptFileType);
    }
}

void Script::bootstrap() {
    const SysInfo& systemInfo = const_sysInfo();
    const stringImpl& path = systemInfo._pathAndFilename._path;

    vectorImpl<std::string> scriptpath{ path + Paths::Scripts::g_scriptsLocation,
                                         path + Paths::Scripts::g_scriptsAtomsLocation };

    _script = 
        std::make_unique<chaiscript::ChaiScript>(scriptpath,
                                                 scriptpath,
                                                 std::vector<chaiscript::Options> 
                                                 {
                                                    chaiscript::Options::Load_Modules,
                                                    chaiscript::Options::External_Scripts
                                                 });

    _script->add(create_chaiscript_stl_extra());

    _script->add(chaiscript::fun(&Script::handleOutput), "handle_output");
}

void Script::preprocessIncludes(const stringImpl& source, I32 level /*= 0 */) {
    if (level > 32) {
        Console::errorfn(Locale::get(_ID("ERROR_SCRIPT_INCLUD_LIMIT")));
    }

    std::smatch matches;
    stringImpl line, include_file, include_string;

    istringstreamImpl input(source);
    while (std::getline(input, line)) {
        if (std::regex_search(line, matches, Paths::g_usePattern)) {
            include_file = Util::Trim(matches[1].str().c_str());
            _usedAtoms.push_back(include_file);

            // Open the atom file and add the code to the atom cache for future reference
            readFile(Paths::Scripts::g_scriptsLocation + include_file, include_string, FileType::TEXT);
            if (include_string.empty()) {
                readFile(Paths::Scripts::g_scriptsAtomsLocation + include_file, include_string, FileType::TEXT);
            }
            if (!include_string.empty()) {
                preprocessIncludes(include_string, level + 1);
            }
        }
    }
}

void Script::extractAtoms() {
    _usedAtoms.clear();
    if (!_scriptFile._fileName.empty()) {
        _usedAtoms.emplace_back(_scriptFile._fileName);
    }
    if (!_scriptSource.empty()) {
        preprocessIncludes(_scriptSource, 0);
    }
}

void Script::handleOutput(const std::string &msg) {
    Console::printfn(Locale::get(_ID("SCRIPT_CONSOLE_OUTPUT")), msg.c_str());
}

void Script::onScriptModify(const char* script, FileUpdateEvent& evt) {
    vectorImpl<Script*> scriptsToReload;

    for (ScriptMap::value_type it : s_scripts) {
        for (const stringImpl& atom : it.second->_usedAtoms) {
            if (Util::CompareIgnoreCase(atom, script)) {
                scriptsToReload.push_back(it.second);
                break;
            }
        }
    }

    for (Script* it : scriptsToReload) {
        it->compile();
        it->extractAtoms();
    }
}

void Script::caughtException(const char* message, bool isEvalException) const {
    Console::printfn(Locale::get(isEvalException ? _ID("SCRIPT_EVAL_EXCEPTION")
                                                 : _ID("SCRIPT_OTHER_EXCEPTION")),
                     message);
}

}; //namespace Divide