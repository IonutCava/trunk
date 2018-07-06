#include "Headers/GUI.h"
#include "Headers/SceneGUIElements.h"

#include "Headers/GUIFlash.h"
#include "Headers/GUIText.h"
#include "Headers/GUIButton.h"
#include "Headers/GUIConsole.h"
#include "Headers/GUIMessageBox.h"
#include "GUIEditor/Headers/GUIEditor.h"

#include "Scenes/Headers/Scene.h"
#include "Core/Headers/XMLEntryData.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
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
namespace {
    GUIMessageBox* g_assertMsgBox = nullptr;
};

void DIVIDE_ASSERT_MSG_BOX(const char* failMessage) {
    if (g_assertMsgBox) {
        g_assertMsgBox->setTitle("Assertion Failed!");
        g_assertMsgBox->setMessage(stringImpl("Assert: ") + failMessage);
        g_assertMsgBox->setMessageType(GUIMessageBox::MessageType::MESSAGE_ERROR);
        g_assertMsgBox->show();
    }
}

GUI::GUI(Kernel& parent)
    : GUIInterface(*this, vec2<U16>(1, 1)), //<dangerous, but better than a Singleton
      KernelComponent(parent),
      _init(false),
      _rootSheet(nullptr),
      _defaultMsgBox(nullptr),
      _enableCEGUIRendering(false),
      _debugVarCacheCount(0),
      _activeScene(nullptr),
      _guiEditor(nullptr),
      _console(nullptr),
      _textRenderInterval(Time::MillisecondsToMicroseconds(10))
{
    // 500ms
    _ceguiInput.setInitialDelay(0.500f);
}

GUI::~GUI()
{
    destroy();
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
    GUIMapPerScene::const_iterator it = _guiStack.find(scene->getGUID());
    if (it != std::cend(_guiStack)) {
        _guiStack.erase(it);
    }
}

void GUI::draw(GFXDevice& context) const {
    static vectorImpl<GUITextBatchEntry> textBatch;

    if (!_init || !_activeScene) {
        return;
    }

    _guiShader->bind();

    // global elements
    textBatch.resize(0);

    for (U8 i = 0; i < to_const_uint(GUIType::COUNT); ++i) {
        if (i != to_const_uint(GUIType::GUI_TEXT)) {
            for (const GUIMap::value_type& guiStackIterator : _guiElements[i]) {
                GUIElement& element = *guiStackIterator.second.first;
                // Skip hidden elements
                if (element.isVisible()) {
                    element.draw(context);
                }
            }
        }
    }

    //cache text elements
    for (const GUIMap::value_type& guiStackIterator : _guiElements[to_const_uint(GUIType::GUI_TEXT)]) {
        GUIText& textElement = static_cast<GUIText&>(*guiStackIterator.second.first);
        if (!textElement.text().empty()) {
            textBatch.emplace_back(&textElement, textElement.getPosition(), textElement.getStateBlockHash());
        }
    }

    if (!textBatch.empty()) {
        Attorney::GFXDeviceGUI::drawText(context, textBatch);
    }

    ReadLock r_lock(_guiStackLock);
    // scene specific
    GUIMapPerScene::const_iterator it = _guiStack.find(_activeScene->getGUID());
    if (it != std::cend(_guiStack)) {
        it->second->draw(context);
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

    _guiEditor->update(deltaTime);

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

bool GUI::init(PlatformContext& context, ResourceCache& cache, const vec2<U16>& renderResolution) {
    if (_init) {
        Console::d_errorfn(Locale::get(_ID("ERROR_GUI_DOUBLE_INIT")));
        return false;
    }

    onChangeResolution(renderResolution.width, renderResolution.height);

    _enableCEGUIRendering = !context.config().gui.cegui.skipRendering;

    _guiEditor = MemoryManager_NEW GUIEditor(context, cache);
    _console = MemoryManager_NEW GUIConsole(context, cache);

    if (Config::Build::IS_DEBUG_BUILD) {
        CEGUI::Logger::getSingleton().setLoggingLevel(CEGUI::Informative);
    }

    CEGUI::DefaultResourceProvider* rp
        = static_cast<CEGUI::DefaultResourceProvider*>(
            CEGUI::System::getSingleton().getResourceProvider());

    CEGUI::String CEGUIInstallSharePath(Paths::g_assetsLocation + Paths::g_GUILocation);
    rp->setResourceGroupDirectory("schemes", CEGUIInstallSharePath + "schemes/");
    rp->setResourceGroupDirectory("imagesets", CEGUIInstallSharePath + "imagesets/");
    rp->setResourceGroupDirectory("fonts", CEGUIInstallSharePath + Paths::g_FontsPath);
    rp->setResourceGroupDirectory("layouts", CEGUIInstallSharePath + "layouts/");
    rp->setResourceGroupDirectory("looknfeels", CEGUIInstallSharePath + "looknfeel/");
    rp->setResourceGroupDirectory("lua_scripts", CEGUIInstallSharePath + "lua_scripts/");
    rp->setResourceGroupDirectory("schemas", CEGUIInstallSharePath + "xml_schemas/");
    rp->setResourceGroupDirectory("animations", CEGUIInstallSharePath + "animations/");

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
    _defaultGUIScheme = context.config().gui.cegui.defaultGUIScheme;
    CEGUI::SchemeManager::getSingleton().createFromFile((_defaultGUIScheme + ".scheme").c_str());

    _rootSheet = CEGUI::WindowManager::getSingleton().createWindow(
        "DefaultWindow", "root_window");
    _rootSheet->setMousePassThroughEnabled(true);
    CEGUI_DEFAULT_CTX.setRootWindow(_rootSheet);
    CEGUI_DEFAULT_CTX.setDefaultTooltipType((_defaultGUIScheme + "/Tooltip").c_str());
    
    _rootSheet->setPixelAligned(false);
    assert(_console);
    //_console->CreateCEGUIWindow();
    _guiEditor->init();

    ResourceDescriptor immediateModeShader("ImmediateModeEmulation.GUI");
    immediateModeShader.setThreadedLoading(false);
    _guiShader = CreateResource<ShaderProgram>(cache, immediateModeShader);
    _guiShader->Uniform("dvd_WorldMatrix", mat4<F32>());
    context.gfx().add2DRenderFunction(GUID_DELEGATE_CBK(DELEGATE_BIND(&GUI::draw, this, std::ref(context.gfx()))),
                                      std::numeric_limits<U32>::max() - 1);
    const OIS::MouseState& mouseState = context.input().getKeyboardMousePair(0).second->getMouseState();

    setCursorPosition(mouseState.X.abs, mouseState.Y.abs);

    _defaultMsgBox = addMsgBox(_ID("AssertMsgBox"),
                               "Assertion failure",
                               "Assertion failed with message: ");

    g_assertMsgBox = _defaultMsgBox;

    GUIButton::soundCallback(DELEGATE_BIND(&SFXDevice::playSound, 
                                           &context.sfx(),
                                           std::placeholders::_1));
    _init = true;
    return true;
}

void GUI::destroy() {
    if (_init) {
        Console::printfn(Locale::get(_ID("STOP_GUI")));
        MemoryManager::DELETE(_guiEditor);
        MemoryManager::DELETE(_console);

        {
            WriteLock w_lock(_guiStackLock);
            assert(_guiStack.empty());
            for (U8 i = 0; i < to_const_uint(GUIType::COUNT); ++i) {
                for (GUIMap::value_type& it : _guiElements[i]) {
                    MemoryManager::DELETE(it.second.first);
                }
                _guiElements[i].clear();
            }
        }

        _defaultMsgBox = nullptr;
        g_assertMsgBox = nullptr;
        // Close CEGUI
        try {
            CEGUI::System::destroy();
        }
        catch (...) {
            Console::d_errorfn(Locale::get(_ID("ERROR_CEGUI_DESTROY")));
        }
        _guiShader.reset();
        _init = false;
    }
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
    _guiEditor->Handle_ChangeSelection(activeScene->getCurrentSelection(0));
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
            _guiEditor->setVisible(!_guiEditor->isVisible());
        }
    }

    return !_ceguiInput.onKeyUp(key);
}

bool GUI::mouseMoved(const Input::MouseEvent& arg) {
    if (!_init) {
        return true;
    }

    GUIEvent event;
    event.mousePoint.x = to_float(arg._event.state.X.abs);
    event.mousePoint.y = to_float(arg._event.state.Y.abs);

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


GUIElement* GUI::getGUIElementImpl(I64 sceneID, U64 elementName, GUIType type) const {
    if (sceneID != 0) {
        ReadLock r_lock(_guiStackLock);
        GUIMapPerScene::const_iterator it = _guiStack.find(sceneID);
        if (it != std::cend(_guiStack)) {
            return it->second->getGUIElement(elementName);
        }
    } else {
        return GUIInterface::getGUIElementImpl(elementName, type);
    }

    return nullptr;
}

GUIElement* GUI::getGUIElementImpl(I64 sceneID, I64 elementID, GUIType type) const {
    if (sceneID != 0) {
        ReadLock r_lock(_guiStackLock);
        GUIMapPerScene::const_iterator it = _guiStack.find(sceneID);
        if (it != std::cend(_guiStack)) {
            return it->second->getGUIElement(elementID);
        }
    } else {
        return GUIInterface::getGUIElementImpl(elementID, type);
    }

    return nullptr;
}

};
