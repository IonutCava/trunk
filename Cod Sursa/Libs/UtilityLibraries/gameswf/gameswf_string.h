// gameswf_string.h	-- Rob Savoye <rob@welcomehome.org> 2005

// This source code has been donated to the Public Domain. Do whatever
// you want with it.

// Implementation for ActionScript String object.


#ifndef GAMESWF_STRING_H
#define GAMESWF_STRING_H


#include "gameswf_action.h"


namespace gameswf 
	{
	// Dispatcher for handling string methods.
	void string_method(const fn_call& fn, const tu_stringi& method_name, const tu_string& this_string);

	// Constructor for creating ActionScript String object.
	void string_ctor(const fn_call& fn);
		}
 

#endif // GAMESWF_STRING_H
