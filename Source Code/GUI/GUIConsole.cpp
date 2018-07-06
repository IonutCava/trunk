#include "CEGUI.h"

#include "Headers/GUIConsole.h"
#include "Headers/GUIConsoleCommandParser.h"
#include "CEGUIAddons/Headers/CEGUIFormattedListBox.h"

#include "Core/Headers/ParamHandler.h"

///Maximum number of lines to display in the console window
#define CEGUI_MAX_CONSOLE_ENTRIES 64
#define CEGUI_MAX_INPUT_HISTORY 5

int GUIConsole::_instanceNumber; 
 
GUIConsole::GUIConsole() : _consoleWindow(NULL),
						   _editBox(NULL),
						   _outputWindow(NULL),
						   _namePrefix(""),
						   _init(false),
						   _inputHistoryIndex(0)
{
	_instanceNumber = 0;
	// we need a default command parser, so just create it here
   _cmdParser = New GUIConsoleCommandParser();
}

GUIConsole::~GUIConsole()
{
	_init = false;
	if (_consoleWindow){
		CEGUI::System::getSingleton().getGUISheet()->removeChildWindow(_consoleWindow);
	}
	SAFE_DELETE(_cmdParser);
}

void GUIConsole::CreateCEGUIWindow(){
	if(_init) ERROR_FN(Locale::get("ERROR_CONSOLE_DOUBLE_INIT"));
	// Get a local pointer to the CEGUI Window Manager, Purely for convenience to reduce typing
	CEGUI::WindowManager *pWindowManager = CEGUI::WindowManager::getSingletonPtr();
 
    // Now before we load anything, lets increase our instance number to ensure no conflicts.  
    _namePrefix = ++_instanceNumber + "_";
 
    // load the console window from the layout file
	std::string layoutFile = ParamHandler::getInstance().getParam<std::string>("GUI.consoleLayout");
	_consoleWindow = pWindowManager->loadWindowLayout(layoutFile,_namePrefix);
 
    if (_consoleWindow){
         // Add the window to the GUI Root Sheet
         CEGUI::System::getSingleton().getGUISheet()->addChildWindow(_consoleWindow);
		 _outputWindow = static_cast<CEGUI::Listbox*>(_consoleWindow->getChild(_namePrefix + "ConsoleRoot/ChatBox"));
		 _editBox = static_cast<CEGUI::Editbox*>(_consoleWindow->getChild(_namePrefix + "ConsoleRoot/EditBox"));
         // Now register the handlers for the events (Clicking, typing, etc)
         RegisterHandlers();
    }else{
         // Loading layout from file, failed, so output an error Message.
         CEGUI::Logger::getSingleton().logEvent("Error: Unable to load the ConsoleWindow from .layout");
		 ERROR_FN(Locale::get("ERROR_CONSOLE_LAYOUT_FILE"),layoutFile.c_str());
    }
	setVisible(false);
	_init = true;
	PRINT_FN(Locale::get("GUI_CONSOLE_CREATED"));
}

void GUIConsole::RegisterHandlers(){

	assert(_editBox != NULL);
	//We need to monitor text/command submission from the editBox
    _editBox->subscribeEvent(CEGUI::Editbox::EventTextAccepted,CEGUI::Event::Subscriber(&GUIConsole::Handle_TextSubmitted,this));
	//we also monitor any key presses to make sure we do not accidentally hide the console in the middle of a sentence
	_editBox->subscribeEvent(CEGUI::Editbox::EventKeyUp,CEGUI::Event::Subscriber(&GUIConsole::Handle_TextInput,this));
}

bool GUIConsole::Handle_TextInput(const CEGUI::EventArgs &e){
	assert(_editBox != NULL);
	const CEGUI::KeyEventArgs* keyEvent = static_cast<const CEGUI::KeyEventArgs*>(&e);
	//Just get the current text string from the editbox at each keypress. Performance isn't a issue for console commands
	if(!_inputHistory.empty()){
		if(keyEvent->scancode == CEGUI::Key::ArrowUp ){
			_inputHistoryIndex--;
			if(_inputHistoryIndex < 0) 
				_inputHistoryIndex = _inputHistory.size()-1;
			_editBox->setText(_inputHistory[_inputHistoryIndex]);
		}
		if(keyEvent->scancode == CEGUI::Key::ArrowDown){
			_inputHistoryIndex++;
			if(_inputHistoryIndex >= (I32)_inputHistory.size())
				_inputHistoryIndex = 0;
			_editBox->setText(_inputHistory[_inputHistoryIndex]);
		}
	}
	
	_inputBuffer = _editBox->getText().c_str();
	return true;
}


bool GUIConsole::Handle_TextSubmitted(const CEGUI::EventArgs &e){
	assert(_editBox != NULL);
    //We need window events, specifically
    const CEGUI::WindowEventArgs* args = static_cast<const CEGUI::WindowEventArgs*>(&e);
    // Since we have that string, lets send it to the TextParser which will handle it from here
    _cmdParser->processCommand(_inputBuffer.c_str());
    // Now that we've finished with the text, we need to ensure that we clear out the EditBox.  
	// This is what we would expect to happen after we press enter
    _editBox->setText("");
	_inputHistory.push_back(_inputBuffer);
	//Keep command history low
	if(_inputHistory.size() > CEGUI_MAX_INPUT_HISTORY){
		_inputHistory.pop_front();
	}
	_inputHistoryIndex = _inputHistory.size()-1;
	//reset the inputbuffer so we can handle console closing properly
	_inputBuffer.clear();
    return true;
}

void GUIConsole::setVisible(bool visible){
	//if it's not the first key (e.g., if the toggle key is "~", then "lorem~ipsum" should not close the window)
	if(!_inputBuffer.empty()) return;
	assert(_editBox != NULL);
	_consoleWindow->setVisible(visible);

    if(visible){
       _editBox->activate();
	}else{
	   _editBox->deactivate();
	   _editBox->setText("");
	}
	
	OutputText(visible ? "Toggling console display: ON" : "Toggling console display: OFF", CEGUI::colour(0.4f,0.4f,0.3f));
}
 
bool GUIConsole::isVisible(){
    return _consoleWindow->isVisible();
}

void GUIConsole::printText(const std::string& output,bool error){
	if(!_init){
		//If the console widget isn't loaded, create an output buffer
		_outputBuffer.push_back(std::make_pair(output,error));
		return;
	}
	//print it all at once
#ifdef _DEBUG
	U16 count = 0;
	reverse_for_each(bufferMap::value_type& iter, _outputBuffer){
		OutputText(iter.first,iter.second ? CEGUI::colour(1.0f,0.0f,0.0f) : CEGUI::colour(0.4f,0.4f,0.3f));
		if(++count > CEGUI_MAX_CONSOLE_ENTRIES) break;
	}
#else
	for_each(bufferMap::value_type& iter, _outputBuffer){
		OutputText(iter.first,iter.second ? CEGUI::colour(1.0f,0.0f,0.0f) : CEGUI::colour(0.4f,0.4f,0.3f));
	}
#endif
	_outputBuffer.clear();

	OutputText(output,error ? CEGUI::colour(1.0f,0.0f,0.0f) : CEGUI::colour(0.4f,0.4f,0.3f));
}

void GUIConsole::OutputText(const std::string& inMsg, CEGUI::colour colour){
	// Create a new List Box item that uses wordwrap. This will hold the output from the command
	// Append the response with left wordwrap alignement
	CEGUI::FormattedListboxTextItem *newItem = new CEGUI::FormattedListboxTextItem(inMsg,CEGUI::HTF_WORDWRAP_LEFT_ALIGNED);
	// Disable any parsing of the text
	newItem->setTextParsingEnabled(false);
	// Set the correct colour (e.g. red for errors)                                                                                          
	newItem->setTextColours(colour);
	// Add the new ListBoxTextItem to the ListBox
	_outputWindow->addItem(newItem); 
#ifdef _DEBUG
	// Try to not overfill the listbox.
    while(_outputWindow->getItemCount() > CEGUI_MAX_CONSOLE_ENTRIES) {
         _outputWindow->removeItem(_outputWindow->getListboxItemFromIndex(0));
    }
#endif
	// Always make sure the last item is visible (auto-scroll)
	_outputWindow->ensureItemIsVisible(newItem);
}