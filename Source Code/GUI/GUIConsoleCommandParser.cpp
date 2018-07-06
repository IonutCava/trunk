#include "Headers/GUIConsoleCommandParser.h"

#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/AIManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "AI/PathFinding/NavMeshes/Headers/NavMesh.h"  ///< For NavMesh creation

#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"

#include "Platform/Video/Shaders/Headers/ShaderManager.h"

namespace Divide {

GUIConsoleCommandParser::GUIConsoleCommandParser() : _sound(nullptr) {
    _commandMap["say"] =
        DELEGATE_BIND(&GUIConsoleCommandParser::handleSayCommand, this,
                      std::placeholders::_1);
    _commandMap["quit"] =
        DELEGATE_BIND(&GUIConsoleCommandParser::handleQuitCommand, this,
                      std::placeholders::_1);
    _commandMap["help"] =
        DELEGATE_BIND(&GUIConsoleCommandParser::handleHelpCommand, this,
                      std::placeholders::_1);
    _commandMap["editparam"] =
        DELEGATE_BIND(&GUIConsoleCommandParser::handleEditParamCommand, this,
                      std::placeholders::_1);
    _commandMap["playsound"] =
        DELEGATE_BIND(&GUIConsoleCommandParser::handlePlaySoundCommand, this,
                      std::placeholders::_1);
    _commandMap["createnavmesh"] =
        DELEGATE_BIND(&GUIConsoleCommandParser::handleNavMeshCommand, this,
                      std::placeholders::_1);
    _commandMap["setfov"] =
        DELEGATE_BIND(&GUIConsoleCommandParser::handleFOVCommand, this,
                      std::placeholders::_1);
    _commandMap["invalidcommand"] =
        DELEGATE_BIND(&GUIConsoleCommandParser::handleInvalidCommand, this,
                      std::placeholders::_1);
    _commandMap["addobject"] = DELEGATE_BIND(
        &GUIConsoleCommandParser::handleAddObject, this, std::placeholders::_1);
    _commandMap["recompileshader"] =
        DELEGATE_BIND(&GUIConsoleCommandParser::handleShaderRecompileCommand,
                      this, std::placeholders::_1);

    _commandHelp["say"] = Locale::get("CONSOLE_SAY_COMMAND_HELP");
    _commandHelp["quit"] = Locale::get("CONSOLE_QUIT_COMMAND_HELP");
    _commandHelp["help"] = Locale::get("CONSOLE_HELP_COMMAND_HELP");
    _commandHelp["editparam"] = Locale::get("CONSOLE_EDITPARAM_COMMAND_HELP");
    _commandHelp["playsound"] = Locale::get("CONSOLE_PLAYSOUND_COMMAND_HELP");
    _commandHelp["createnavmesh"] = Locale::get("CONSOLE_NAVMESH_COMMAND_HELP");
    _commandHelp["recompileshader"] =
        Locale::get("CONSOLE_SHADER_RECOMPILE_COMMAND_HELP");
    _commandHelp["setfov"] = Locale::get("CONSOLE_CHANGE_FOV_COMMAND_HELP");
    _commandHelp["addObject"] = Locale::get("CONSOLE_ADD_OBJECT_COMMAND_HELP");
    _commandHelp["invalidhelp"] = Locale::get("CONSOLE_INVALID_HELP_ARGUMENT");
}

GUIConsoleCommandParser::~GUIConsoleCommandParser() {
    if (_sound != nullptr) {
        RemoveResource(_sound);
    }
}

bool GUIConsoleCommandParser::processCommand(const stringImpl& commandString) {
    // Be sure we have a string longer than 0
    if (commandString.length() >= 1) {
        // Check if the first letter is a 'command' operator
        if (commandString.at(0) == '/') {
            stringImpl::size_type commandEnd = commandString.find(" ", 1);
            stringImpl command = commandString.substr(1, commandEnd - 1);
            stringImpl commandArgs = commandString.substr(
                commandEnd + 1, commandString.length() - (commandEnd + 1));
            if (commandString.compare(commandArgs) == 0) commandArgs.clear();
            // convert command to lower case
            for (stringImpl::size_type i = 0; i < command.length(); i++) {
                command[i] = tolower(command[i]);
            }
            if (_commandMap.find(command) != std::end(_commandMap)) {
                // we have a valid command
                _commandMap[command](commandArgs);
            } else {
                // invalid command
                _commandMap["invalidcommand"](command);
            }
        } else {
            Console::printfn(
                "%s", commandString
                          .c_str());  // no commands, just output what was typed
        }
    }
    return true;
}

void GUIConsoleCommandParser::handleSayCommand(const stringImpl& args) {
    Console::printfn(Locale::get("CONSOLE_SAY_NAME_TAG"), args.c_str());
}

void GUIConsoleCommandParser::handleQuitCommand(const stringImpl& args) {
    if (!args.empty()) {
        // quit command can take an extra argument. A reason, for example
        Console::printfn(Locale::get("CONSOLE_QUIT_COMMAND_ARGUMENT"),
                         args.c_str());
    }
    Application::getInstance().RequestShutdown();
}

void GUIConsoleCommandParser::handleHelpCommand(const stringImpl& args) {
    if (args.empty()) {
        Console::printfn(Locale::get("HELP_CONSOLE_COMMAND"));
        for (const CommandMap::value_type& it : _commandMap) {
            if (it.first.find("invalid") == stringImpl::npos) {
                Console::printfn("/%s - %s", it.first.c_str(),
                                 _commandHelp[it.first]);
            }
        }
    } else {
        if (_commandHelp.find(args) != std::end(_commandHelp)) {
            Console::printfn("%s", _commandHelp[args]);
        } else {
            Console::printfn("%s", _commandHelp["invalidhelp"]);
        }
    }
}

void GUIConsoleCommandParser::handleEditParamCommand(const stringImpl& args) {
    if (ParamHandler::getInstance().isParam<stringImpl>(args)) {
        Console::printfn(Locale::get("CONSOLE_EDITPARAM_FOUND"), args.c_str(),
                         "N/A", "N/A", "N/A");
    } else {
        Console::printfn(Locale::get("CONSOLE_EDITPARAM_NOT_FOUND"),
                         args.c_str());
    }
}

void GUIConsoleCommandParser::handlePlaySoundCommand(const stringImpl& args) {
    std::string filename =
        ParamHandler::getInstance().getParam<std::string>("assetsLocation") +
        "/" + stringAlg::fromBase(args);
    std::ifstream soundfile(filename.c_str());
    if (soundfile) {
        // Check extensions (not really, musicwav.abc would still be valid, but
        // still ...)
        if (filename.substr(filename.length() - 4, filename.length())
                    .compare(".wav") != 0 &&
            filename.substr(filename.length() - 4, filename.length())
                    .compare(".mp3") != 0 &&
            filename.substr(filename.length() - 4, filename.length())
                    .compare(".ogg") != 0) {
            Console::errorfn(Locale::get("CONSOLE_PLAY_SOUND_INVALID_FORMAT"));
            return;
        }
        if (_sound != nullptr) RemoveResource(_sound);

        // The file is valid, so create a descriptor for it
        ResourceDescriptor sound("consoleFilePlayback");
        sound.setResourceLocation(stringAlg::toBase(filename));
        sound.setFlag(false);
        _sound = CreateResource<AudioDescriptor>(sound);
        if (filename.find("music") != stringImpl::npos) {
            // play music
            SFXDevice::getInstance().playMusic(_sound);
        } else {
            // play sound but stop music first if it's playing
            SFXDevice::getInstance().stopMusic();
            SFXDevice::getInstance().playSound(_sound);
        }
    } else {
        Console::errorfn(Locale::get("CONSOLE_PLAY_SOUND_INVALID_FILE"),
                         filename.c_str());
    }
}

void GUIConsoleCommandParser::handleNavMeshCommand(const stringImpl& args) {
    SceneGraphNode* sgn = nullptr;
    if (!args.empty()) {
        sgn = GET_ACTIVE_SCENEGRAPH().findNode("args");
        if (!sgn) {
            Console::errorfn(Locale::get("CONSOLE_NAVMESH_NO_NODE"),
                             args.c_str());
            return;
        }
    }
    // Check if we already have a NavMesh created
    AI::Navigation::NavigationMesh* temp =
        AI::AIManager::getInstance().getNavMesh(
            AI::AIEntity::PresetAgentRadius::AGENT_RADIUS_SMALL);
    // Create a new NavMesh if we don't currently have one
    if (!temp) {
        temp = MemoryManager_NEW AI::Navigation::NavigationMesh();
    }
    // Set it's file name
    temp->setFileName(GET_ACTIVE_SCENE()->getName());
    // Try to load it from file
    bool loaded = temp->load(GET_ACTIVE_SCENEGRAPH().getRoot());
    if (!loaded) {
        // If we failed to load it from file, we need to build it first
        loaded = temp->build(
            GET_ACTIVE_SCENEGRAPH().getRoot(), 
            AI::Navigation::NavigationMesh::CreationCallback(), false);
        // Then save it to file
        temp->save();
    }
    // If we loaded/built the NavMesh correctly, add it to the AIManager
    if (loaded) {
        AI::AIManager::getInstance().addNavMesh(
            AI::AIEntity::PresetAgentRadius::AGENT_RADIUS_SMALL, temp);
    }
}

void GUIConsoleCommandParser::handleShaderRecompileCommand(
    const stringImpl& args) {
    ShaderManager::getInstance().recompileShaderProgram(args);
}

void GUIConsoleCommandParser::handleFOVCommand(const stringImpl& args) {
    if (!Util::isNumber(args)) {
        Console::errorfn(Locale::get("CONSOLE_INVALID_NUMBER"));
        return;
    }
    I32 FoV = (atoi(args.c_str()));
    CLAMP<I32>(FoV, 40, 140);

    Application::getInstance()
        .getKernel()
        .getCameraMgr()
        .getActiveCamera()
        ->setHorizontalFoV(FoV);
}

void GUIConsoleCommandParser::handleAddObject(const stringImpl& args) {
    std::istringstream ss(args.c_str());
    std::string args1, args2;
    std::getline(ss, args1, ',');
    std::getline(ss, args2, ',');

    float scale = 1.0f;
    if (!Util::isNumber(args2.c_str())) {
        Console::errorfn(Locale::get("CONSOLE_INVALID_NUMBER"));
    } else {
        scale = (F32)atof(args2.c_str());
    }
    std::string assetLocation(
        ParamHandler::getInstance().getParam<std::string>("assetsLocation") +
        "/");

    FileData model;
    model.ItemName = stringAlg::toBase(args1 + "_console" + args2);
    model.ModelName = stringAlg::toBase(
        ((args1.compare("Box3D") == 0 || args1.compare("Sphere3D") == 0)
             ? ""
             : assetLocation) +
        args1);
    model.position =
        GET_ACTIVE_SCENE()->state().renderState().getCamera().getEye();
    model.data = 1.0f;
    model.scale = vec3<F32>(scale);
    model.orientation =
        GET_ACTIVE_SCENE()->state().renderState().getCamera().getEuler();
    model.type = (args1.compare("Box3D") == 0 || args1.compare("Sphere3D") == 0)
                     ? GeometryType::PRIMITIVE
                     : GeometryType::GEOMETRY;
    model.version = 1.0f;
    model.staticUsage = false;
    model.navigationUsage = true;
    model.useHighDetailNavMesh = true;
    model.physicsUsage = true;
    model.physicsPushable = true;
    GET_ACTIVE_SCENE()->addModel(model);
}

void GUIConsoleCommandParser::handleInvalidCommand(const stringImpl& args) {
    Console::errorfn(Locale::get("CONSOLE_INVALID_COMMAND"), args.c_str());
}
};