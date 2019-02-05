#include "stdafx.h"

#include "Headers/GUI.h"
#include "Headers/SceneGUIElements.h"

#include "Headers/GUIFlash.h"
#include "Headers/GUIText.h"
#include "Headers/GUIButton.h"
#include "Headers/GUIConsole.h"
#include "Headers/GUIMessageBox.h"

#include "Scenes/Headers/Scene.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/XMLEntryData.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Core/Debugging/Headers/DebugInterface.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

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
    SharedLock r_lock(_guiStackLock);
    if (_activeScene != nullptr && _activeScene->getGUID() != newScene->getGUID()) {
        const GUIMapPerScene::const_iterator it = _guiStack.find(_activeScene->getGUID());
        if (it != std::cend(_guiStack)) {
            it->second->onDisable();
        }
    }

    const GUIMapPerScene::const_iterator it = _guiStack.find(newScene->getGUID());
    if (it != std::cend(_guiStack)) {
        it->second->onEnable();
    } else {
        SceneGUIElements* elements = Attorney::SceneGUI::guiElements(*newScene);
        hashAlg::insert(_guiStack, newScene->getGUID(), elements);
        elements->onEnable();
    }

    _activeScene = newScene;
}

void GUI::onUnloadScene(Scene* const scene) {
    assert(scene != nullptr);
    UniqueLockShared w_lock(_guiStackLock);
    const GUIMapPerScene::const_iterator it = _guiStack.find(scene->getGUID());
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
        const GUIText& textLabel = static_cast<GUIText&>(*guiStackIterator.second.first);
        if (textLabel.isVisible() && !textLabel.text().empty()) {
            textBatch._data.push_back(textLabel);
        }
    }

    if (!textBatch().empty()) {
        Attorney::GFXDeviceGUI::drawText(context, textBatch, bufferInOut);
    }

    {
        SharedLock r_lock(_guiStackLock);
        // scene specific
        const GUIMapPerScene::const_iterator it = _guiStack.find(_activeScene->getGUID());
        if (it != std::cend(_guiStack)) {
            it->second->draw(context, bufferInOut);
        }
    }

    if (parent().platformContext().config().gui.cegui.enabled) {
        if (!parent().platformContext().config().gui.cegui.skipRendering) {

            _ceguiRenderer->beginRendering();

            _ceguiRenderTextureTarget->clear();
            getCEGUIContext().draw();

            _ceguiRenderer->endRendering();

            GFX::SetBlendCommand blendCmd;
            blendCmd._blendProperties = BlendingProperties{
                true,
                BlendProperty::SRC_ALPHA,
                BlendProperty::INV_SRC_ALPHA,
                BlendOperation::ADD
            };
            GFX::EnqueueCommand(bufferInOut, blendCmd);

            context.drawTextureInRenderWindow(getCEGUIRenderTextureData(), bufferInOut);
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

bool GUI::init(PlatformContext& context, ResourceCache& cache) {
    if (_init) {
        Console::d_errorfn(Locale::get(_ID("ERROR_GUI_DOUBLE_INIT")));
        return false;
    }
    _defaultGUIScheme = context.config().gui.cegui.defaultGUIScheme;

    if (Config::Build::IS_DEBUG_BUILD) {
        CEGUI::Logger::getSingleton().setLogFilename(Paths::g_logPath + "CEGUI.log", false);
        CEGUI::Logger::getSingleton().setLoggingLevel(CEGUI::Informative);
    }

    CEGUI::DefaultResourceProvider* rp = static_cast<CEGUI::DefaultResourceProvider*>(CEGUI::System::getSingleton().getResourceProvider());

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

    const vec2<U16>& renderSize = context.gfx().renderingResolution();

    const CEGUI::Sizef size(static_cast<float>(renderSize.width), static_cast<float>(renderSize.height));
    // We create a CEGUI texture target and create a GUIContext that will use it.

    _ceguiRenderer = CEGUI::System::getSingleton().getRenderer();
    _ceguiRenderTextureTarget = _ceguiRenderer->createTextureTarget();
    _ceguiRenderTextureTarget->declareRenderSize(size);
    _ceguiContext = &CEGUI::System::getSingleton().createGUIContext(static_cast<CEGUI::RenderTarget&>(*_ceguiRenderTextureTarget));

    _rootSheet = CEGUI::WindowManager::getSingleton().createWindow("DefaultWindow", "root_window");
    _rootSheet->setMousePassThroughEnabled(true);
    _rootSheet->setUsingAutoRenderingSurface(false);
    _rootSheet->setPixelAligned(false);

    if (parent().platformContext().config().gui.cegui.showDebugCursor) {
        _rootSheet->setMouseCursor("GWEN/Tree.Plus");
    }
    _ceguiContext->setRootWindow(_rootSheet);
    _ceguiContext->setDefaultTooltipType((_defaultGUIScheme + "/Tooltip").c_str());
  

    _console = MemoryManager_NEW GUIConsole(*this, context, cache);
    assert(_console);
    _console->createCEGUIWindow();

    _defaultMsgBox = addMsgBox(_ID("AssertMsgBox"),
                               "Assertion failure",
                               "Assertion failed with message: ");

    g_assertMsgBox = _defaultMsgBox;

    GUIButton::soundCallback([&context](const AudioDescriptor_ptr& sound) { context.sfx().playSound(sound); });

    if (parent().platformContext().config().gui.cegui.enabled) {
        CEGUI::System::getSingleton().notifyDisplaySizeChanged(size);
    }

    _init = true;
    return true;
}

void GUI::destroy() {
    if (_init) {
        Console::printfn(Locale::get(_ID("STOP_GUI")));
        MemoryManager::DELETE(_console);

        {
            UniqueLockShared w_lock(_guiStackLock);
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
    if (params.winGUID != _context->parent().platformContext().app().windowManager().getMainWindow().getGUID() ||
        !parent().platformContext().config().gui.cegui.enabled)
    {
        return;
    }

    const CEGUI::Sizef windowSize(params.width, params.height);
    CEGUI::System::getSingleton().notifyDisplaySizeChanged(windowSize);
    if (_ceguiRenderTextureTarget) {
        _ceguiRenderTextureTarget->declareRenderSize(windowSize);
    }


    if (_rootSheet) {
        const Rect<I32>& renderViewport = parent().platformContext().activeWindow().renderingViewport();
        _rootSheet->setSize(CEGUI::USize(CEGUI::UDim(0.0f, to_F32(renderViewport.z)),
                                            CEGUI::UDim(0.0f, to_F32(renderViewport.w))));
        _rootSheet->setPosition(CEGUI::UVector2(CEGUI::UDim(0.0f, to_F32(renderViewport.x)),
                                                CEGUI::UDim(0.0f, to_F32(renderViewport.y))));
    }

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
bool GUI::mouseMoved(const Input::MouseMoveEvent& arg) {
    if (_init && parent().platformContext().config().gui.cegui.enabled) {
        return _ceguiInput.mouseMoved(arg);
    }

    return false;
}

// Return true if input was consumed
bool GUI::mouseButtonPressed(const Input::MouseButtonEvent& arg) {
    if (_init && parent().platformContext().config().gui.cegui.enabled) {
        return _ceguiInput.mouseButtonPressed(arg);
    }

    return false;
}

// Return true if input was consumed
bool GUI::mouseButtonReleased(const Input::MouseButtonEvent& arg) {
    if (_init && parent().platformContext().config().gui.cegui.enabled) {
        return _ceguiInput.mouseButtonReleased(arg);
    }

    return false;
}

// Return true if input was consumed
bool GUI::joystickAxisMoved(const Input::JoystickEvent& arg) {
    if (parent().platformContext().config().gui.cegui.enabled) {
        return _ceguiInput.joystickAxisMoved(arg);
    }

    return false;
}

// Return true if input was consumed
bool GUI::joystickPovMoved(const Input::JoystickEvent& arg) {
    if (parent().platformContext().config().gui.cegui.enabled) {
        return _ceguiInput.joystickPovMoved(arg);
    }

    return false;
}

// Return true if input was consumed
bool GUI::joystickButtonPressed(const Input::JoystickEvent& arg) {
    if (parent().platformContext().config().gui.cegui.enabled) {
        return _ceguiInput.joystickButtonPressed(arg);
    }

    return false;
}

// Return true if input was consumed
bool GUI::joystickButtonReleased(const Input::JoystickEvent& arg) {
    if (parent().platformContext().config().gui.cegui.enabled) {
        return _ceguiInput.joystickButtonReleased(arg);
    }

    return false;
}

// Return true if input was consumed
bool GUI::joystickBallMoved(const Input::JoystickEvent& arg) {
    if (parent().platformContext().config().gui.cegui.enabled) {
        return _ceguiInput.joystickBallMoved(arg);
    }

    return false;
}

// Return true if input was consumed
bool GUI::joystickAddRemove(const Input::JoystickEvent& arg) {
    if (parent().platformContext().config().gui.cegui.enabled) {
        return _ceguiInput.joystickAddRemove(arg);
    }

    return false;
}

bool GUI::joystickRemap(const Input::JoystickEvent &arg) {
    if (parent().platformContext().config().gui.cegui.enabled) {
        return _ceguiInput.joystickRemap(arg);
    }

    return false;
}

bool GUI::onUTF8(const Input::UTF8Event& arg) {
    ACKNOWLEDGE_UNUSED(arg);

    return false;
}

GUIElement* GUI::getGUIElementImpl(I64 sceneID, U64 elementName, GUIType type) const {
    if (sceneID != 0) {
        SharedLock r_lock(_guiStackLock);
        const GUIMapPerScene::const_iterator it = _guiStack.find(sceneID);
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
        SharedLock r_lock(_guiStackLock);
        const GUIMapPerScene::const_iterator it = _guiStack.find(sceneID);
        if (it != std::cend(_guiStack)) {
            return it->second->getGUIElement(elementID);
        }
    } else {
        return GUIInterface::getGUIElementImpl(elementID, type);
    }

    return nullptr;
}
CEGUI::GUIContext& GUI::getCEGUIContext() noexcept {
    assert(_ceguiContext != nullptr);
    return *_ceguiContext;
}

const CEGUI::GUIContext& GUI::getCEGUIContext() const noexcept {
    assert(_ceguiContext != nullptr);
    return *_ceguiContext;
}

TextureData GUI::getCEGUIRenderTextureData() const {
    TextureData ret;

    if (_ceguiRenderTextureTarget != nullptr) {
        const GFXDevice& gfx = _context->parent().platformContext().gfx();
        ret.setHandle(gfx.getHandleFromCEGUITexture(_ceguiRenderTextureTarget->getTexture()));
        ret._textureType = TextureType::TEXTURE_2D;

    }

    return ret;
}
};
