/*
   Copyright (c) 2013 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.
   
   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

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
    void handleNavMeshCommand(const std::string& args);
	void handleShaderRecompileCommand(const std::string& args);
	void handleFOVCommand(const std::string& args);
	void handleInvalidCommand(const std::string& args);

private:
	///Help text for every command
	Unordered_map<std::string, const char* > _commandHelp;
	///used for sound playback
	AudioDescriptor* _sound;
};

#endif