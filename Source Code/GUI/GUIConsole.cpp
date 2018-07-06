#include <CEGUI/CEGUI.h>
#include "core.h"
#include "Headers/GUIConsole.h"
#include "Headers/GUIConsoleCommandParser.h"
#include "CEGUIAddons/Headers/CEGUIFormattedListBox.h"

#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include <boost/lockfree/queue.hpp>

namespace {
#	ifndef CEGUI_DEFAULT_CONTEXT
#		define CEGUI_DEFAULT_CONTEXT CEGUI::System::getSingleton().getDefaultGUIContext()
#	endif

    class MessageStruct {
    public:
        MessageStruct(const char* msg, bool error) : _error(error)
        {
            _msg = _strdup(msg);
        }
        ~MessageStruct()
        {
            SAFE_DELETE(_msg);
        }

        const char* msg()   const {return _msg;}
              bool  error() const {return _error;}
    private:
        char* _msg;
        bool  _error;
    };

    /// Used to queue output text to be displayed when '_init' becomes true
    static boost::lockfree::queue<MessageStruct*, boost::lockfree::capacity<256> >  _outputBuffer;
};

GUIConsole::GUIConsole() : _consoleWindow(NULL),
                           _editBox(NULL),
                           _outputWindow(NULL),
                           _init(false),
                           _inputHistoryIndex(0)
{
    // we need a default command parser, so just create it here
   _cmdParser = New GUIConsoleCommandParser();
}

GUIConsole::~GUIConsole()
{
    _init = false;
    setVisible(false);
    if (_consoleWindow)
        CEGUI_DEFAULT_CONTEXT.getRootWindow()->removeChild(_consoleWindow);

    SAFE_DELETE(_cmdParser);
}

void GUIConsole::CreateCEGUIWindow(){
    if(_init)
        ERROR_FN(Locale::get("ERROR_CONSOLE_DOUBLE_INIT"));

    // Get a local pointer to the CEGUI Window Manager, Purely for convenience to reduce typing
    CEGUI::WindowManager *pWindowManager = CEGUI::WindowManager::getSingletonPtr();

    // load the console Window from the layout file
    std::string layoutFile = ParamHandler::getInstance().getParam<std::string>("GUI.consoleLayout");
    _consoleWindow = pWindowManager->loadLayoutFromFile(layoutFile);

    if (_consoleWindow){
         // Add the Window to the GUI Root Sheet
         CEGUI_DEFAULT_CONTEXT.getRootWindow()->addChild(_consoleWindow);
         _outputWindow = static_cast<CEGUI::Listbox*>(_consoleWindow->getChild("ChatBox"));
         _editBox = static_cast<CEGUI::Editbox*>(_consoleWindow->getChild("EditBox"));
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
    _editBox->subscribeEvent(CEGUI::Editbox::EventTextAccepted,
                             CEGUI::Event::Subscriber(&GUIConsole::Handle_TextSubmitted,this));
    //we also monitor any key presses to make sure we do not accidentally hide the console in the middle of a sentence
    _editBox->subscribeEvent(CEGUI::Editbox::EventKeyUp,
                             CEGUI::Event::Subscriber(&GUIConsole::Handle_TextInput,this));
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
    // Since we have that string, lets send it to the TextParser which will handle it from here
    _cmdParser->processCommand(_inputBuffer.c_str());
    // Now that we've finished with the text, we need to ensure that we clear out the EditBox.
    // This is what we would expect to happen after we press enter
    _editBox->setText("");
    _inputHistory.push_back(_inputBuffer);
    //Keep command history low
    if(_inputHistory.size() > _CEGUI_MAX_CONSOLE_ENTRIES)
        _inputHistory.pop_front();

    _inputHistoryIndex = _inputHistory.size()-1;
    //reset the inputbuffer so we can handle console closing properly
    _inputBuffer.clear();
    return true;
}

void GUIConsole::setVisible(bool visible){
    //if it's not the first key (e.g., if the toggle key is "~", then "lorem~ipsum" should not close the Window)
    if(!_inputBuffer.empty())
        return;

    assert(_editBox != NULL);
    _consoleWindow->setVisible(visible);

    if(visible){
       _editBox->activate();
    }else{
       _editBox->deactivate();
       _editBox->setText("");
    }

    OutputText(visible ? "Toggling console display: ON" : "Toggling console display: OFF", CEGUI::Colour(0.4f,0.4f,0.3f));
}

bool GUIConsole::isVisible(){
    return _consoleWindow->isVisible();
}

void GUIConsole::printText(const char* output, bool error){
    if(!_init || !Application::getInstance().isMainThread()){
        //If the console Window isn't loaded, create an output buffer
        MessageStruct* msg = New MessageStruct(output, error);
        while(!_outputBuffer.push(msg));
        return;
    }

   //print it all at once
    MessageStruct* outMsg = NULL;
    while(_outputBuffer.pop(outMsg)){
        OutputText(outMsg->msg(), outMsg->error());
        SAFE_DELETE(outMsg);
    }

    OutputText(output, error);
 }

void GUIConsole::OutputText(const char* inMsg, const bool error){
    CEGUI::Colour color = error ? CEGUI::Colour(1.0f,0.0f,0.0f) : CEGUI::Colour(0.4f,0.4f,0.3f);
    // Create a new List Box item that uses wordwrap. This will hold the output from the command
    // Append the response with left wordwrap alignement
    CEGUI::FormattedListboxTextItem *newItem = New CEGUI::FormattedListboxTextItem(inMsg,CEGUI::HTF_WORDWRAP_LEFT_ALIGNED);
    // Disable any parsing of the text
    newItem->setTextParsingEnabled(false);
    // Set the correct color (e.g. red for errors)
    newItem->setTextColours(color);
    // Add the new ListBoxTextItem to the ListBox
    _outputWindow->addItem(newItem);
    // Always make sure the last item is visible (auto-scroll)
    _outputWindow->ensureItemIsVisible(newItem);
       // Try to not overfill the listbox but make room for the new item
    if(_outputWindow->getItemCount() > _CEGUI_MAX_CONSOLE_ENTRIES)
         _outputWindow->removeItem(_outputWindow->getListboxItemFromIndex(0));
}