#include "Headers/CommandParser.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"

CommandParser::CommandParser()
{
}

CommandParser::~CommandParser()
{
}

bool CommandParser::processCommand(const std::string& commandString){
	// Be sure we got a string longer than 0
	if (commandString.length() >= 1) {
		// Check if the first letter is a 'command'
		if (commandString.at(0) == '/') 	{
			std::string::size_type commandEnd = commandString.find(" ", 1);
			std::string command = commandString.substr(1, commandEnd - 1);
			std::string commandArgs = commandString.substr(commandEnd + 1, commandString.length() - (commandEnd + 1));
			//convert command to lower case
			for(std::string::size_type i=0; i < command.length(); i++){		
				command[i] = tolower(command[i]);
			}
 
			// Begin processing
 			if (command.compare("say") == 0){
                PRINT_FN("You: %s",commandArgs.c_str());
			}else if (command.compare("quit") == 0)	{
				Application::getInstance().RequestShutdown(); 
			}else if (command.compare("help") == 0)	{
				PRINT_FN("Help text here -Ionut");
			}else if (command.compare("editparam") == 0){
				if(ParamHandler::getInstance().isParam(commandString)){
					PRINT_FN("Editing [ %s ] of value [ %s ] to value [ %s ]", commandString.c_str(), "N/A", "N/A");
				}
			}else{				
				ERROR_FN("< %s > is an invalid command",commandString.c_str());
			}
		} else	{
			PRINT_FN("%s",commandString.c_str()); // no commands, just output what they wrote
		}
	} 
	return true;
}