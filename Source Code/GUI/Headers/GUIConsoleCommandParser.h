/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */
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
    void handleNavMeshCommand(const std::string& args);

private:
	///Help text for every command
	Unordered_map<std::string, const char* > _commandHelp;
	///used for sound playback
	AudioDescriptor* _sound;
};

#endif