#include "stdafx.h"

#include "Headers/GUI.h"
#include "Headers/SceneGUIElements.h"

#include "Headers/GUIFlash.h"
#include "Headers/GUIText.h"
#include "Headers/GUIButton.h"
#include "Headers/GUIConsole.h"
#include "Headers/GUIMessageBox.h"

#include "Scenes/Headers/Scene.h"
#include "Core/Headers/XMLEntryData.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Core/Debugging/Headers/DebugInterface.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Input/Headers/InputInterface.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

#if !defined(CEGUI_STATIC)
#define CEGUI_STATIC
#endif
#include <CEGUI/CEGUI.h>

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
  : GUIInterface(*this),
    KernelComponent(parent),
    _ceguiInput(*this),
    _init(false),
    _rootSheet(nullptr),
    _defaultMsgBox(nullptr),
    _debugVarCacheCount(0),
    _activeScene(nullptr),
    _console(nullptr),
    _ceguiContext(nullptr),
    _ceguiRenderer(nullptr),
    _ceguiRenderTextureTarget(nullptr),
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
        hashAlg::insert(_guiStack, newScene->getGUID(), elements);
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

void GUI::draw(GFXDevice& context, GFX::CommandBuffer& bufferInOut) {
    if (!_init || !_activeScene) {
        return;
    }

    TextElementBatch textBatch;
    for (const GUIMap::value_type& guiStackIterator : _guiElements[to_base(GUIType::GUI_TEXT)]) {
        GUIText& textLabel = static_cast<GUIText&>(*guiStackIterator.second.first);
        if (textLabel.isVisible() && !textLabel.text().empty()) {
            textBatch._data.push_back(textLabel);
        }
    }

    if (!textBatch().empty()) {
        Attorney::GFXDeviceGUI::drawText(context, textBatch, bufferInOut);
    }

    ReadLock r_lock(_guiStackLock);
    // scene specific
    GUIMapPerScene::const_iterator it = _guiStack.find(_activeScene->getGUID());
    if (it != std::cend(_guiStack)) {
        it->second->draw(context, bufferInOut);
    }

    if (parent().platformContext().config().gui.cegui.enabled) {
        if (!parent().platformContext().config().gui.cegui.skipRendering) {

            _ceguiRenderer->beginRendering();

            _ceguiRenderTextureTarget->clear();
            getCEGUIContext().draw();

            _ceguiRenderer->endRendering();

            context.drawFullscreenTexture(getCEGUIRenderTextureData(), bufferInOut);
        }
    }
}

void GUI::update(const U64 deltaTimeUS) {
    if (!_init) {
        return;
    }

    if (parent().platformContext().config().gui.cegui.enabled) {
        _ceguiInput.update(deltaTimeUS);
        CEGUI::System::getSingleton().injectTimePulse(Time::MicrosecondsToSeconds<F32>(deltaTimeUS));
        CEGUI::System::getSingleton().getDefaultGUIContext().injectTimePulse(Time::MicrosecondsToSeconds<F32>(deltaTimeUS));
    }

    if (_console) {
        _console->update(deltaTimeUS);
    }
}

bool GUI::init(PlatformContext& context, ResourceCache& cache, const vec2<U16>& renderResolution) {
    if (_init) {
        Console::d_errorfn(Locale::get(_ID("ERROR_GUI_DOUBLE_INIT")));
        return false;
    }
    _defaultGUIScheme = context.config().gui.cegui.defaultGUIScheme;

    _console = MemoryManager_NEW GUIConsole(*this, context, cache);

    if (Config::Build::IS_DEBUG_BUILD) {
        CEGUI::Logger::getSingleton().setLoggingLevel(CEGUI::Informative);
    }

    CEGUI::DefaultResourceProvider* rp
        = static_cast<CEGUI::DefaultResourceProvider*>(CEGUI::System::getSingleton().getResourceProvider());

    CEGUI::String CEGUIInstallSharePath(Paths::g_assetsLocation + Paths::g_GUILocation);
    rp->setResourceGroupDirectory("schemes", CEGUIInstallSharePath + "schemes/");
    rp->setResourceGroupDirectory("imagesets", CEGUIInstallSharePath + "imagesets/");
    rp->setResourceGroupDirectory("fonts", CEGUIInstallSharePath + Paths::g_fontsPath);
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
    CEGUI::FontManager::getSingleton().createFromFile("DejaVuSans-10-NoScale.font");
    CEGUI::FontManager::getSingleton().createFromFile("DejaVuSans-12-NoScale.font");
    CEGUI::SchemeManager::getSingleton().createFromFile((_defaultGUIScheme + ".scheme").c_str());

    _rootSheet = CEGUI::WindowManager::getSingleton().createWindow("DefaultWindow", "root_window");
    _rootSheet->setMousePassThroughEnabled(true);
    _rootSheet->setUsingAutoRenderingSurface(false);
    _rootSheet->setPixelAligned(false);

    CEGUI::Sizef size(static_cast<float>(renderResolution.width), static_cast<float>(renderResolution.height));
    // We create a CEGUI texture target and create a GUIContext that will use it.
    _ceguiRenderTextureTarget = CEGUI::System::getSingleton().getRenderer()->createTextureTarget();
    _ceguiRenderTextureTarget->declareRenderSize(size);
    _ceguiContext = &CEGUI::System::getSingleton().createGUIContext(static_cast<CEGUI::RenderTarget&>(*_ceguiRenderTextureTarget));

    _ceguiContext->setRootWindow(_rootSheet);
    _ceguiContext->setDefaultTooltipType((_defaultGUIScheme + "/Tooltip").c_str());
  
    _ceguiRenderer = CEGUI::System::getSingleton().getRenderer();
    assert(_console);
    
    _ceguiContext->getMouseCursor().setDefaultImage("GWEN/Tree.Plus");

    _console->createCEGUIWindow();

    _defaultMsgBox = addMsgBox(_ID("AssertMsgBox"),
                               "Assertion failure",
                               "Assertion failed with message: ");

    g_assertMsgBox = _defaultMsgBox;

    GUIButton::soundCallback([&context](const AudioDescriptor_ptr& sound) { context.sfx().playSound(sound); });

    if (parent().platformContext().config().gui.cegui.enabled) {
        CEGUI::System::getSingleton().notifyDisplaySizeChanged(CEGUI::Sizef(renderResolution.width, renderResolution.height));
    }

    _init = true;
    return true;
}

void GUI::destroy() {
    if (_init) {
        Console::printfn(Locale::get(_ID("STOP_GUI")));
        MemoryManager::DELETE(_console);

        {
            WriteLock w_lock(_guiStackLock);
            assert(_guiStack.empty());
            for (U8 i = 0; i < to_base(GUIType::COUNT); ++i) {
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
        _init = false;
    }
}

void GUI::onSizeChange(const SizeChangeParams& params) {
    if (parent().platformContext().config().gui.cegui.enabled) {
        if ((params.isWindowResize || params.isFullScreen)) {
            CEGUI::System::getSingleton().notifyDisplaySizeChanged(CEGUI::Sizef(params.width, params.height));
        }

        if (_ceguiRenderTextureTarget) {
            const Rect<I32>& renderViewport = parent().platformContext().activeWindow().renderingViewport();
            CEGUI::Sizef size(static_cast<float>(renderViewport.z), static_cast<float>(renderViewport.w));
            //_ceguiRenderTextureTarget->setArea(CEGUI::Rectf(renderViewport.x, renderViewport.y, renderViewport.z, renderViewport.w));
            _ceguiRenderTextureTarget->declareRenderSize(size);
        }

    }

    ReadLock r_lock(_guiStackLock);
    if (!_guiStack.empty()) {
        // scene specific
        GUIMapPerScene::const_iterator it = _guiStack.find(_activeScene->getGUID());
        if (it != std::cend(_guiStack)) {
            it->second->onSizeChange(params);
        }
    }

    GUIInterface::onSizeChange(params);
}

void GUI::selectionChangeCallback(Scene* const activeScene, PlayerIndex idx, SceneGraphNode* node) {
    ACKNOWLEDGE_UNUSED(activeScene);
    ACKNOWLEDGE_UNUSED(idx);
    ACKNOWLEDGE_UNUSED(node);
}

void GUI::setCursorPosition(I32 x, I32 y) {
    if (parent().platformContext().config().gui.cegui.enabled) {
        getCEGUIContext().injectMousePosition(to_F32(x), to_F32(y));
    }
}

// Return true if input was consumed
bool GUI::onKeyDown(const Input::KeyEvent& key) {
    if (!_init) {
        return false;
    }

    if (parent().platformContext().config().gui.cegui.enabled) {
        return _ceguiInput.onKeyDown(key);
    }

    return false;
}

// Return true if input was consumed
bool GUI::onKeyUp(const Input::KeyEvent& key) {
    if (!_init) {
        return false;
    }

    if (parent().platformContext().config().gui.cegui.enabled) {
        return _ceguiInput.onKeyUp(key);
    }

    return false;
}

// Return true if input was consumed
bool GUI::mouseMoved(const Input::MouseEvent& arg) {
    if (!_init) {
        return false;
    }

    GUIEvent event;
    event.mousePoint.set(arg.X().abs, arg.Y().abs);

    if (GUIInterface::mouseMoved(event)) {
        return true;
    }

    // scene specific
    ReadLock r_lock(_guiStackLock);
    GUIMapPerScene::const_iterator it = _guiStack.find(_activeScene->getGUID());
    if (it != std::cend(_guiStack)) {
        if (it->second->mouseMoved(event)) {
            return true;
        }
    }

    if (parent().platformContext().config().gui.cegui.enabled) {
        return _ceguiInput.mouseMoved(arg);
    }

    return false;
}

// Return true if input was consumed
bool GUI::mouseButtonPressed(const Input::MouseEvent& arg,
                             Input::MouseButton button) {
    if (!_init) {
        return false;
    }

    bool consumed = false;
    if (parent().platformContext().config().gui.cegui.enabled) {
        consumed = _ceguiInput.mouseButtonPressed(arg, button);
    }

    if (!consumed) {
        if (button == Input::MouseButton::MB_Left) {
            GUIEvent event;
            event.mouseClickCount = 0;

            if (GUIInterface::onMouseDown(event)) {
                return true;
            }

            // scene specific
            ReadLock r_lock(_guiStackLock);
            GUIMapPerScene::const_iterator it = _guiStack.find(_activeScene->getGUID());
            if (it != std::cend(_guiStack)) {
                if (it->second->onMouseDown(event)) {
                    return true;
                }
            }
        }
    }

    return consumed;
}

// Return true if input was consumed
bool GUI::mouseButtonReleased(const Input::MouseEvent& arg,
                              Input::MouseButton button) {
    if (!_init) {
        return false;
    }

    bool consumed = false;
    if (parent().platformContext().config().gui.cegui.enabled) {
        consumed = _ceguiInput.mouseButtonReleased(arg, button);
    }

    if (!consumed) {
        if (button == Input::MouseButton::MB_Left) {
            GUIEvent event;
            event.mouseClickCount = 1;

            if (GUIInterface::onMouseUp(event)) {
                return true;
            }

            // scene specific
            ReadLock r_lock(_guiStackLock);
            GUIMapPerScene::const_iterator it = _guiStack.find(_activeScene->getGUID());
            if (it != std::cend(_guiStack)) {
                if (it->second->onMouseUp(event)) {
                    return true;
                }
            }
        }
    }

    return consumed;
}

// Return true if input was consumed
bool GUI::joystickAxisMoved(const Input::JoystickEvent& arg, I8 axis) {
    if (parent().platformContext().config().gui.cegui.enabled) {
        return _ceguiInput.joystickAxisMoved(arg, axis);
    }

    return false;
}

// Return true if input was consumed
bool GUI::joystickPovMoved(const Input::JoystickEvent& arg, I8 pov) {
    if (parent().platformContext().config().gui.cegui.enabled) {
        return _ceguiInput.joystickPovMoved(arg, pov);
    }

    return false;
}

// Return true if input was consumed
bool GUI::joystickButtonPressed(const Input::JoystickEvent& arg,
                                Input::JoystickButton button) {
    if (parent().platformContext().config().gui.cegui.enabled) {
        return _ceguiInput.joystickButtonPressed(arg, button);
    }

    return false;
}

// Return true if input was consumed
bool GUI::joystickButtonReleased(const Input::JoystickEvent& arg,
                                 Input::JoystickButton button) {
    if (parent().platformContext().config().gui.cegui.enabled) {
        return _ceguiInput.joystickButtonReleased(arg, button);
    }

    return false;
}

// Return true if input was consumed
bool GUI::joystickSliderMoved(const Input::JoystickEvent& arg, I8 index) {
    if (parent().platformContext().config().gui.cegui.enabled) {
        return _ceguiInput.joystickSliderMoved(arg, index);
    }

    return false;
}

// Return true if input was consumed
bool GUI::joystickvector3Moved(const Input::JoystickEvent& arg, I8 index) {
    if (parent().platformContext().config().gui.cegui.enabled) {
        return _ceguiInput.joystickvector3Moved(arg, index);
    }

    return false;
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
CEGUI::GUIContext& GUI::getCEGUIContext() {
    assert(_ceguiContext != nullptr);
    return *_ceguiContext;
}

const CEGUI::GUIContext& GUI::getCEGUIContext() const {
    assert(_ceguiContext != nullptr);
    return *_ceguiContext;
}

TextureData GUI::getCEGUIRenderTextureData() const {
    TextureData ret;

    if (_ceguiRenderTextureTarget != nullptr) {
        const GFXDevice& gfx = _context->parent().platformContext().gfx();
        ret.setHandle(gfx.getHandleFromCEGUITexture(_ceguiRenderTextureTarget->getTexture()));
    }

    return ret;
}
};
