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

namespace {
#	ifndef CEGUI_DEFAULT_CONTEXT
#		define CEGUI_DEFAULT_CONTEXT CEGUI::System::getSingleton().getDefaultGUIContext()
#	endif
     
    static const I32 messageQueueTimeoutInSec = 3;
    static vectorImpl<CEGUI::FormattedListboxTextItem* > _newItem;
}

#if defined(_MSC_VER)
#	pragma warning( pop )
#elif defined(__GNUC__)
#	pragma GCC diagnostic pop
#endif

U64 GUIConsole::_totalTime = 0ULL;
I32 GUIConsole::_currentItem = 0;
boost::lockfree::queue<MessageStruct*, boost::lockfree::capacity<GUIConsole::_messageQueueCapacity> >  GUIConsole::_outputBuffer;
vectorImpl<std::pair<std::string, bool > > GUIConsole::_outputTempBuffer;

GUIConsole::GUIConsole() : _consoleWindow(nullptr),
                           _editBox(nullptr),
                           _outputWindow(nullptr),
                           _init(false),
                           _closing(false),
                           _inputHistoryIndex(0)
{
    // we need a default command parser, so just create it here
    _cmdParser = New GUIConsoleCommandParser();
    _outputTempBuffer.reserve(1024);

    for(U16 i = 0; i < _CEGUI_MAX_CONSOLE_ENTRIES; ++i){
        _newItem.push_back(New CEGUI::FormattedListboxTextItem("",CEGUI::HTF_WORDWRAP_LEFT_ALIGNED));
         // Disable any parsing of the text
        _newItem[i]->setTextParsingEnabled(false);
        _newItem[i]->setAutoDeleted(false);
    }
    
}

GUIConsole::~GUIConsole()
{
    _closing = true;
    setVisible(false);

    _init = false;

    _outputTempBuffer.clear();

    if (_consoleWindow)
        CEGUI_DEFAULT_CONTEXT.getRootWindow()->removeChild(_consoleWindow);

    SAFE_DELETE(_cmdParser);
    for(U16 i = 0; i < _CEGUI_MAX_CONSOLE_ENTRIES; ++i){
        _outputWindow->removeItem(_newItem[i]);
        SAFE_DELETE(_newItem[i]);
    }
    _newItem.clear();

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
    if(_inputHistory.size() > _CEGUI_MAX_CONSOLE_ENTRIES)
        _inputHistory.pop_front();

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

    OutputText(visible ? "Toggling console display: ON" : "Toggling console display: OFF", false);
}

bool GUIConsole::isVisible(){
    if (!_init)
        return false;

    return _consoleWindow->isVisible();
}

void GUIConsole::printText(const char* output, bool error){

    if (!_init && !_closing){
        std::string outString(output);
        assert(!outString.empty());
        _outputTempBuffer.push_back(std::make_pair(outString, error));
        return;
    }

    PushText(New MessageStruct(output, error));
 }

void GUIConsole::PushText(MessageStruct* msg){
    U64 startTimer = GETUSTIME(true);
    while (!_outputBuffer.push(msg)){
        if (getUsToSec(GETUSTIME(true) - startTimer) > messageQueueTimeoutInSec){
            break;
        }
    }
}

void GUIConsole::OutputText(const char* inMsg, const bool error){
    // Create a new List Box item that uses wordwrap. This will hold the output from the command
    // Append the response with left wordwrap alignment
    //CEGUI::FormattedListboxTextItem *newItem = New CEGUI::FormattedListboxTextItem(inMsg,CEGUI::HTF_WORDWRAP_LEFT_ALIGNED);
    _newItem[_currentItem]->setText(inMsg);
    // Set the correct color (e.g. red for errors)
    _newItem[_currentItem]->setTextColours(error ? CEGUI::Colour(1.0f, 0.0f, 0.0f) : CEGUI::Colour(0.4f, 0.4f, 0.3f));
    // Add the new ListBoxTextItem to the ListBox
    _outputWindow->removeItem(_newItem[_currentItem]);
    _outputWindow->addItem(_newItem[_currentItem]);
    // Always make sure the last item is visible (auto-scroll)
    _outputWindow->ensureItemIsVisible(_newItem[_currentItem]);

    _currentItem = ++_currentItem % _CEGUI_MAX_CONSOLE_ENTRIES;
}

void GUIConsole::update(const U64 deltaTime){
    _totalTime += deltaTime;
    if(!_init || !Application::getInstance().isMainThread() || _closing){
        return;
    }

    static bool lastMsgError = false;
    static std::string lastMsg;

    for (std::pair<std::string, bool> message : _outputTempBuffer){
        if (lastMsgError != message.second){
            lastMsgError = message.second;
            assert(!message.first.empty());
            OutputText(message.first.c_str(), lastMsgError);
            lastMsg.clear();
        }

        lastMsg.append(message.first.c_str());
        lastMsg.append("\n");
    }
    _outputTempBuffer.clear();

    MessageStruct* outMsg = nullptr;
    while(_outputBuffer.pop(outMsg)){
        if(!outMsg) continue;
            
        if(lastMsgError != outMsg->error()){
            lastMsgError = outMsg->error();
            OutputText(lastMsg.c_str(), lastMsgError);
            lastMsg.clear();
        }

        lastMsg.append(outMsg->msg());
        lastMsg.append("\n");
        SAFE_DELETE(outMsg);
    }

    if(!lastMsg.empty()){
        OutputText(lastMsg.c_str(), lastMsgError);
        lastMsg.clear();
    }
}