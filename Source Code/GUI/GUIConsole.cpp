#include <CEGUI/CEGUI.h>
#include "core.h"

#include "Headers/GUIConsole.h"
#include "Headers/GUIConsoleCommandParser.h"
#include "CEGUIAddons/Headers/CEGUIFormattedListBox.h"

#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"

#if defined(_MSC_VER)
#	pragma warning( push )
#		pragma warning( disable: 4324 ) //<structure was padded ...
#		pragma warning( disable: 4189 ) //<local variable is initialized but not referenced
#elif defined(__GNUC__)
#	pragma GCC diagnostic push
#		pragma GCC diagnostic ignored "-Wall"
#endif

#	ifndef CEGUI_DEFAULT_CONTEXT
#		define CEGUI_DEFAULT_CONTEXT CEGUI::System::getSingleton().getDefaultGUIContext()
#	endif


#if defined(_MSC_VER)
#	pragma warning( pop )
#elif defined(__GNUC__)
#	pragma GCC diagnostic pop
#endif

namespace Divide {

GUIConsole::GUIConsole() : _init(false),
                           _closing(false),
                           _editBox(nullptr),
                           _lastMsgError(false),
                           _inputHistoryIndex(0),
                           _outputWindow(nullptr),
                           _consoleWindow(nullptr),
                           _outputBuffer(_CEGUI_MAX_CONSOLE_ENTRIES)
{
    // we need a default command parser, so just create it here
    _cmdParser = New GUIConsoleCommandParser();
}

GUIConsole::~GUIConsole()
{
    _closing = true;
    setVisible(false);

    _init = false;

    if (_consoleWindow) {
        CEGUI_DEFAULT_CONTEXT.getRootWindow()->removeChild(_consoleWindow);
    }
    SAFE_DELETE(_cmdParser);

    _outputBuffer.clear();
}

void GUIConsole::CreateCEGUIWindow(){
    if (_init) {
        ERROR_FN(Locale::get("ERROR_CONSOLE_DOUBLE_INIT"));
    }
    // load the console Window from the layout file
    std::string layoutFile = ParamHandler::getInstance().getParam<std::string>("GUI.consoleLayout");
    _consoleWindow = CEGUI::WindowManager::getSingletonPtr()->loadLayoutFromFile(layoutFile);

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

    _init = true;
    PRINT_FN(Locale::get("GUI_CONSOLE_CREATED"));
}

void GUIConsole::RegisterHandlers(){
    assert(_editBox != nullptr);
    //We need to monitor text/command submission from the editBox
    _editBox->subscribeEvent(CEGUI::Editbox::EventTextAccepted,
                             CEGUI::Event::Subscriber(&GUIConsole::Handle_TextSubmitted,this));
    //we also monitor any key presses to make sure we do not accidentally hide the console in the middle of a sentence
    _editBox->subscribeEvent(CEGUI::Editbox::EventKeyUp,
                             CEGUI::Event::Subscriber(&GUIConsole::Handle_TextInput,this));
}

bool GUIConsole::Handle_TextInput(const CEGUI::EventArgs &e){
    assert(_editBox != nullptr);
    const CEGUI::KeyEventArgs* keyEvent = static_cast<const CEGUI::KeyEventArgs*>(&e);
    //Just get the current text string from the editbox at each keypress. Performance isn't a issue for console commands
    if(!_inputHistory.empty()){
        if(keyEvent->scancode == CEGUI::Key::ArrowUp ){
            _inputHistoryIndex--;
            if(_inputHistoryIndex < 0)
                _inputHistoryIndex = (I16)_inputHistory.size()-1;
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
    assert(_editBox != nullptr);
    // Since we have that string, lets send it to the TextParser which will handle it from here
    _cmdParser->processCommand(_inputBuffer.c_str());
    // Now that we've finished with the text, we need to ensure that we clear out the EditBox.
    // This is what we would expect to happen after we press enter
    _editBox->setText("");
    _inputHistory.push_back(_inputBuffer);
    //Keep command history low
    if (_inputHistory.size() > _CEGUI_MAX_CONSOLE_ENTRIES) {
        _inputHistory.pop_front();
    }
    _inputHistoryIndex = (I16)_inputHistory.size()-1;
    //reset the inputbuffer so we can handle console closing properly
    _inputBuffer.clear();
    return true;
}

void GUIConsole::setVisible(bool visible){
    if (!_init)
        CreateCEGUIWindow();
    
    //if it's not the first key (e.g., if the toggle key is "~", then "lorem~ipsum" should not close the Window)
    if(!_inputBuffer.empty())
        return;

    assert(_editBox != nullptr);
    _consoleWindow->setVisible(visible);

    if(visible){
       _editBox->activate();
    }else{
       _editBox->deactivate();
       _editBox->setText("");
    }

    printText(visible ? "Toggling console display: ON" : "Toggling console display: OFF", false);
    if (_outputWindow->getItemCount() > 0 && visible) {
        _outputWindow->ensureItemIsVisible(_outputWindow->getListboxItemFromIndex(_outputWindow->getItemCount() - 1));
    }
}

bool GUIConsole::isVisible(){
    if (!_init) {
        return false;
    }
    return _consoleWindow->isVisible();
}

void GUIConsole::printText(const char* output, bool error){
    WriteLock w_lock(_outputLock);
    _outputBuffer.push_back(std::make_pair(std::string(output), error));
}

void GUIConsole::OutputText(const char* inMsg, const bool error) {
    if (_outputWindow->getItemCount() == _CEGUI_MAX_CONSOLE_ENTRIES - 1) {
        _outputWindow->removeItem(_outputWindow->getListboxItemFromIndex(0));
    }

    CEGUI::FormattedListboxTextItem* crtItem = nullptr;
    crtItem = New CEGUI::FormattedListboxTextItem(CEGUI::String(inMsg), 
                                                  error ? CEGUI::Colour(1.0f, 0.0f, 0.0f) : 
                                                          CEGUI::Colour(0.4f, 0.4f, 0.3f), 
                                                  CEGUI::HTF_WORDWRAP_LEFT_ALIGNED);
    crtItem->setTextParsingEnabled(false);
    _outputWindow->addItem(crtItem);
    _outputWindow->ensureItemIsVisible(crtItem);
}

void GUIConsole::update(const U64 deltaTime){
    if(!_init || !Application::getInstance().isMainThread() || _closing){
        return;
    }
    if (!isVisible()) {
        return;
    }
    WriteLock w_lock(_outputLock);
    std::pair<std::string, bool> message;
    while(!_outputBuffer.empty()){
        message = _outputBuffer.front();
        if (_lastMsgError != message.second){
            _lastMsgError = message.second;
            OutputText(_lastMsg.c_str(), _lastMsgError);
            _lastMsg.clear();
        }

        _lastMsg.append(message.first.c_str());
        _lastMsg.append("\n");
        _outputBuffer.pop_front();
    }
    _outputBuffer.clear();
     
    if(!_lastMsg.empty()){
        OutputText(_lastMsg.c_str(), _lastMsgError);
        _lastMsg.clear();
    }
}

};