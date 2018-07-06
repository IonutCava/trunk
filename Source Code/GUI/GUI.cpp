#include "Headers/GUI.h"

#include "Headers/GUIFlash.h"
#include "Headers/GUIText.h"
#include "Headers/GUIButton.h"
#include "Headers/GUIConsole.h"
#include "Headers/GUIMessageBox.h"
#include "GUIEditor/Headers/GUIEditor.h"

#include "Scenes/Headers/Scene.h"
#include "Core/Headers/ParamHandler.h"
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
      _activeSceneGUID(0),
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
    for (GUIMapPerScene::value_type& it : _guiStack) {
        MemoryManager::DELETE_HASHMAP(it.second);
    }
    _guiStack.clear();

    _defaultMsgBox = nullptr;

    // Close CEGUI
    try {
        CEGUI::System::destroy();
    } catch (...) {
        Console::d_errorfn(Locale::get(_ID("ERROR_CEGUI_DESTROY")));
    }
}

I64 GUI::activeSceneGUID() const { 
    return _activeSceneGUID;
}

void GUI::onChangeScene(I64 newSceneGUID) {
    if (_activeSceneGUID > 0 && _activeSceneGUID != newSceneGUID) {
        GUIMapPerScene::const_iterator it = _guiStack.find(_activeSceneGUID);
        for (const GUIMap::value_type& guiStackIterator : it->second) {
           guiStackIterator.second->setVisible(false);
        }
    }

    _activeSceneGUID = newSceneGUID;
}

void GUI::draw() const {
    if (!_init || _activeSceneGUID == -1) {
        return;
    }

    _guiShader->bind();

    // global elements
    GUIMapPerScene::const_iterator it = _guiStack.find(0);
    for (const GUIMap::value_type& guiStackIterator : it->second) {
        GUIElement& element = *guiStackIterator.second;
        // Skip hidden elements
        if (element.isVisible()) {
            element.draw();
        }
    }

    // scene specific
    it = _guiStack.find(_activeSceneGUID);
    if (it != std::cend(_guiStack)) {
        for (const GUIMap::value_type& guiStackIterator : it->second) {
            GUIElement& element = *guiStackIterator.second;
            // Skip hidden elements
            if (element.isVisible()) {
                element.draw();
            }
        }
    }

    const OIS::MouseState& mouseState =
        Input::InputInterface::instance().getMouse().getMouseState();

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

    GUIEditor::instance().update(deltaTime);

    const DebugInterface& debugInterface = DebugInterface::instance();
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

    _enableCEGUIRendering = !(ParamHandler::instance().getParam<bool>(_ID("GUI.CEGUI.SkipRendering")));
#ifdef _DEBUG
    CEGUI::Logger::getSingleton().setLoggingLevel(CEGUI::Informative);
#endif
    CEGUI::DefaultResourceProvider* rp = nullptr;
    rp = static_cast<CEGUI::DefaultResourceProvider*>(
        CEGUI::System::getSingleton().getResourceProvider());
    CEGUI::String CEGUIInstallSharePath(
        ParamHandler::instance().getParam<stringImpl>(_ID("assetsLocation")).c_str());
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
        ParamHandler::instance().getParam<stringImpl>(_ID("GUI.defaultScheme"));
    CEGUI::SchemeManager::getSingleton().createFromFile((_defaultGUIScheme + ".scheme").c_str());

    _rootSheet = CEGUI::WindowManager::getSingleton().createWindow(
        "DefaultWindow", "root_window");
    _rootSheet->setMousePassThroughEnabled(true);
    CEGUI_DEFAULT_CTX.setRootWindow(_rootSheet);
    CEGUI_DEFAULT_CTX.setDefaultTooltipType((_defaultGUIScheme + "/Tooltip").c_str());
    
    _rootSheet->setPixelAligned(false);
    assert(_console);
    //_console->CreateCEGUIWindow();
    GUIEditor::instance().init();

    ResourceDescriptor immediateModeShader("ImmediateModeEmulation.GUI");
    immediateModeShader.setThreadedLoading(false);
    _guiShader = CreateResource<ShaderProgram>(immediateModeShader);
    _guiShader->Uniform("dvd_WorldMatrix", mat4<F32>());
    GFX_DEVICE.add2DRenderFunction(DELEGATE_BIND(&GUI::draw, this),
                                   std::numeric_limits<U32>::max() - 1);
    const OIS::MouseState& mouseState =
        Input::InputInterface::instance().getMouse().getMouseState();

    setCursorPosition(mouseState.X.abs, mouseState.Y.abs);

    _defaultMsgBox = addGlobalMsgBox(_ID("AssertMsgBox"),
                                     "Assertion failure",
                                     "Assertion failed with message: ");

    _init = true;
    return true;
}

void GUI::onChangeResolution(U16 w, U16 h) {
    _resolutionCache.set(w, h);
    CEGUI::System::getSingleton().notifyDisplaySizeChanged(CEGUI::Sizef(w, h));

    
    for (const GUIMapPerScene::value_type& guiSceneIterator : _guiStack) {
        for (const GUIMap::value_type& guiStackIterator : guiSceneIterator.second) {
            guiStackIterator.second->onChangeResolution(w, h);
        }
    }
}

void GUI::selectionChangeCallback(Scene* const activeScene) {
    GUIEditor::instance().Handle_ChangeSelection(activeScene->getCurrentSelection());
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
        GUIEditor::instance().setVisible(
            !GUIEditor::instance().isVisible());
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

    GUIMapPerScene::const_iterator it = _guiStack.find(_activeSceneGUID);
    assert(it != std::cend(_guiStack));
    for (const GUIMap::value_type& guiStackIterator : it->second) {
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
            GUIMapPerScene::const_iterator it = _guiStack.find(_activeSceneGUID);
            assert(it != std::cend(_guiStack));
            for (GUIMap::value_type guiStackIterator : it->second) {
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
            GUIMapPerScene::const_iterator it = _guiStack.find(_activeSceneGUID);
            assert(it != std::cend(_guiStack));
            for (GUIMap::value_type guiStackIterator : it->second) {
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

void GUI::addElement(ULL id, I64 sceneID, GUIElement* element) {
    GUIMap& stack = _guiStack[sceneID];

    GUIMap::iterator it2 = stack.find(id);
    if (it2 != std::end(stack)) {
        MemoryManager::SAFE_UPDATE(it2->second, element);
    } else {
        hashAlg::insert(stack, std::make_pair(id, element));
    }
}

GUIButton* GUI::addGlobalButton(ULL ID,
                                const stringImpl& text,
                                const vec2<I32>& position,
                                const vec2<U32>& dimensions,
                                ButtonCallback callback,
                                const stringImpl& rootSheetID) {
    vec2<F32> relOffset((position.x * 100.0f) / _resolutionCache.width,
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

    GUIButton* btn = MemoryManager_NEW GUIButton(ID, text, _defaultGUIScheme, relOffset, relDim, parent, callback);

    addElement(ID, 0, btn);

    return btn;
}

GUIButton* GUI::addButton(ULL ID,
                          const stringImpl& text,
                          const vec2<I32>& position,
                          const vec2<U32>& dimensions,
                          ButtonCallback callback,
                          const stringImpl& rootSheetID) {
    vec2<F32> relOffset((position.x * 100.0f) / _resolutionCache.width,
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

    GUIButton* btn = MemoryManager_NEW GUIButton(ID, text, _defaultGUIScheme, relOffset, relDim, parent, callback);

    addElement(ID, _activeSceneGUID, btn);

    return btn;
}

GUIMessageBox* GUI::addMsgBox(ULL ID,
                              const stringImpl& title,
                              const stringImpl& message,
                              const vec2<I32>& offsetFromCentre) {
    GUIMessageBox* box = MemoryManager_NEW GUIMessageBox(ID, title, message, offsetFromCentre, _rootSheet);
    addElement(ID, _activeSceneGUID, box);

    return box;
}

GUIMessageBox* GUI::addGlobalMsgBox(ULL ID,
                                    const stringImpl& title,
                                    const stringImpl& message,
                                    const vec2<I32>& offsetFromCentre) {
    GUIMessageBox* box = MemoryManager_NEW GUIMessageBox(ID, title, message, offsetFromCentre, _rootSheet);
    addElement(ID, 0, box);

    return box;
}
GUIText* GUI::addText(ULL ID,
                      const vec2<I32>& position,
                      const stringImpl& font,
                      const vec4<U8>& color,
                      const stringImpl& text,
                      U32 fontSize) {

    GUIText* t = MemoryManager_NEW GUIText(ID,
                                           text,
                                           vec2<F32>(position.width, 
                                                     position.height),
                                           font,
                                           color,
                                           _rootSheet,
                                           fontSize);

    addElement(ID, _activeSceneGUID, t);

    return t;
}

GUIText* GUI::addGlobalText(ULL ID,
                            const vec2<I32>& position,
                            const stringImpl& font,
                            const vec4<U8>& color,
                            const stringImpl& text,
                            U32 fontSize) {

    GUIText* t = MemoryManager_NEW GUIText(ID,
                                           text,
                                           vec2<F32>(position.width,
                                                     position.height),
                                           font,
                                           color,
                                           _rootSheet,
                                           fontSize);

    addElement(ID, 0, t);

    return t;
}

GUIFlash* GUI::addFlash(ULL ID,
                        stringImpl movie,
                        const vec2<U32>& position,
                        const vec2<U32>& extent) {

    GUIFlash* flash = MemoryManager_NEW GUIFlash(ID, _rootSheet);
    addElement(ID, _activeSceneGUID, flash);

    return flash;
}

GUIFlash* GUI::addGlobalFlash(ULL ID,
                              stringImpl movie,
                              const vec2<U32>& position,
                              const vec2<U32>& extent) {

    GUIFlash* flash = MemoryManager_NEW GUIFlash(ID, _rootSheet);
    addElement(ID, 0, flash);

    return flash;
}

GUIText* GUI::modifyText(ULL ID, const stringImpl& text) {
    GUIMapPerScene::iterator it = _guiStack.find(_activeSceneGUID);
    if (it == std::end(_guiStack)) {
        return nullptr;
    }

    GUIMap::iterator it2 = it->second.find(ID);

    if (it2 == std::cend(it->second)) {
        return nullptr;
    }
 
    GUIElement* element = it2->second;
    assert(element->getType() == GUIType::GUI_TEXT);

    GUIText* textElement = dynamic_cast<GUIText*>(element);
    assert(textElement != nullptr);

    textElement->text(text);
    
    return textElement;
}

GUIText* GUI::modifyGlobalText(ULL ID, const stringImpl& text) {
    GUIMapPerScene::iterator it = _guiStack.find(0);

    GUIMap::iterator it2 = it->second.find(ID);

    if (it2 == std::cend(it->second)) {
        return nullptr;
    }

    GUIElement* element = it2->second;
    assert(element->getType() == GUIType::GUI_TEXT);

    GUIText* textElement = dynamic_cast<GUIText*>(element);
    assert(textElement != nullptr);

    textElement->text(text);

    return textElement;
}
};
