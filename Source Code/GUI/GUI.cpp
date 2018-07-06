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
#include "Core/Debugging/Headers/DebugInterface.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Input/Headers/InputInterface.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

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
      _enableCEGUIRendering(false),
      _debugVarCacheCount(0),
      _console(MemoryManager_NEW GUIConsole())
{
    // 500ms
    _textRenderInterval = Time::MillisecondsToMicroseconds(10);
    _ceguiInput.setInitialDelay(0.500f);
    GUIEditor::createInstance();
}

GUI::~GUI()
{
    Console::printfn(Locale::get(_ID("STOP_GUI")));
    GUIEditor::destroyInstance();
    MemoryManager::DELETE(_console);
    RemoveResource(_guiShader);
    MemoryManager::DELETE_HASHMAP(_guiStack);
    _defaultMsgBox = nullptr;

    // Close CEGUI
    try {
        CEGUI::System::destroy();
    }
    catch (...) {
        Console::d_errorfn(Locale::get(_ID("ERROR_CEGUI_DESTROY")));
    }
}

void GUI::draw() const {
    if (!_init) {
        return;
    }

    _guiShader->bind();

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

    setCursorPosition(mouseState.X.abs, mouseState.Y.abs);

    // CEGUI handles its own states, so render it after we clear our states but
    // before we swap buffers
    if (_enableCEGUIRendering) {
        CEGUI::System::getSingleton().renderAllGUIContexts();
    }
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

    const DebugInterface& debugInterface = DebugInterface::getInstance();
    U32 debugVarEntries = to_uint(debugInterface.debugVarCount());
    if (_debugVarCacheCount != debugVarEntries) {

        Attorney::DebugInterfaceGUI::lockVars(false);
        Attorney::DebugInterfaceGUI::lockGroups(false);

        const hashMapImpl<I64, DebugInterface::DebugGroup>& groups
            = Attorney::DebugInterfaceGUI::getDebugGroups();

        const hashMapImpl<I64, DebugInterface::DebugVar>& vars 
            = Attorney::DebugInterfaceGUI::getDebugVariables();

        // Add default group (GUID == -1) entries and root entries only.
        // Child entries should be expanded on-click
        // SLOOOOOOOOOOW - but it shouldn't change all that often
        _debugDisplayEntries.reserve(groups.size() + 1);
        for (hashMapImpl<I64, DebugInterface::DebugVar>::value_type it1 : vars) {
            if (it1.second._group == -1) {
                _debugDisplayEntries.push_back(std::make_pair(-1, it1.first));
            } else {
                for (hashMapImpl<I64, DebugInterface::DebugGroup>::value_type it2 : groups) {
                    if (it1.second._group == it2.first && it2.second._parentGroup == -1) {
                        _debugDisplayEntries.push_back(std::make_pair(it2.first, it1.first));
                    }
                }
            }
        }

        _debugVarCacheCount = debugVarEntries;

        for (std::pair<I64, I64>& entry : _debugDisplayEntries) {
            ACKNOWLEDGE_UNUSED(entry);
            //I64 groupID = entry.first;
            //I64 varID = entry.second;
            // Add a clickable text field for each
        }

        Attorney::DebugInterfaceGUI::unlockGroups();
        Attorney::DebugInterfaceGUI::unlockVars();
    }
}

bool GUI::init(const vec2<U16>& renderResolution) {
    if (_init) {
        Console::d_errorfn(Locale::get(_ID("ERROR_GUI_DOUBLE_INIT")));
        return false;
    }
    _resolutionCache.set(renderResolution);

    _enableCEGUIRendering = !(ParamHandler::getInstance().getParam<bool>("GUI.CEGUI.SkipRendering"));
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

    setCursorPosition(mouseState.X.abs, mouseState.Y.abs);

    _defaultMsgBox = addMsgBox("AssertMsgBox", "Assertion failure",
                               "Assertion failed with message: ");

    _init = true;
    return true;
}

void GUI::onChangeResolution(U16 w, U16 h) {
    _resolutionCache.set(w, h);
    CEGUI::System::getSingleton().notifyDisplaySizeChanged(CEGUI::Sizef(w, h));


    for (const guiMap::value_type& guiStackIterator : _guiStack) {
        guiStackIterator.second->onChangeResolution(w, h);
    }
}

void GUI::selectionChangeCallback(Scene* const activeScene) {
    GUIEditor::getInstance().Handle_ChangeSelection(
        activeScene->getCurrentSelection());
}

void GUI::setCursorPosition(I32 x, I32 y) const {
    CEGUI_DEFAULT_CTX.injectMousePosition(to_float(x), to_float(y));
}

bool GUI::onKeyDown(const Input::KeyEvent& key) {
    if (!_init) {
        return true;
    }

    return !_ceguiInput.onKeyDown(key);
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

    return !_ceguiInput.onKeyUp(key);
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

    return !_ceguiInput.mouseMoved(arg);
}

bool GUI::mouseButtonPressed(const Input::MouseEvent& arg,
                             Input::MouseButton button) {
    if (!_init) {
        return true;
    }

    bool processed = false;
    if (!_ceguiInput.mouseButtonPressed(arg, button)) {
        if (button == Input::MouseButton::MB_Left) {
            GUIEvent event;
            event.mouseClickCount = 0;
            for (guiMap::value_type& guiStackIterator : _guiStack) {
                guiStackIterator.second->onMouseDown(event);
            }
        }
        processed = true;
    }

    return processed;
}

bool GUI::mouseButtonReleased(const Input::MouseEvent& arg,
                              Input::MouseButton button) {
    if (!_init) {
        return true;
    }

    bool processed = false;
    if (!_ceguiInput.mouseButtonReleased(arg, button)) {
        if (button == Input::MouseButton::MB_Left) {
            GUIEvent event;
            event.mouseClickCount = 1;
            for (guiMap::value_type& guiStackIterator : _guiStack) {
                guiStackIterator.second->onMouseUp(event);
            }
        }
        processed = true;
    }

    return processed;
}

bool GUI::joystickAxisMoved(const Input::JoystickEvent& arg, I8 axis) {
    return !_ceguiInput.joystickAxisMoved(arg, axis);
}

bool GUI::joystickPovMoved(const Input::JoystickEvent& arg, I8 pov) {
    return !_ceguiInput.joystickPovMoved(arg, pov);
}

bool GUI::joystickButtonPressed(const Input::JoystickEvent& arg,
                                Input::JoystickButton button) {
    return !_ceguiInput.joystickButtonPressed(arg, button);
}

bool GUI::joystickButtonReleased(const Input::JoystickEvent& arg,
                                 Input::JoystickButton button) {
    return !_ceguiInput.joystickButtonReleased(arg, button);
}

bool GUI::joystickSliderMoved(const Input::JoystickEvent& arg, I8 index) {
    return !_ceguiInput.joystickSliderMoved(arg, index);
}

bool GUI::joystickVector3DMoved(const Input::JoystickEvent& arg, I8 index) {
    return !_ceguiInput.joystickVector3DMoved(arg, index);
}

GUIButton* GUI::addButton(const stringImpl& ID,
                          const stringImpl& text,
                          const vec2<I32>& position,
                          const vec2<U32>& dimensions,
                          const vec3<F32>& color,
                          ButtonCallback callback,
                          const stringImpl& rootSheetID) {
    vec2<F32> relOffset(((_resolutionCache.width - position.x) * 100.0f) / _resolutionCache.width,
                        (position.y * 100.0f) / _resolutionCache.height);

    vec2<F32> relDim((dimensions.x * 100.0f) / _resolutionCache.width,
                     (dimensions.y * 100.0f) / _resolutionCache.height);

    CEGUI::Window* parent = nullptr;
    if (!rootSheetID.empty()) {
        parent = CEGUI_DEFAULT_CTX.getRootWindow()->getChild(rootSheetID.c_str());
    }
    if (!parent) {
        parent = _rootSheet;
    }
    GUIButton* btn =
        MemoryManager_NEW GUIButton(ID, text, _defaultGUIScheme, relOffset,
                                    relDim, color, parent, callback);
    ULL idHash = _ID_RT(ID);
    guiMap::iterator it = _guiStack.find(idHash);
    if (it != std::end(_guiStack)) {
        MemoryManager::SAFE_UPDATE(it->second, btn);
    } else {
        hashAlg::insert(_guiStack, std::make_pair(idHash, static_cast<GUIElement*>(btn)));
    }

    return btn;
}

GUIMessageBox* GUI::addMsgBox(const stringImpl& ID, const stringImpl& title,
                              const stringImpl& message,
                              const vec2<I32>& offsetFromCentre) {
    ULL idHash = _ID_RT(ID);
    GUIMessageBox* box = MemoryManager_NEW GUIMessageBox(
        ID, title, message, offsetFromCentre, _rootSheet);
    guiMap::iterator it = _guiStack.find(idHash);
    if (it != std::end(_guiStack)) {
        MemoryManager::SAFE_UPDATE(it->second, box);
    } else {
        hashAlg::insert(_guiStack, std::make_pair(idHash, static_cast<GUIElement*>(box)));
    }

    return box;
}

GUIText* GUI::addText(const stringImpl& ID, const vec2<I32>& position,
                      const stringImpl& font, const vec3<F32>& color,
                      const stringImpl& text) {
    ULL idHash = _ID_RT(ID);

    GUIText* t = MemoryManager_NEW GUIText(ID,
                                           text,
                                           vec2<F32>(position.width, 
                                                     position.height),
                                           font,
                                           color,
                                           _rootSheet);
    guiMap::iterator it = _guiStack.find(idHash);
    if (it != std::end(_guiStack)) {
        MemoryManager::SAFE_UPDATE(it->second, t);
    } else {
        hashAlg::insert(_guiStack, std::make_pair(idHash, static_cast<GUIElement*>(t)));
    }

    return t;
}

GUIFlash* GUI::addFlash(const stringImpl& ID, stringImpl movie,
                        const vec2<U32>& position, const vec2<U32>& extent) {
    ULL idHash = _ID_RT(ID);
    GUIFlash* flash = MemoryManager_NEW GUIFlash(_rootSheet);
    guiMap::iterator it = _guiStack.find(idHash);
    if (it != std::end(_guiStack)) {
        MemoryManager::SAFE_UPDATE(it->second, flash);
    } else {
        hashAlg::insert(_guiStack, std::make_pair(idHash, static_cast<GUIElement*>(flash)));
    }
    return flash;
}

GUIText* GUI::modifyText(const char* ID, const stringImpl& text) {
    ULL idHash = _ID_RT(ID);
    
    guiMap::iterator it = _guiStack.find(idHash);
    if (it == std::cend(_guiStack)) {
        return nullptr;
    }
 
    GUIElement* element = it->second;
    assert(element->getType() == GUIType::GUI_TEXT);

    GUIText* textElement = dynamic_cast<GUIText*>(element);
    assert(textElement != nullptr);

    textElement->text(text);
    
    return textElement;
}
};
