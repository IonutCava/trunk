#ifndef _COMMAND_PARSER_H_
#define _COMMAND_PARSER_H_

#include <string>
#include "UnorderedMap.h"
#include <boost/function.hpp>

namespace Divide {

///A utility class used to process a string input
class CommandParser {
public:
	CommandParser();  //< Constructor
	virtual ~CommandParser(); //< Destructor
	///If we need a parser , just override this
	virtual bool processCommand(const std::string& commandString) = 0;

protected:
	Unordered_map<std::string/*command name*/, boost::function1<void, std::string /*args*/ > > _commandMap;
};

};

#endif