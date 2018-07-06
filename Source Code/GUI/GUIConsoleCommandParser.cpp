#include "stdafx.h"

#include "Headers/GUIConsoleCommandParser.h"

#include "Headers/GUI.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/XMLEntryData.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Math/Headers/MathHelper.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "AI/PathFinding/NavMeshes/Headers/NavMesh.h"  ///< For NavMesh creation
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Predefined/Headers/Box3D.h"
#include "Geometry/Shapes/Predefined/Headers/Sphere3D.h"

namespace Divide {

GUIConsoleCommandParser::GUIConsoleCommandParser(PlatformContext& context, ResourceCache& cache)
    : PlatformContextComponent(context),
      _resCache(cache),
      _sound(nullptr)
{
    _commandMap[_ID("say")] = [this](const stringImpl& args) { handleSayCommand(args); };
    _commandMap[_ID("quit")] = [this](const stringImpl& args) { handleQuitCommand(args); };
    _commandMap[_ID("help")] = [this](const stringImpl& args) { handleHelpCommand(args); };
    _commandMap[_ID("editparam")] = [this](const stringImpl& args) { handleEditParamCommand(args); };
    _commandMap[_ID("playsound")] = [this](const stringImpl& args) { handlePlaySoundCommand(args); };
    _commandMap[_ID("createnavmesh")] = [this](const stringImpl& args) { handleNavMeshCommand(args); };
    _commandMap[_ID("setfov")] = [this](const stringImpl& args) { handleFOVCommand(args); };
    _commandMap[_ID("invalidcommand")] = [this](const stringImpl& args) { handleInvalidCommand(args); };
    _commandMap[_ID("addobject")] = [this](const stringImpl& args) { handleAddObject(args); };
    _commandMap[_ID("recompileshader")] = [this](const stringImpl& args) { handleShaderRecompileCommand(args); };

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

GUIConsoleCommandParser::~GUIConsoleCommandParser()
{
}

bool GUIConsoleCommandParser::processCommand(const stringImpl& commandString) {
    // Be sure we have a string longer than 0
    if (commandString.length() >= 1) {
        // Check if the first letter is a 'command' operator
        if (commandString.at(0) == '/') {
            stringImpl::size_type commandEnd = commandString.find(" ", 1);
            stringImpl command = commandString.substr(1, commandEnd - 1);
            stringImpl commandArgs = commandString.substr(commandEnd + 1, commandString.length() - (commandEnd + 1));

            if (commandString.compare(commandArgs) == 0) {
                commandArgs.clear();
            }
            // convert command to lower case
            for (stringImpl::size_type i = 0; i < command.length(); i++) {
                command[i] = static_cast<char>(tolower(command[i]));
            }
            if (_commandMap.find(_ID_RT(command)) != std::end(_commandMap)) {
                // we have a valid command
                _commandMap[_ID_RT(command)](commandArgs);
            } else {
                // invalid command
                _commandMap[_ID("invalidcommand")](command);
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
        for (const CommandMap::value_type& it : _commandMap) {
            if (it.first != _ID("invalidhelp") &&
                it.first != _ID("invalidcommand")) {
                Console::printfn("%s", _commandHelp[it.first]);
            }
        }
    } else {
        if (_commandHelp.find(_ID_RT(args)) != std::end(_commandHelp)) {
            Console::printfn("%s", _commandHelp[_ID_RT(args)]);
        } else {
            Console::printfn("%s", _commandHelp[_ID("invalidhelp")]);
        }
    }
}

void GUIConsoleCommandParser::handleEditParamCommand(const stringImpl& args) {
    if (ParamHandler::instance().isParam<stringImpl>(args.c_str())) {
        Console::printfn(Locale::get(_ID("CONSOLE_EDITPARAM_FOUND")), args.c_str(),
                         "N/A", "N/A", "N/A");
    } else {
        Console::printfn(Locale::get(_ID("CONSOLE_EDITPARAM_NOT_FOUND")), args.c_str());
    }
}

void GUIConsoleCommandParser::handlePlaySoundCommand(const stringImpl& args) {
    stringImpl filename(Paths::g_assetsLocation + args);

    std::ifstream soundfile(filename.c_str());
    if (soundfile) {
        // Check extensions (not really, musicwav.abc would still be valid, but
        // still ...)
        if (!hasExtension(filename, "wav") &&
            !hasExtension(filename, "mp3") &&
            !hasExtension(filename, "ogg")) {
            Console::errorfn(Locale::get(_ID("CONSOLE_PLAY_SOUND_INVALID_FORMAT")));
            return;
        }

        FileWithPath fileResult = splitPathToNameAndLocation(filename);
        const stringImpl& name = fileResult._fileName;
        const stringImpl& path = fileResult._path;

        // The file is valid, so create a descriptor for it
        ResourceDescriptor sound("consoleFilePlayback");
        sound.setResourceName(name);
        sound.setResourceLocation(path);
        sound.setFlag(false);
        _sound = CreateResource<AudioDescriptor>(_resCache, sound);
        if (filename.find("music") != stringImpl::npos) {
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
    SceneGraph& sceneGraph = _context.gui().activeScene()->sceneGraph();
    if (!args.empty()) {
        SceneGraphNode* sgn = sceneGraph.findNode(args);
        if (!sgn) {
            Console::errorfn(Locale::get(_ID("CONSOLE_NAVMESH_NO_NODE")), args.c_str());
            return;
        }
    }
    AI::AIManager& aiManager = sceneGraph.parentScene().aiManager();
    // Check if we already have a NavMesh created
    AI::Navigation::NavigationMesh* temp = aiManager.getNavMesh(AI::AIEntity::PresetAgentRadius::AGENT_RADIUS_SMALL);
    // Create a new NavMesh if we don't currently have one
    if (!temp) {
        temp = MemoryManager_NEW AI::Navigation::NavigationMesh(_context);
    }
    // Set it's file name
    temp->setFileName(_context.gui().activeScene()->name());
    // Try to load it from file
    bool loaded = temp->load(sceneGraph.getRoot());
    if (!loaded) {
        // If we failed to load it from file, we need to build it first
        loaded = temp->build(
            sceneGraph.getRoot(),
            AI::Navigation::NavigationMesh::CreationCallback(), false);
        // Then save it to file
        temp->save(sceneGraph.getRoot());
    }
    // If we loaded/built the NavMesh correctly, add it to the AIManager
    if (loaded) {
        aiManager.addNavMesh(AI::AIEntity::PresetAgentRadius::AGENT_RADIUS_SMALL, temp);
    }
}

void GUIConsoleCommandParser::handleShaderRecompileCommand(const stringImpl& args) {
    ShaderProgram::recompileShaderProgram(args);
}

void GUIConsoleCommandParser::handleFOVCommand(const stringImpl& args) {
    if (!Util::IsNumber(args)) {
        Console::errorfn(Locale::get(_ID("CONSOLE_INVALID_NUMBER")));
        return;
    }
    I32 FoV = (atoi(args.c_str()));
    CLAMP<I32>(FoV, 40, 140);

    Attorney::SceneManagerCameraAccessor::playerCamera(_context.gfx().parent().sceneManager())->setHorizontalFoV(Angle::DEGREES<F32>(FoV));
}

void GUIConsoleCommandParser::handleAddObject(const stringImpl& args) {
    std::istringstream ss(args.c_str());
    stringImpl args1, args2;
    std::getline(ss, args1, ',');
    std::getline(ss, args2, ',');

    Camera* cam = Attorney::SceneManagerCameraAccessor::playerCamera(_context.gfx().parent().sceneManager());

    float scale = 1.0f;
    if (!Util::IsNumber(args2.c_str())) {
        Console::errorfn(Locale::get(_ID("CONSOLE_INVALID_NUMBER")));
    } else {
        scale = to_F32(atof(args2.c_str()));
    }
    stringImpl assetLocation(Paths::g_assetsLocation);

    FileData model;
    model.ItemName = args1 + "_console" + args;
    model.ModelName = 
        ((args1.compare("Box3D") == 0 || args1.compare("Sphere3D") == 0)
             ? ""
             : assetLocation) +
        args1;
    model.position = cam->getEye();
    model.data = 1.0f;
    model.scale = vec3<F32>(scale);
    model.orientation = cam->getEuler();
    model.type = (args1.compare("Box3D") == 0 || args1.compare("Sphere3D") == 0)
                     ? GeometryType::PRIMITIVE
                     : GeometryType::GEOMETRY;
    model.version = 1.0f;
    model.isUnit = false;
    model.staticUsage = false;
    model.navigationUsage = true;
    model.useHighDetailNavMesh = true;
    model.physicsUsage = true;
    model.physicsStatic = true;
    _context.gui().activeScene()->addModel(model);
}

void GUIConsoleCommandParser::handleInvalidCommand(const stringImpl& args) {
    Console::errorfn(Locale::get(_ID("CONSOLE_INVALID_COMMAND")), args.c_str());
}
};