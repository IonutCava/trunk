#ifndef _COMMAND_PARSER_H_
#define _COMMAND_PARSER_H_

#include "String.h"
#include "HashMap.h"
#include <boost/function.hpp>

namespace Divide {

///A utility class used to process a string input
class CommandParser {
public:
	CommandParser();  //< Constructor
	virtual ~CommandParser(); //< Destructor
	///If we need a parser , just override this
	virtual bool processCommand(const stringImpl& commandString) = 0;

protected:
	hashMapImpl<stringImpl/*command name*/, boost::function1<void, stringImpl /*args*/ > > _commandMap;
};

};

#endif