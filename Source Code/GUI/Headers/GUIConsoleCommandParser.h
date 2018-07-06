#ifndef _GUI_CONSOLE_COMMAND_PARSER_H_
#define _GUI_CONSOLE_COMMAND_PARSER_H_

#include "Utility/Headers/CommandParser.h"
///Handle console commands that start with a forward slash
class AudioDescriptor;
class GUIConsoleCommandParser : public CommandParser {
public:
	GUIConsoleCommandParser();
	~GUIConsoleCommandParser();

	bool processCommand(const std::string& commandString);

private:
	typedef Unordered_map<std::string/*command name*/, boost::function1<void, std::string /*args*/ > > CommandMap;

	void handleSayCommand(const std::string& args);
	void handleQuitCommand(const std::string& args);
	void handleHelpCommand(const std::string& args);
	void handleEditParamCommand(const std::string& args);
	void handlePlaySoundCommand(const std::string& args);
	void handleInvalidCommand(const std::string& args);	

private:
	///Help text for every command
	Unordered_map<std::string, const char* > _commandHelp;
	///used for sound playback
	AudioDescriptor* _sound;
};

#endif