#ifndef _COMMAND_PARSER_H_
#define _COMMAND_PARSER_H_

#include <string>

///A utility class used to process a string input
class CommandParser {

public:
	CommandParser();  //< Constructor
	~CommandParser(); //< Destructor
	///If we need a parser for something else, just override this
	virtual bool processCommand(const std::string& commandString);

};

#endif