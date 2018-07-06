#ifndef _COMMAND_PARSER_H_
#define _COMMAND_PARSER_H_

#include "HashMap.h"
#include <functional>

namespace Divide {

///A utility class used to process a string input
class CommandParser {
public:
	CommandParser();  //< Constructor
	virtual ~CommandParser(); //< Destructor
	///If we need a parser , just override this
	virtual bool processCommand(const stringImpl& commandString) = 0;

protected:
	hashMapImpl<stringImpl/*command name*/, std::function<void (stringImpl /*args*/) > > _commandMap;
};

};

#endif