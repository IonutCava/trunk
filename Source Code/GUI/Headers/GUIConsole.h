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
#ifndef _GUI_CONSOLE_H_
#define _GUI_CONSOLE_H_

#define CEGUI_MAX_INPUT_HISTORY 5

#include <CEGUI/CEGUI.h>
#include "Utility/Headers/Vector.h"
#include "Hardware/Platform/Headers/PlatformDefines.h"
#include "Hardware/Platform/Headers/SharedMutex.h"           //For multi-threading
#include <deque>

	///Maximum number of lines to display in the console Window
#ifdef _DEBUG
#	define _CEGUI_MAX_CONSOLE_ENTRIES  32
#else
#   define _CEGUI_MAX_CONSOLE_ENTRIES  128
#endif

class GUIConsoleCommandParser;
///GUIConsole implementation, CEGUI based, as in the practical tutorial series
class GUIConsole{

    public:
       GUIConsole();                          //< Constructor
	   ~GUIConsole();                         //< Destructor

       void setVisible(bool visible);         //< Hide or show the console
       bool isVisible();                      //< Return true if console is visible, false if is hidden
	   ///add text to the console Window. Uses a text buffer if the console isn't ready for display yet
	   void printText(const std::string& output,bool error);
	
    protected:

       void RegisterHandlers();                                   //< Register our handler functions
       bool Handle_TextSubmitted(const CEGUI::EventArgs &e);      //< Handle when we press Enter after typing
	   bool Handle_TextInput(const CEGUI::EventArgs &e);          //< A key is pressed in the console input editbox

	protected:
	   friend class GUI;
       void CreateCEGUIWindow();                                  //< The function which will load in the CEGUI Window and register event handlers
 	   // Post the message to the ChatHistory listbox with a white color default
       void OutputText(const std::string& inMsg, CEGUI::Colour color = CEGUI::Colour( 0xFFFFFFFF));

	protected:
		bool _init;                          //< used to check if the console is ready
	    CEGUI::Editbox* _editBox;            //< pointer to the editBox to reduce typing and casting
	    CEGUI::Listbox* _outputWindow;       //< pointer to the listbox that will contain all of the text we output to the console
	    GUIConsoleCommandParser* _cmdParser; //< pointer to the command parser instance used
        CEGUI::Window *_consoleWindow;       //< This will be a pointer to the ConsoleRoot Window.
	    std::string _inputBuffer;            //< Used to check the text we are typing so that we don't close the console in the middle of a sentence/command
	    std::deque<std::string >_inputHistory; //< Used to manage the input history
	    I16 _inputHistoryIndex;                //< Used to cycle through history
};

#endif