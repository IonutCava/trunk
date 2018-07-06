#include "Headers/GUI.h"

#include "Headers/GUIFlash.h"
#include "Headers/GUIText.h"
#include "Headers/GUIButton.h"
#include "Headers/GUIConsole.h"
#include "Headers/GUIMessageBox.h"
#include "GUIEditor/Headers/GUIEditor.h"

#include "Scenes/Headers/Scene.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/ApplicationTimer.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Input/Headers/InputInterface.h"

#ifndef CEGUI_STATIC
#define CEGUI_STATIC
#include <CEGUI/CEGUI.h>
#endif //CEGUI_STATIC

#include <stdarg.h>

namespace Divide {

GUI::GUI()
    : _init(false),
      _rootSheet(nullptr),
      _defaultMsgBox(nullptr),
      _console(MemoryManager_NEW GUIConsole())
{
    // 500ms
    _textRenderInterval = Time::MillisecondsToMicroseconds(10);
    _ceguiInput.setInitialDelay(0.500f);
    GUIEditor::createInstance();
}

GUI::~GUI() {
    Console::printfn(Locale::get("STOP_GUI"));
    GUIEditor::destroyInstance();
    MemoryManager::DELETE(_console);
    RemoveResource(_guiShader);
    MemoryManager::DELETE_HASHMAP(_guiStack);
    _defaultMsgBox = nullptr;
}

void GUI::draw() const {
    if (!_init) {
        return;
    }

    _guiShader->bind();

    GFXDevice& gfx = GFX_DEVICE;
    for (const guiMap::value_type& guiStackIterator : _guiStack) {
        GUIElement& element = *guiStackIterator.second;
        // Skip hidden elements
        if (element.isVisible()) {
            element.draw();
             // Update internal timer
            element.lastDrawTimer(Time::ElapsedMicroseconds());
        }
    }
     
    
    const OIS::MouseState& mouseState =
        Input::InputInterface::getInstance().getMouse().getMouseState();

    setCursorPosition(static_cast<U16>(mouseState.X.abs),
                      static_cast<U16>(mouseState.Y.abs));
}

void GUI::update(const U64 deltaTime) {
    if (!_init) {
        return;
    }

    _ceguiInput.update(deltaTime);
    CEGUI::System::getSingleton().injectTimePulse(
        Time::MicrosecondsToSeconds<F32>(deltaTime));
    CEGUI::System::getSingleton().getDefaultGUIContext().injectTimePulse(
        Time::MicrosecondsToSeconds<F32>(deltaTime));

    if (_console) {
        _console->update(deltaTime);
    }

    GUIEditor::getInstance().update(deltaTime);
}

bool GUI::init(const vec2<U16>& resolution) {
    if (_init) {
        Console::d_errorfn(Locale::get("ERROR_GUI_DOUBLE_INIT"));
        return false;
    }

#ifdef _DEBUG
    CEGUI::Logger::getSingleton().setLoggingLevel(CEGUI::Informative);
#endif
    CEGUI::DefaultResourceProvider* rp = nullptr;
    rp = static_cast<CEGUI::DefaultResourceProvider*>(
        CEGUI::System::getSingleton().getResourceProvider());
    CEGUI::String CEGUIInstallSharePath(
        ParamHandler::getInstance().getParam<stringImpl>("assetsLocation"));
    CEGUIInstallSharePath += "/GUI/";
    rp->setResourceGroupDirectory("schemes",
                                  CEGUIInstallSharePath + "schemes/");
    rp->setResourceGroupDirectory("imagesets",
                                  CEGUIInstallSharePath + "imagesets/");
    rp->setResourceGroupDirectory("fonts", CEGUIInstallSharePath + "fonts/");
    rp->setResourceGroupDirectory("layouts",
                                  CEGUIInstallSharePath + "layouts/");
    rp->setResourceGroupDirectory("looknfeels",
                                  CEGUIInstallSharePath + "looknfeel/");
    rp->setResourceGroupDirectory("lua_scripts",
                                  CEGUIInstallSharePath + "lua_scripts/");
    rp->setResourceGroupDirectory("schemas",
                                  CEGUIInstallSharePath + "xml_schemas/");
    rp->setResourceGroupDirectory("animations",
                                  CEGUIInstallSharePath + "animations/");

    // set the default resource groups to be used
    CEGUI::ImageManager::setImagesetDefaultResourceGroup("imagesets");
    CEGUI::Font::setDefaultResourceGroup("fonts");
    CEGUI::Scheme::setDefaultResourceGroup("schemes");
    CEGUI::WidgetLookManager::setDefaultResourceGroup("looknfeels");
    CEGUI::WindowManager::setDefaultResourceGroup("layouts");
    CEGUI::ScriptModule::setDefaultResourceGroup("lua_scripts");
    // setup default group for validation schemas
    CEGUI::XMLParser* parser = CEGUI::System::getSingleton().getXMLParser();
    if (parser->isPropertyPresent("SchemaDefaultResourceGroup")) {
        parser->setProperty("SchemaDefaultResourceGroup", "schemas");
    }
    CEGUI::FontManager::getSingleton().createFromFile("DejaVuSans-10.font");
    CEGUI::FontManager::getSingleton().createFromFile("DejaVuSans-12.font");
    CEGUI::FontManager::getSingleton().createFromFile(
        "DejaVuSans-10-NoScale.font");
    CEGUI::FontManager::getSingleton().createFromFile(
        "DejaVuSans-12-NoScale.font");
    _defaultGUIScheme =
        ParamHandler::getInstance().getParam<stringImpl>("GUI.defaultScheme");
    CEGUI::SchemeManager::getSingleton().createFromFile(_defaultGUIScheme + ".scheme");

    _rootSheet = CEGUI::WindowManager::getSingleton().createWindow(
        "DefaultWindow", "root_window");
    _rootSheet->setMousePassThroughEnabled(true);
    CEGUI_DEFAULT_CTX.setRootWindow(_rootSheet);
    CEGUI_DEFAULT_CTX.setDefaultTooltipType(_defaultGUIScheme + "/Tooltip");
    _rootSheet->setPixelAligned(false);
    assert(_console);
    //_console->CreateCEGUIWindow();
    GUIEditor::getInstance().init();

    ResourceDescriptor immediateModeShader("ImmediateModeEmulation.GUI");
    immediateModeShader.setThreadedLoading(false);
    _guiShader = CreateResource<ShaderProgram>(immediateModeShader);
    _guiShader->Uniform("dvd_WorldMatrix", mat4<F32>());
    GFX_DEVICE.add2DRenderFunction(DELEGATE_BIND(&GUI::draw, this),
                                   std::numeric_limits<U32>::max() - 1);
    const OIS::MouseState& mouseState =
        Input::InputInterface::getInstance().getMouse().getMouseState();

    setCursorPosition(static_cast<U16>(mouseState.X.abs),
                      static_cast<U16>(mouseState.Y.abs));

    _defaultMsgBox = addMsgBox("AssertMsgBox", "Assertion failure",
                               "Assertion failed with message: ");
    _init = true;
    return true;
}

void GUI::selectionChangeCallback(Scene* const activeScene) {
    GUIEditor::getInstance().Handle_ChangeSelection(
        activeScene->getCurrentSelection());
}

void GUI::setCursorPosition(U16 x, U16 y) const {
    CEGUI_DEFAULT_CTX.injectMousePosition(x, y);
}

bool GUI::onKeyDown(const Input::KeyEvent& key) {
    if (!_init) {
        return true;
    }

    if (_ceguiInput.onKeyDown(key)) {
        return (!_console->isVisible() &&
                !GUIEditor::getInstance().isVisible());
    }

    return false;
}

bool GUI::onKeyUp(const Input::KeyEvent& key) {
    if (!_init) {
        return true;
    }

    if (key._key == Input::KeyCode::KC_GRAVE) {
        _console->setVisible(!_console->isVisible());
    }

#ifdef _DEBUG
    if (key._key == Input::KeyCode::KC_F11) {
        GUIEditor::getInstance().setVisible(
            !GUIEditor::getInstance().isVisible());
    }
#endif

    if (_ceguiInput.onKeyUp(key)) {
        return (!_console->isVisible() &&
                !GUIEditor::getInstance().isVisible());
    }

    return false;
}

bool GUI::mouseMoved(const Input::MouseEvent& arg) {
    if (!_init) {
        return true;
    }

    GUIEvent event;
    event.mousePoint.x = to_float(arg.state.X.abs);
    event.mousePoint.y = to_float(arg.state.Y.abs);

    for (guiMap::value_type& guiStackIterator : _guiStack) {
        guiStackIterator.second->mouseMoved(event);
    }

    return _ceguiInput.mouseMoved(arg);
}

bool GUI::mouseButtonPressed(const Input::MouseEvent& arg,
                             Input::MouseButton button) {
    if (!_init) {
        return true;
    }

    if (_ceguiInput.mouseButtonPressed(arg, button)) {
        if (button == Input::MouseButton::MB_Left) {
            GUIEvent event;
            event.mouseClickCount = 0;
            for (guiMap::value_type& guiStackIterator : _guiStack) {
                guiStackIterator.second->onMouseDown(event);
            }
        }
    }

    return !_console->isVisible() &&
           !GUIEditor::getInstance().wasControlClick();
}

bool GUI::mouseButtonReleased(const Input::MouseEvent& arg,
                              Input::MouseButton button) {
    if (!_init) {
        return true;
    }

    if (_ceguiInput.mouseButtonReleased(arg, button)) {
        if (button == Input::MouseButton::MB_Left) {
            GUIEvent event;
            event.mouseClickCount = 1;
            for (guiMap::value_type& guiStackIterator : _guiStack) {
                guiStackIterator.second->onMouseUp(event);
            }
        }
    }

    return !_console->isVisible() &&
           !GUIEditor::getInstance().wasControlClick();
}

bool GUI::joystickAxisMoved(const Input::JoystickEvent& arg, I8 axis) {
    return _ceguiInput.joystickAxisMoved(arg, axis);
}

bool GUI::joystickPovMoved(const Input::JoystickEvent& arg, I8 pov) {
    return _ceguiInput.joystickPovMoved(arg, pov);
}

bool GUI::joystickButtonPressed(const Input::JoystickEvent& arg,
                                Input::JoystickButton button) {
    return _ceguiInput.joystickButtonPressed(arg, button);
}

bool GUI::joystickButtonReleased(const Input::JoystickEvent& arg,
                                 Input::JoystickButton button) {
    return _ceguiInput.joystickButtonReleased(arg, button);
}

bool GUI::joystickSliderMoved(const Input::JoystickEvent& arg, I8 index) {
    return _ceguiInput.joystickSliderMoved(arg, index);
}

bool GUI::joystickVector3DMoved(const Input::JoystickEvent& arg, I8 index) {
    return _ceguiInput.joystickVector3DMoved(arg, index);
}

GUIButton* GUI::addButton(const stringImpl& ID,
                          const stringImpl& text,
                          const vec2<I32>& position,
                          const vec2<U32>& dimensions,
                          const vec3<F32>& color,
                          ButtonCallback callback,
                          const stringImpl& rootSheetID) {

    const vec2<U16>& displaySize
        = Application::getInstance().getWindowManager().getWindowDimensions();

    vec2<F32> relOffset((position.x * 100.0f) / displaySize.x,
                        (position.y * 100.0f) / displaySize.y);

    vec2<F32> relDim((dimensions.x * 100.0f) / displaySize.x,
                     (dimensions.y * 100.0f) / displaySize.y);

    CEGUI::Window* parent = nullptr;
    if (!rootSheetID.empty()) {
        parent = CEGUI_DEFAULT_CTX.getRootWindow()->getChild(
            rootSheetID.c_str());
    }
    if (!parent) {
        parent = _rootSheet;
    }
    GUIButton* btn =
        MemoryManager_NEW GUIButton(ID, text, _defaultGUIScheme, relOffset,
                                    relDim, color, parent, callback);
    guiMap::iterator it = _guiStack.find(ID);
    if (it != std::end(_guiStack)) {
        MemoryManager::SAFE_UPDATE(it->second, btn);
    } else {
        hashAlg::insert(_guiStack, std::make_pair(ID, static_cast<GUIElement*>(btn)));
    }

    return btn;
}

GUIMessageBox* GUI::addMsgBox(const stringImpl& id, const stringImpl& title,
                              const stringImpl& message,
                              const vec2<I32>& offsetFromCentre) {
    GUIMessageBox* box = MemoryManager_NEW GUIMessageBox(
        id, title, message, offsetFromCentre, _rootSheet);
    guiMap::iterator it = _guiStack.find(id);
    if (it != std::end(_guiStack)) {
        MemoryManager::SAFE_UPDATE(it->second, box);
    } else {
        hashAlg::insert(_guiStack, std::make_pair(id, static_cast<GUIElement*>(box)));
    }

    return box;
}

GUIText* GUI::addText(const stringImpl& id, const vec2<I32>& position,
                      const stringImpl& font, const vec3<F32>& color,
                      const char* format, ...) {

    const vec2<U16>& displaySize
        = Application::getInstance().getWindowManager().getWindowDimensions();

    vec2<F32> relOffset((position.x * 100.0f) / displaySize.x,
                        ((displaySize.y - position.y) * 100.0f) / displaySize.y);

    va_list args;
    stringImpl fmt_text;

    va_start(args, format);
    I32 len = _vscprintf(format, args) + 1;
    char* text = MemoryManager_NEW char[len];
    vsprintf(text, format, args);
    fmt_text.append(text);
    MemoryManager::DELETE_ARRAY(text);
    va_end(args);

    GUIText* t = MemoryManager_NEW GUIText(id, fmt_text, relOffset, font, color, _rootSheet);
    guiMap::iterator it = _guiStack.find(id);
    if (it != std::end(_guiStack)) {
        MemoryManager::SAFE_UPDATE(it->second, t);
    } else {
        hashAlg::insert(_guiStack, std::make_pair(id, static_cast<GUIElement*>(t)));
    }

    fmt_text.empty();

    return t;
}

GUIFlash* GUI::addFlash(const stringImpl& id, stringImpl movie,
                        const vec2<U32>& position, const vec2<U32>& extent) {
    GUIFlash* flash = MemoryManager_NEW GUIFlash(_rootSheet);
    guiMap::iterator it = _guiStack.find(id);
    if (it != std::end(_guiStack)) {
        MemoryManager::SAFE_UPDATE(it->second, flash);
    } else {
        hashAlg::insert(_guiStack, std::make_pair(id, static_cast<GUIElement*>(flash)));
    }
    return flash;
}

GUIText* GUI::modifyText(const stringImpl& id, const char* format, ...) {
    if (_guiStack.find(id) == std::end(_guiStack)) {
        return nullptr;
    }
    va_list args;
    stringImpl fmt_text;

    va_start(args, format);
    I32 len = _vscprintf(format, args) + 1;
    char* text = MemoryManager_NEW char[len];
    vsprintf(text, format, args);
    fmt_text.append(text);
    MemoryManager::DELETE_ARRAY(text);
    va_end(args);

    GUIElement* element = _guiStack[id];
    assert(element->getType() == GUIType::GUI_TEXT);

    GUIText* textElement = dynamic_cast<GUIText*>(element);
    assert(textElement != nullptr);

    textElement->text(fmt_text);
    fmt_text.empty();
    return textElement;
}
};
