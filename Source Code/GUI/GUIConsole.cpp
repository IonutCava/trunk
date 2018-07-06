#include "Headers/GUIConsole.h"
#include "Headers/GUIConsoleCommandParser.h"
#include "CEGUIAddons/Headers/CEGUIFormattedListBox.h"

#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"

#ifndef CEGUI_STATIC
#define CEGUI_STATIC
#include <CEGUI/CEGUI.h>
#endif //CEGUI_STATIC

#ifndef CEGUI_DEFAULT_CTX
#define CEGUI_DEFAULT_CTX \
    CEGUI::System::getSingleton().getDefaultGUIContext()
#endif

namespace Divide {

GUIConsole::GUIConsole()
    : _init(false),
      _closing(false),
      _editBox(nullptr),
      _lastMsgError(false),
      _inputHistoryIndex(0),
      _outputWindow(nullptr),
      _consoleWindow(nullptr),
      _outputBuffer(_CEGUI_MAX_CONSOLE_ENTRIES) {
    // we need a default command parser, so just create it here
    _cmdParser = MemoryManager_NEW GUIConsoleCommandParser();
}

GUIConsole::~GUIConsole() {
    _closing = true;

    if (_consoleWindow) {
        setVisible(false);
        _init = false;
        CEGUI_DEFAULT_CTX.getRootWindow()->removeChild(_consoleWindow);
        MemoryManager::DELETE(_consoleWindow);
    }
    MemoryManager::DELETE(_cmdParser);

    _outputBuffer.clear();
}

void GUIConsole::CreateCEGUIWindow() {
    if (_init) {
        Console::errorfn(Locale::get(_ID("ERROR_CONSOLE_DOUBLE_INIT")));
    }
    // load the console Window from the layout file
    const stringImpl& layoutFile =
        ParamHandler::instance().getParam<stringImpl>(_ID("GUI.consoleLayout"));
    _consoleWindow =
        CEGUI::WindowManager::getSingletonPtr()->loadLayoutFromFile(layoutFile.c_str());

    if (_consoleWindow) {
        // Add the Window to the GUI Root Sheet
        CEGUI_DEFAULT_CTX.getRootWindow()->addChild(_consoleWindow);
        _outputWindow =
            static_cast<CEGUI::Listbox*>(_consoleWindow->getChild("ChatBox"));
        _editBox =
            static_cast<CEGUI::Editbox*>(_consoleWindow->getChild("EditBox"));
        // Now register the handlers for the events (Clicking, typing, etc)
        RegisterHandlers();
    } else {
        // Loading layout from file, failed, so output an error Message.
        CEGUI::Logger::getSingleton().logEvent(
            "Error: Unable to load the ConsoleWindow from .layout");
        Console::errorfn(Locale::get(_ID("ERROR_CONSOLE_LAYOUT_FILE")),
                         layoutFile.c_str());
    }

    _init = true;
    Console::printfn(Locale::get(_ID("GUI_CONSOLE_CREATED")));
}

void GUIConsole::RegisterHandlers() {
    assert(_editBox != nullptr);
    // We need to monitor text/command submission from the editBox
    _editBox->subscribeEvent(
        CEGUI::Editbox::EventTextAccepted,
        CEGUI::Event::Subscriber(&GUIConsole::Handle_TextSubmitted, this));
    // we also monitor any key presses to make sure we do not accidentally hide
    // the console in the middle of a sentence
    _editBox->subscribeEvent(
        CEGUI::Editbox::EventKeyUp,
        CEGUI::Event::Subscriber(&GUIConsole::Handle_TextInput, this));
}

bool GUIConsole::Handle_TextInput(const CEGUI::EventArgs& e) {
    assert(_editBox != nullptr);
    const CEGUI::KeyEventArgs* keyEvent =
        static_cast<const CEGUI::KeyEventArgs*>(&e);
    // Just get the current text string from the editbox at each keypress.
    // Performance isn't a issue for console commands
    if (!_inputHistory.empty()) {
        if (keyEvent->scancode == CEGUI::Key::ArrowUp) {
            _inputHistoryIndex--;
            if (_inputHistoryIndex < 0)
                _inputHistoryIndex = (I16)_inputHistory.size() - 1;
            _editBox->setText(_inputHistory[_inputHistoryIndex]);
        }
        if (keyEvent->scancode == CEGUI::Key::ArrowDown) {
            _inputHistoryIndex++;
            if (_inputHistoryIndex >= (I32)_inputHistory.size())
                _inputHistoryIndex = 0;
            _editBox->setText(_inputHistory[_inputHistoryIndex]);
        }
    }

    _inputBuffer = _editBox->getText();
    return true;
}

bool GUIConsole::Handle_TextSubmitted(const CEGUI::EventArgs& e) {
    assert(_editBox != nullptr);
    // Since we have that string, lets send it to the TextParser which will
    // handle it from here
    _cmdParser->processCommand(_inputBuffer.c_str());
    // Now that we've finished with the text, we need to ensure that we clear
    // out the EditBox.
    // This is what we would expect to happen after we press enter
    _editBox->setText("");
    _inputHistory.push_back(_inputBuffer);
    // Keep command history low
    if (_inputHistory.size() > _CEGUI_MAX_CONSOLE_ENTRIES) {
        _inputHistory.pop_front();
    }
    _inputHistoryIndex = (I16)_inputHistory.size() - 1;
    // reset the inputbuffer so we can handle console closing properly
    _inputBuffer.clear();
    return true;
}

void GUIConsole::setVisible(bool visible) {
    if (!_init) {
        CreateCEGUIWindow();
    }
    // if it's not the first key (e.g., if the toggle key is "~", then
    // "lorem~ipsum" should not close the Window)
    if (!_inputBuffer.empty()) {
        return;
    }
    assert(_editBox != nullptr);
    _consoleWindow->setVisible(visible);

    if (visible) {
        _editBox->activate();
    } else {
        _editBox->deactivate();
        _editBox->setText("");
    }

    printText(visible ? "Toggling console display: ON"
                      : "Toggling console display: OFF",
              false);
    if (_outputWindow->getItemCount() > 0 && visible) {
        _outputWindow->ensureItemIsVisible(
            _outputWindow->getListboxItemFromIndex(
                _outputWindow->getItemCount() - 1));
    }
}

bool GUIConsole::isVisible() {
    if (!_init) {
        return false;
    }
    return _consoleWindow->isVisible();
}

void GUIConsole::printText(const char* output, bool error) {
    if (!_init) {
        _outputBuffer.push_back(std::make_pair(output, error));
    } else {
        WriteLock w_lock(_outputLock);
        _outputBuffer.push_back(std::make_pair(output, error));
    }
}

void GUIConsole::OutputText(const char* inMsg, const bool error) {
    if (_outputWindow->getItemCount() == _CEGUI_MAX_CONSOLE_ENTRIES - 1) {
        _outputWindow->removeItem(_outputWindow->getListboxItemFromIndex(0));
    }

    CEGUI::FormattedListboxTextItem* crtItem = nullptr;
    crtItem = new CEGUI::FormattedListboxTextItem(
        CEGUI::String(inMsg), error ? CEGUI::Colour(1.0f, 0.0f, 0.0f)
                                    : CEGUI::Colour(0.4f, 0.4f, 0.3f),
        CEGUI::HTF_WORDWRAP_LEFT_ALIGNED);
    crtItem->setTextParsingEnabled(false);
    _outputWindow->addItem(crtItem);
    _outputWindow->ensureItemIsVisible(crtItem);
}

void GUIConsole::update(const U64 deltaTime) {
    if (!_init || !Application::instance().isMainThread() || _closing) {
        return;
    }
    if (!isVisible()) {
        return;
    }
    WriteLock w_lock(_outputLock);
    while (!_outputBuffer.empty()) {
        const std::pair<stringImpl, bool>& message = _outputBuffer.front();
        bool error = message.second;
        if (_lastMsgError != error) {
            _lastMsgError = error;
            OutputText(_lastMsg.c_str(), _lastMsgError);
            _lastMsg.clear();
        }
        if (error) {
            _lastMsg.append("Error: ");
        }
        _lastMsg.append(message.first.c_str());
        _lastMsg.append("\n");
        _outputBuffer.pop_front();
    }
    _outputBuffer.clear();

    if (!_lastMsg.empty()) {
        OutputText(_lastMsg.c_str(), _lastMsgError);
        _lastMsg.clear();
    }
}
};