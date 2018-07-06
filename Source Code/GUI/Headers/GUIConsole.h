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
	   void printText(const char* output,bool error);
	
    protected:

       void RegisterHandlers();                                   //< Register our handler functions
       bool Handle_TextSubmitted(const CEGUI::EventArgs &e);      //< Handle when we press Enter after typing
	   bool Handle_TextInput(const CEGUI::EventArgs &e);          //< A key is pressed in the console input editbox

	protected:
	   friend class GUI;
       void CreateCEGUIWindow();                                  //< The function which will load in the CEGUI Window and register event handlers
 	   // Post the message to the ChatHistory listbox with a white color default
       void OutputText(const char* inMsg, const bool error = false);

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