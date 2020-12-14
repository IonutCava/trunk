#include "stdafx.h"

#include "Headers/GUIConsoleCommandParser.h"

#include "Headers/GUI.h"

#include "AI/PathFinding/NavMeshes/Headers/NavMesh.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Math/Headers/MathHelper.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Audio/Headers/SFXDevice.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {

GUIConsoleCommandParser::GUIConsoleCommandParser(PlatformContext& context, ResourceCache* cache)
    : PlatformContextComponent(context),
      _resCache(cache),
      _sound(nullptr)
{
    _commands[_ID("say")] = [this](const stringImpl& args) { handleSayCommand(args); };
    _commands[_ID("quit")] = [this](const stringImpl& args) { handleQuitCommand(args); };
    _commands[_ID("help")] = [this](const stringImpl& args) { handleHelpCommand(args); };
    _commands[_ID("editparam")] = [this](const stringImpl& args) { handleEditParamCommand(args); };
    _commands[_ID("playsound")] = [this](const stringImpl& args) { handlePlaySoundCommand(args); };
    _commands[_ID("createnavmesh")] = [this](const stringImpl& args) { handleNavMeshCommand(args); };
    _commands[_ID("setfov")] = [this](const stringImpl& args) { handleFOVCommand(args); };
    _commands[_ID("invalidcommand")] = [this](const stringImpl& args) { handleInvalidCommand(args); };
    _commands[_ID("recompileshader")] = [this](const stringImpl& args) { handleShaderRecompileCommand(args); };

    _commandHelp[_ID("say")] = Locale::get(_ID("CONSOLE_SAY_COMMAND_HELP"));
    _commandHelp[_ID("quit")] = Locale::get(_ID("CONSOLE_QUIT_COMMAND_HELP"));
    _commandHelp[_ID("help")] = Locale::get(_ID("CONSOLE_HELP_COMMAND_HELP"));
    _commandHelp[_ID("editparam")] = Locale::get(_ID("CONSOLE_EDITPARAM_COMMAND_HELP"));
    _commandHelp[_ID("playsound")] = Locale::get(_ID("CONSOLE_PLAYSOUND_COMMAND_HELP"));
    _commandHelp[_ID("createnavmesh")] = Locale::get(_ID("CONSOLE_NAVMESH_COMMAND_HELP"));
    _commandHelp[_ID("recompileshader")] = Locale::get(_ID("CONSOLE_SHADER_RECOMPILE_COMMAND_HELP"));
    _commandHelp[_ID("setfov")] = Locale::get(_ID("CONSOLE_CHANGE_FOV_COMMAND_HELP"));
    _commandHelp[_ID("addObject")] = Locale::get(_ID("CONSOLE_ADD_OBJECT_COMMAND_HELP"));
    _commandHelp[_ID("invalidhelp")] = Locale::get(_ID("CONSOLE_INVALID_HELP_ARGUMENT"));
}

bool GUIConsoleCommandParser::processCommand(const stringImpl& commandString) {
    // Be sure we have a string longer than 0
    if (commandString.length() >= 1) {
        // Check if the first letter is a 'command' operator
        if (commandString.at(0) == '/') {
            const stringImpl::size_type commandEnd = commandString.find(' ', 1);
            stringImpl command = commandString.substr(1, commandEnd - 1);
            stringImpl commandArgs = commandString.substr(commandEnd + 1, commandString.length() - (commandEnd + 1));

            if (commandString != commandArgs) {
                commandArgs.clear();
            }
            // convert command to lower case
            for (auto& it : command) {
                it = static_cast<char>(tolower(it));
            }
            if (_commands.find(_ID(command.c_str())) != std::end(_commands)) {
                // we have a valid command
                _commands[_ID(command.c_str())](commandArgs);
            } else {
                // invalid command
                _commands[_ID("invalidcommand")](command);
            }
        } else {
            // no commands, just output what was typed
            Console::printfn("%s", commandString .c_str());  
        }
    }
    return true;
}

void GUIConsoleCommandParser::handleSayCommand(const stringImpl& args) {
    Console::printfn(Locale::get(_ID("CONSOLE_SAY_NAME_TAG")), args.c_str());
}

void GUIConsoleCommandParser::handleQuitCommand(const stringImpl& args) {
    if (!args.empty()) {
        // quit command can take an extra argument. A reason, for example
        Console::printfn(Locale::get(_ID("CONSOLE_QUIT_COMMAND_ARGUMENT")),
                         args.c_str());
    }
    _context.app().RequestShutdown();
}

void GUIConsoleCommandParser::handleHelpCommand(const stringImpl& args) {
    if (args.empty()) {
        Console::printfn(Locale::get(_ID("HELP_CONSOLE_COMMAND")));
        for (const CommandMap::value_type& it : _commands) {
            if (it.first != _ID("invalidhelp") &&
                it.first != _ID("invalidcommand")) {
                Console::printfn("%s", _commandHelp[it.first]);
            }
        }
    } else {
        if (_commandHelp.find(_ID(args.c_str())) != std::end(_commandHelp)) {
            Console::printfn("%s", _commandHelp[_ID(args.c_str())]);
        } else {
            Console::printfn("%s", _commandHelp[_ID("invalidhelp")]);
        }
    }
}

void GUIConsoleCommandParser::handleEditParamCommand(const stringImpl& args) {
    if (context().paramHandler().isParam<stringImpl>(_ID(args.c_str()))) {
        Console::printfn(Locale::get(_ID("CONSOLE_EDITPARAM_FOUND")), args.c_str(),
                         "N/A", "N/A", "N/A");
    } else {
        Console::printfn(Locale::get(_ID("CONSOLE_EDITPARAM_NOT_FOUND")), args.c_str());
    }
}

void GUIConsoleCommandParser::handlePlaySoundCommand(const stringImpl& args) {
    const ResourcePath filename(Paths::g_assetsLocation + args);

    const std::ifstream soundfile(filename.str());
    if (soundfile) {
        // Check extensions (not really, musicwav.abc would still be valid, but
        // still ...)
        if (!hasExtension(filename, "wav") &&
            !hasExtension(filename, "mp3") &&
            !hasExtension(filename, "ogg")) {
            Console::errorfn(Locale::get(_ID("CONSOLE_PLAY_SOUND_INVALID_FORMAT")));
            return;
        }

        auto[name, path] = splitPathToNameAndLocation(filename);

        // The file is valid, so create a descriptor for it
        ResourceDescriptor sound("consoleFilePlayback");
        sound.assetName(name);
        sound.assetLocation(path);
        _sound = CreateResource<AudioDescriptor>(_resCache, sound);
        if (filename.str().find("music") != stringImpl::npos) {
            // play music
            _context.sfx().playMusic(_sound);
        } else {
            // play sound but stop music first if it's playing
            _context.sfx().stopMusic();
            _context.sfx().playSound(_sound);
        }
    } else {
        Console::errorfn(Locale::get(_ID("CONSOLE_PLAY_SOUND_INVALID_FILE")),
                         filename.c_str());
    }
}

void GUIConsoleCommandParser::handleNavMeshCommand(const stringImpl& args) {
    SceneManager* sMgr = _context.kernel().sceneManager();
    SceneGraph* sceneGraph = sMgr->getActiveScene().sceneGraph();
    if (!args.empty()) {
        SceneGraphNode* sgn = sceneGraph->findNode(args.c_str());
        if (!sgn) {
            Console::errorfn(Locale::get(_ID("CONSOLE_NAVMESH_NO_NODE")), args.c_str());
            return;
        }
    }
    AI::AIManager& aiManager = sceneGraph->parentScene().aiManager();
    // Check if we already have a NavMesh created
    AI::Navigation::NavigationMesh* temp = aiManager.getNavMesh(AI::AIEntity::PresetAgentRadius::AGENT_RADIUS_SMALL);
    // Create a new NavMesh if we don't currently have one
    if (!temp) {
        temp = MemoryManager_NEW AI::Navigation::NavigationMesh(_context, *sMgr->recast());
    }
    // Set it's file name
    temp->setFileName(_context.gui().activeScene()->resourceName());
    // Try to load it from file
    bool loaded = temp->load(sceneGraph->getRoot());
    if (!loaded) {
        // If we failed to load it from file, we need to build it first
        loaded = temp->build(
            sceneGraph->getRoot(),
            AI::Navigation::NavigationMesh::CreationCallback(), false);
        // Then save it to file
        temp->save(sceneGraph->getRoot());
    }
    // If we loaded/built the NavMesh correctly, add it to the AIManager
    if (loaded) {
        aiManager.addNavMesh(AI::AIEntity::PresetAgentRadius::AGENT_RADIUS_SMALL, temp);
    }
}

void GUIConsoleCommandParser::handleShaderRecompileCommand(const stringImpl& args) {
    ShaderProgram::RecompileShaderProgram(args.c_str());
}

void GUIConsoleCommandParser::handleFOVCommand(const stringImpl& args) {
    if (!Util::IsNumber(args)) {
        Console::errorfn(Locale::get(_ID("CONSOLE_INVALID_NUMBER")));
        return;
    }

    const I32 FoV = CLAMPED<I32>(atoi(args.c_str()), 40, 140);

    Attorney::SceneManagerCameraAccessor::playerCamera(_context.kernel().sceneManager())->setHorizontalFoV(Angle::DEGREES<F32>(FoV));
}

void GUIConsoleCommandParser::handleInvalidCommand(const stringImpl& args) {
    Console::errorfn(Locale::get(_ID("CONSOLE_INVALID_COMMAND")), args.c_str());
}
};