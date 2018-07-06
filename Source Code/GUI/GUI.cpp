#include "Headers/GUI.h"
#include "Headers/SceneGUIElements.h"

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
    : GUIInterface(vec2<U16>(1, 1)),
      _init(false),
      _rootSheet(nullptr),
      _defaultMsgBox(nullptr),
      _enableCEGUIRendering(false),
      _debugVarCacheCount(0),
      _activeScene(nullptr),
      _console(MemoryManager_NEW GUIConsole())
{
    // 500ms
    _textRenderInterval = Time::MillisecondsToMicroseconds(10);
    _ceguiInput.setInitialDelay(0.500f);
    GUIEditor::createInstance();
}

GUI::~GUI()
{
    WriteLock w_lock(_guiStackLock);
    Console::printfn(Locale::get(_ID("STOP_GUI")));
    GUIEditor::destroyInstance();
    MemoryManager::DELETE(_console);
    assert(_guiStack.empty());
    for (GUIMap::value_type& it : _guiElements) {
        MemoryManager::DELETE(it.second.first);
    }
    _guiElements.clear();

    _defaultMsgBox = nullptr;

    // Close CEGUI
    try {
        CEGUI::System::destroy();
    } catch (...) {
        Console::d_errorfn(Locale::get(_ID("ERROR_CEGUI_DESTROY")));
    }
}

void GUI::onChangeScene(Scene* newScene) {
    assert(newScene != nullptr);
    ReadLock r_lock(_guiStackLock);
    if (_activeScene != nullptr && _activeScene->getGUID() != newScene->getGUID()) {
        GUIMapPerScene::const_iterator it = _guiStack.find(_activeScene->getGUID());
        if (it != std::cend(_guiStack)) {
            it->second->onDisable();
        }
    }

    GUIMapPerScene::const_iterator it = _guiStack.find(newScene->getGUID());
    if (it != std::cend(_guiStack)) {
        it->second->onEnable();
    } else {
        SceneGUIElements* elements = Attorney::SceneGUI::guiElements(*newScene);
        hashAlg::emplace(_guiStack, newScene->getGUID(), elements);
        elements->onEnable();
    }

    _activeScene = newScene;
}

void GUI::onUnloadScene(Scene* scene) {
    assert(scene != nullptr);
    WriteLock w_lock(_guiStackLock);
    _guiStack.erase(_guiStack.find(scene->getGUID()));
}

void GUI::draw() const {
    static vectorImpl<GUITextBatchEntry> textBatch;

    if (!_init || !_activeScene) {
        return;
    }

    const OIS::MouseState& mouseState = Input::InputInterface::instance().getMouse().getMouseState();

    setCursorPosition(mouseState.X.abs, mouseState.Y.abs);

    _guiShader->bind();

    // global elements
    textBatch.resize(0);
    for (const GUIMap::value_type& guiStackIterator : _guiElements) {
        GUIElement& element = *guiStackIterator.second.first;
        // Skip hidden elements
        if (element.isVisible()) {
            // Cache text elements
            if (element.getType() == GUIType::GUI_TEXT) {
                GUIText& textElement = static_cast<GUIText&>(element);
                if (!textElement.text().empty()) {
                    textBatch.emplace_back(&textElement, textElement.getPosition(), textElement.getStateBlockHash());
                }
            } else {
                element.draw();
            }
        }
    }

    if (!textBatch.empty()) {
        Attorney::GFXDeviceGUI::drawText(textBatch);
    }

    ReadLock r_lock(_guiStackLock);
    // scene specific
    GUIMapPerScene::const_iterator it = _guiStack.find(_activeScene->getGUID());
    if (it != std::cend(_guiStack)) {
        it->second->draw();
    }

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
    CEGUI::System::getSingleton().injectTimePulse(Time::MicrosecondsToSeconds<F32>(deltaTime));
    CEGUI::System::getSingleton().getDefaultGUIContext().injectTimePulse(Time::MicrosecondsToSeconds<F32>(deltaTime));

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
    onChangeResolution(renderResolution.width, renderResolution.height);

    _enableCEGUIRendering = !(ParamHandler::instance().getParam<bool>(_ID("GUI.CEGUI.SkipRendering")));

    if (Config::Build::IS_DEBUG_BUILD) {
        CEGUI::Logger::getSingleton().setLoggingLevel(CEGUI::Informative);
    }

    CEGUI::DefaultResourceProvider* rp
        = static_cast<CEGUI::DefaultResourceProvider*>(
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
    GFX_DEVICE.add2DRenderFunction(GUID_DELEGATE_CBK(DELEGATE_BIND(&GUI::draw, this)),
                                                     std::numeric_limits<U32>::max() - 1);
    const OIS::MouseState& mouseState =
        Input::InputInterface::instance().getMouse().getMouseState();

    setCursorPosition(mouseState.X.abs, mouseState.Y.abs);

    _defaultMsgBox = addMsgBox(_ID("AssertMsgBox"),
                               "Assertion failure",
                               "Assertion failed with message: ");

    _init = true;
    return true;
}

void GUI::onChangeResolution(U16 w, U16 h) {
    CEGUI::System::getSingleton().notifyDisplaySizeChanged(CEGUI::Sizef(w, h));

    GUIInterface::onChangeResolution(w, h);

    ReadLock r_lock(_guiStackLock);
    if (!_guiStack.empty()) {
        // scene specific
        GUIMapPerScene::const_iterator it = _guiStack.find(_activeScene->getGUID());
        if (it != std::cend(_guiStack)) {
            it->second->onChangeResolution(w, h);
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

    if (Config::Build::IS_DEBUG_BUILD) {
        if (key._key == Input::KeyCode::KC_F11) {
            GUIEditor::instance().setVisible(
                !GUIEditor::instance().isVisible());
        }
    }

    return !_ceguiInput.onKeyUp(key);
}

bool GUI::mouseMoved(const Input::MouseEvent& arg) {
    if (!_init) {
        return true;
    }

    GUIEvent event;
    event.mousePoint.x = to_float(arg.state.X.abs);
    event.mousePoint.y = to_float(arg.state.Y.abs);

    GUIInterface::mouseMoved(event);

    // scene specific
    ReadLock r_lock(_guiStackLock);
    GUIMapPerScene::const_iterator it = _guiStack.find(_activeScene->getGUID());
    if (it != std::cend(_guiStack)) {
        it->second->mouseMoved(event);
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

            GUIInterface::onMouseDown(event);

            // scene specific
            ReadLock r_lock(_guiStackLock);
            GUIMapPerScene::const_iterator it = _guiStack.find(_activeScene->getGUID());
            if (it != std::cend(_guiStack)) {
                it->second->onMouseDown(event);
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

            GUIInterface::onMouseUp(event);

            // scene specific
            ReadLock r_lock(_guiStackLock);
            GUIMapPerScene::const_iterator it = _guiStack.find(_activeScene->getGUID());
            if (it != std::cend(_guiStack)) {
                it->second->onMouseUp(event);
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


GUIElement* GUI::getGUIElementImpl(I64 sceneID, U64 elementName) const {
    if (sceneID != 0) {
        ReadLock r_lock(_guiStackLock);
        GUIMapPerScene::const_iterator it = _guiStack.find(sceneID);
        if (it != std::cend(_guiStack)) {
            return it->second->getGUIElement(elementName);
        }
    } else {
        return GUIInterface::getGUIElement(elementName);
    }

    return nullptr;
}

GUIElement* GUI::getGUIElementImpl(I64 sceneID, I64 elementID) const {
    if (sceneID != 0) {
        ReadLock r_lock(_guiStackLock);
        GUIMapPerScene::const_iterator it = _guiStack.find(sceneID);
        if (it != std::cend(_guiStack)) {
            return it->second->getGUIElement(elementID);
        }
    } else {
        return GUIInterface::getGUIElement(elementID);
    }

    return nullptr;
}

};
