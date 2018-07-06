#include "Headers/GUIConsoleCommandParser.h"

#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"
#include "AI/PathFinding/Headers/NavigationMesh.h" ///< For NavMesh creation

GUIConsoleCommandParser::GUIConsoleCommandParser() : _sound(NULL)
{
	_commandMap.insert(std::make_pair("say",boost::bind(&GUIConsoleCommandParser::handleSayCommand,this,_1)));
	_commandMap.insert(std::make_pair("quit",boost::bind(&GUIConsoleCommandParser::handleQuitCommand,this,_1)));
	_commandMap.insert(std::make_pair("help",boost::bind(&GUIConsoleCommandParser::handleHelpCommand,this,_1)));
	_commandMap.insert(std::make_pair("editparam",boost::bind(&GUIConsoleCommandParser::handleEditParamCommand,this,_1)));
	_commandMap.insert(std::make_pair("playsound",boost::bind(&GUIConsoleCommandParser::handlePlaySoundCommand,this,_1)));
    _commandMap.insert(std::make_pair("createnavmesh",boost::bind(&GUIConsoleCommandParser::handleNavMeshCommand,this,_1)));
	_commandMap.insert(std::make_pair("invalidcommand",boost::bind(&GUIConsoleCommandParser::handleInvalidCommand,this,_1)));


	_commandHelp.insert(std::make_pair("say",Locale::get("CONSOLE_SAY_COMMAND_HELP")));
	_commandHelp.insert(std::make_pair("quit",Locale::get("CONSOLE_QUIT_COMMAND_HELP")));
	_commandHelp.insert(std::make_pair("help",Locale::get("CONSOLE_HELP_COMMAND_HELP")));
	_commandHelp.insert(std::make_pair("editparam",Locale::get("CONSOLE_EDITPARAM_COMMAND_HELP")));
	_commandHelp.insert(std::make_pair("playsound",Locale::get("CONSOLE_PLAYSOUND_COMMAND_HELP")));
    _commandHelp.insert(std::make_pair("createNavMesh",Locale::get("CONSOLE_NAVMESH_COMMAND_HELP")));
	_commandHelp.insert(std::make_pair("invalidhelp",Locale::get("CONSOLE_INVALID_HELP_ARGUMENT")));
}

GUIConsoleCommandParser::~GUIConsoleCommandParser()
{
	if(_sound != NULL){
		RemoveResource(_sound);
	}
}

bool GUIConsoleCommandParser::processCommand(const std::string& commandString){
	// Be sure we have a string longer than 0
	if (commandString.length() >= 1) {
		// Check if the first letter is a 'command' operator
		if (commandString.at(0) == '/') 	{
			std::string::size_type commandEnd = commandString.find(" ", 1);
			std::string command = commandString.substr(1, commandEnd - 1);
			std::string commandArgs = commandString.substr(commandEnd + 1, commandString.length() - (commandEnd + 1));
			if(commandString.compare(commandArgs) == 0) commandArgs.clear();
			//convert command to lower case
			for(std::string::size_type i=0; i < command.length(); i++){		
				command[i] = tolower(command[i]);
			}
			if(_commandMap.find(command) != _commandMap.end()){
				//we have a valid command
				_commandMap[command](commandArgs);
			}else{
				//invalid command
				_commandMap["invalidcommand"](command);
			}
		} else	{
			PRINT_FN("%s",commandString.c_str()); // no commands, just output what was typed
		}
	} 
	return true;
}

void GUIConsoleCommandParser::handleSayCommand(const std::string& args){
	PRINT_FN(Locale::get("CONSOLE_SAY_NAME_TAG"),args.c_str());
}

void GUIConsoleCommandParser::handleQuitCommand(const std::string& args){
	if(!args.empty()){
		//quit command can take an extra argument. A reason, for example
		PRINT_FN(Locale::get("CONSOLE_QUIT_COMMAND_ARGUMENT"), args.c_str());
	}
	Application::getInstance().RequestShutdown();
}

void GUIConsoleCommandParser::handleHelpCommand(const std::string& args){
	if(args.empty()){
		PRINT_FN(Locale::get("HELP_CONSOLE_COMMAND"));
		for_each(CommandMap::value_type& it,_commandMap){
            if(it.first.find("invalid") == std::string::npos){
			    PRINT_FN("/%s - %s",it.first.c_str(),_commandHelp[it.first]);
            }
		}
	}else{
		if(_commandHelp.find(args) != _commandHelp.end()){
			PRINT_FN("%s", _commandHelp[args]);
		}else{
			PRINT_FN("%s",_commandHelp["invalidhelp"]);
		}
	}
}

void GUIConsoleCommandParser::handleEditParamCommand(const std::string& args){
	if(ParamHandler::getInstance().isParam(args)){
		PRINT_FN(Locale::get("CONSOLE_EDITPARAM_FOUND"), args.c_str(), 
														 ParamHandler::getInstance().getType(args),
														 "N/A", "N/A");
	}else{
		PRINT_FN(Locale::get("CONSOLE_EDITPARAM_NOT_FOUND"), args.c_str());
	}
}

void GUIConsoleCommandParser::handlePlaySoundCommand(const std::string& args){
	std::string filename = ParamHandler::getInstance().getParam<std::string>("assetsLocation")+"/" + args;
	std::ifstream soundfile(filename.c_str());
	if (soundfile) {
		//Check extensions (not really, musicwav.abc would still be valid, but still ...)
		if(filename.substr(filename.length()-4,filename.length()).compare(".wav") != 0 &&
		   filename.substr(filename.length()-4,filename.length()).compare(".mp3") != 0 &&
		   filename.substr(filename.length()-4,filename.length()).compare(".ogg") != 0){
			   ERROR_FN(Locale::get("CONSOLE_PLAY_SOUND_INVALID_FORMAT"));
			   return;
		}
		if(_sound != NULL) RemoveResource(_sound);

		//The file is valid, so create a descriptor for it
		ResourceDescriptor sound("consoleFilePlayback");
		sound.setResourceLocation(filename);
		sound.setFlag(false);
		_sound = CreateResource<AudioDescriptor>(sound);
		if(filename.find("music") != std::string::npos){
			//play music
			SFXDevice::getInstance().playMusic(_sound);
		}else{
			//play sound but stop music first if it's playing
			SFXDevice::getInstance().stopMusic();
			SFXDevice::getInstance().playSound(_sound);
		}
	}else{
		ERROR_FN(Locale::get("CONSOLE_PLAY_SOUND_INVALID_FILE"),filename.c_str());
	}
	
}

void GUIConsoleCommandParser::handleNavMeshCommand(const std::string& args){
    SceneGraphNode* sgn = NULL;
    if(!args.empty()){
        sgn = GET_ACTIVE_SCENE()->getSceneGraph()->findNode("args");
        if(!sgn){
            ERROR_FN(Locale::get("CONSOLE_NAVMESH_NO_NODE"),args.c_str());
            return;
        }
    }
    Navigation::NavigationMesh* temp = New Navigation::NavigationMesh();
    temp->build(sgn,false);
    SAFE_DELETE(temp);
}

void GUIConsoleCommandParser::handleInvalidCommand(const std::string& args){
	ERROR_FN(Locale::get("CONSOLE_INVALID_COMMAND"),args.c_str());
}