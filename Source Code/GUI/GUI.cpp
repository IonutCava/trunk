#include <stdarg.h>
#include "Headers/GUI.h"
#include <CEGUI/CEGUI.h>

#include "Headers/GUIFlash.h"
#include "Headers/GUIText.h"
#include "Headers/GUIButton.h"
#include "Headers/GUIConsole.h"
#include "Headers/GUIMessageBox.h"
#include "GUIEditor/Headers/GUIEditor.h"

#include "Scenes/Headers/Scene.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Input/Headers/InputInterface.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"

namespace Divide {

GUI::GUI() : _init(false),
             _rootSheet(nullptr),
             _console(New GUIConsole()),
             _textRenderInterval(getMsToUs(10))
{
    //500ms
    _ceguiInput.setInitialDelay(0.500f);
    GUIEditor::createInstance();
}

GUI::~GUI()
{
    PRINT_FN(Locale::get("STOP_GUI"));
    SAFE_DELETE(_console);
    RemoveResource(_guiShader);
    FOR_EACH(guiMap::value_type it, _guiStack) {
        SAFE_DELETE(it.second);
    }
    _guiStack.clear();
}

void GUI::onResize(const vec2<U16>& newResolution) {
    //CEGUI handles it's own init checks
    CEGUI::System::getSingleton().notifyDisplaySizeChanged(CEGUI::Sizef(newResolution.width, newResolution.height));

    if (!_init || _cachedResolution == newResolution) {
        return;
    }

    vec2<I32> difDimensions((I32)_cachedResolution.width - newResolution.width, (I32)_cachedResolution.height - newResolution.height);

    FOR_EACH(guiMap::value_type& guiStackIterator,_guiStack) {
        guiStackIterator.second->onResize(difDimensions);
    }

    _cachedResolution = newResolution;
}

void GUI::draw2D() {
    if (!_init) {
        return;
    }

    _guiShader->bind();

    GFXDevice& gfx = GFX_DEVICE;
    FOR_EACH(guiMap::value_type& guiStackIterator, _guiStack) {
        gfx.drawGUIElement(guiStackIterator.second);
    }
    const OIS::MouseState& mouseState = Input::InputInterface::getInstance().getMouse()->getMouseState();
    setCursorPosition(mouseState.X.abs, mouseState.Y.abs);
}

void GUI::update(const U64 deltaTime) {
    if (!_init) {
        return;
    }

    _ceguiInput.update(deltaTime);
    CEGUI::System::getSingleton().injectTimePulse(getUsToSec(deltaTime));
    CEGUI::System::getSingleton().getDefaultGUIContext().injectTimePulse(getUsToSec(deltaTime));

    if (_console) {
        _console->update(deltaTime);
    }

    GUIEditor::getInstance().update(deltaTime);
}

bool GUI::init(const vec2<U16>& resolution) {
    if (_init) {
        D_ERROR_FN(Locale::get("ERROR_GUI_DOUBLE_INIT"));
        return false;
    }
    _cachedResolution = resolution;

#ifdef _DEBUG
    CEGUI::Logger::getSingleton().setLoggingLevel(CEGUI::Informative);
#endif
    CEGUI::DefaultResourceProvider* rp = static_cast<CEGUI::DefaultResourceProvider*>( CEGUI::System::getSingleton().getResourceProvider() ) ;
    CEGUI::String CEGUIInstallSharePath(ParamHandler::getInstance().getParam<std::string>("assetsLocation"));
    CEGUIInstallSharePath += "/GUI/" ;
    rp->setResourceGroupDirectory( "schemes",    CEGUIInstallSharePath + "schemes/" ) ;
    rp->setResourceGroupDirectory( "imagesets",  CEGUIInstallSharePath + "imagesets/" ) ;
    rp->setResourceGroupDirectory( "fonts",      CEGUIInstallSharePath + "fonts/" ) ;
    rp->setResourceGroupDirectory( "layouts",    CEGUIInstallSharePath + "layouts/" ) ;
    rp->setResourceGroupDirectory( "looknfeels", CEGUIInstallSharePath + "looknfeel/" ) ;
    rp->setResourceGroupDirectory( "lua_scripts",CEGUIInstallSharePath + "lua_scripts/" ) ;
    rp->setResourceGroupDirectory( "schemas",    CEGUIInstallSharePath + "xml_schemas/" ) ;
    rp->setResourceGroupDirectory( "animations", CEGUIInstallSharePath + "animations/" ) ;

    // set the default resource groups to be used
    CEGUI::ImageManager::setImagesetDefaultResourceGroup("imagesets");
    CEGUI::Font::setDefaultResourceGroup("fonts");
    CEGUI::Scheme::setDefaultResourceGroup("schemes");
    CEGUI::WidgetLookManager::setDefaultResourceGroup("looknfeels");
    CEGUI::WindowManager::setDefaultResourceGroup("layouts");
    CEGUI::ScriptModule::setDefaultResourceGroup("lua_scripts");
    // setup default group for validation schemas
    CEGUI::XMLParser* parser = CEGUI::System::getSingleton().getXMLParser();
    if (parser->isPropertyPresent("SchemaDefaultResourceGroup")){
        parser->setProperty("SchemaDefaultResourceGroup", "schemas");
    }
    CEGUI::FontManager::getSingleton().createFromFile("DejaVuSans-10.font");
    CEGUI::FontManager::getSingleton().createFromFile("DejaVuSans-12.font");
    CEGUI::FontManager::getSingleton().createFromFile("DejaVuSans-10-NoScale.font");
    CEGUI::FontManager::getSingleton().createFromFile("DejaVuSans-12-NoScale.font");
	_defaultGUIScheme = ParamHandler::getInstance().getParam<stringImpl>("GUI.defaultScheme");
    CEGUI::SchemeManager::getSingleton().createFromFile(stringAlg::fromBase(_defaultGUIScheme + ".scheme")) ;

    _rootSheet = CEGUI::WindowManager::getSingleton().createWindow( "DefaultWindow","root_window");
    _rootSheet->setMousePassThroughEnabled(true);
    CEGUI_DEFAULT_CONTEXT.setRootWindow( _rootSheet );
    CEGUI_DEFAULT_CONTEXT.setDefaultTooltipType(stringAlg::fromBase(_defaultGUIScheme + "/Tooltip"));

    assert(_console);
    //_console->CreateCEGUIWindow();
    GUIEditor::getInstance().init();

    ResourceDescriptor immediateModeShader("ImmediateModeEmulation.GUI");
    immediateModeShader.setThreadedLoading(false);
    _guiShader = CreateResource<ShaderProgram>(immediateModeShader);

    GFX_DEVICE.add2DRenderFunction(DELEGATE_BIND(&GUI::draw2D, this), std::numeric_limits<U32>::max() - 1);
    const OIS::MouseState& mouseState = Input::InputInterface::getInstance().getMouse()->getMouseState();
    setCursorPosition(mouseState.X.abs, mouseState.Y.abs);
    _init = true;
    return true;
}

void GUI::selectionChangeCallback(Scene* const activeScene) {
    GUIEditor::getInstance().Handle_ChangeSelection(activeScene->getCurrentSelection());
}

void GUI::setCursorPosition(U16 x, U16 y) const {
    CEGUI_DEFAULT_CONTEXT.injectMousePosition(x, y);
}

bool GUI::onKeyDown(const Input::KeyEvent& key) {
    if (!_init) {
        return true;
    }

    if (_ceguiInput.onKeyDown(key) ) {
        return (!_console->isVisible() && !GUIEditor::getInstance().isVisible());
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

#   ifdef _DEBUG
        if (key._key == Input::KeyCode::KC_F11) {
            GUIEditor::getInstance().setVisible(!GUIEditor::getInstance().isVisible());
        }
#   endif

    if (_ceguiInput.onKeyUp(key)) {
        return (!_console->isVisible() && !GUIEditor::getInstance().isVisible());
    }

    return false;
}

bool GUI::mouseMoved(const Input::MouseEvent& arg) {
    if (!_init) { 
        return true;
    }

    GUIEvent event;
    event.mousePoint.x = arg.state.X.abs;
    event.mousePoint.y = arg.state.Y.abs;

    FOR_EACH(guiMap::value_type& guiStackIterator,_guiStack) {
        guiStackIterator.second->mouseMoved(event);
    }

    return _ceguiInput.mouseMoved(arg);
}

bool GUI::mouseButtonPressed(const Input::MouseEvent& arg, Input::MouseButton button) {
    if (!_init) {
        return true;
    }

    if (_ceguiInput.mouseButtonPressed(arg, button) ) {
        if (button == Input::MouseButton::MB_Left) {
            GUIEvent event;
            event.mouseClickCount = 0;
            FOR_EACH(guiMap::value_type& guiStackIterator,_guiStack) {
                guiStackIterator.second->onMouseDown(event);
            }
        }
    }

    return !_console->isVisible() && !GUIEditor::getInstance().wasControlClick();
}

bool GUI::mouseButtonReleased(const Input::MouseEvent& arg, Input::MouseButton button) {
    if (!_init) {
        return true;
    }
    if (_ceguiInput.mouseButtonReleased(arg, button) ) {
        if (button == Input::MouseButton::MB_Left) {
            GUIEvent event;
            event.mouseClickCount = 1;
            FOR_EACH(guiMap::value_type& guiStackIterator,_guiStack) {
                guiStackIterator.second->onMouseUp(event);
            }
        }
    }

    return !_console->isVisible() && !GUIEditor::getInstance().wasControlClick();
}

bool GUI::joystickAxisMoved(const Input::JoystickEvent& arg, I8 axis) {
    return _ceguiInput.joystickAxisMoved(arg, axis);
}

bool GUI::joystickPovMoved(const Input::JoystickEvent& arg, I8 pov){
    return _ceguiInput.joystickPovMoved(arg, pov);
}

bool GUI::joystickButtonPressed(const Input::JoystickEvent& arg, I8 button){
    return _ceguiInput.joystickButtonPressed(arg, button);
}

bool GUI::joystickButtonReleased(const Input::JoystickEvent& arg, I8 button){
    return _ceguiInput.joystickButtonReleased(arg, button);
}

bool GUI::joystickSliderMoved( const Input::JoystickEvent &arg, I8 index){
    return _ceguiInput.joystickSliderMoved(arg, index);
}

bool GUI::joystickVector3DMoved( const Input::JoystickEvent &arg, I8 index){
    return _ceguiInput.joystickVector3DMoved(arg, index);
}

GUIButton* GUI::addButton(const stringImpl& id, const stringImpl& text,
                          const vec2<I32>& position,const vec2<U32>& dimensions,const vec3<F32>& color,
                          ButtonCallback callback,const stringImpl& rootSheetId){
    CEGUI::Window* parent = nullptr;
    if (!rootSheetId.empty()) {
        parent = CEGUI_DEFAULT_CONTEXT.getRootWindow()->getChild(rootSheetId.c_str());
    }
    if (!parent) {
        parent = _rootSheet;
    }
    GUIButton* btn = New GUIButton(id, text, _defaultGUIScheme,position,dimensions,color,parent,callback);
    guiMap::iterator it = _guiStack.find(id);
    if (it != _guiStack.end()) {
        SAFE_UPDATE(it->second, btn);
    }else {
        hashAlg::insert(_guiStack, hashAlg::makePair(id,  btn));
    }
    
    return btn;
}

GUIMessageBox* GUI::addMsgBox(const stringImpl& id, const stringImpl& title, const stringImpl& message, const vec2<I32>& offsetFromCentre ) {
    GUIMessageBox* box = New GUIMessageBox(id, title, message, offsetFromCentre, _rootSheet);
    guiMap::iterator it = _guiStack.find(id);
    if (it != _guiStack.end()) {
        SAFE_UPDATE(it->second, box);
    }else {
        hashAlg::insert(_guiStack, hashAlg::makePair(id,  box));
    }
    
    return box;
}

GUIText* GUI::addText(const stringImpl& id,const vec2<I32> &position, const stringImpl& font,const vec3<F32> &color, char* format, ...){
    va_list args;
    stringImpl fmt_text;

    va_start(args, format);
    I32 len = _vscprintf(format, args) + 1;
    char *text = new char[len];
    vsprintf_s(text, len, format, args);
    fmt_text.append(text);
    delete [] text;
    va_end(args);

    GUIText *t = New GUIText(id,fmt_text,position,font,color,_rootSheet);
    guiMap::iterator it = _guiStack.find(id);
    if (it != _guiStack.end()) {
        SAFE_UPDATE(it->second, t);
    }else {
        hashAlg::insert(_guiStack, hashAlg::makePair(id, t));
    }

    fmt_text.empty();

    return t;
}

GUIFlash* GUI::addFlash(const stringImpl& id, stringImpl movie, const vec2<U32>& position, const vec2<U32>& extent){
    GUIFlash *flash = New GUIFlash(_rootSheet);
    guiMap::iterator it = _guiStack.find(id);
    if (it != _guiStack.end()) {
        SAFE_UPDATE(it->second, flash);
    }else {
        hashAlg::insert(_guiStack, hashAlg::makePair(id, flash));
    }
    return flash;
}

GUIText* GUI::modifyText(const stringImpl& id, char* format, ...){
    if(_guiStack.find(id) == _guiStack.end()) return nullptr;

    va_list args;
    stringImpl fmt_text;

    va_start(args, format);
    I32 len = _vscprintf(format, args) + 1;
    char * text = new char[len];
    vsprintf_s(text, len, format, args);
    fmt_text.append(text);
    delete [] text;
    va_end(args);

    GUIElement* element = _guiStack[id];
    assert(element->getType() == GUI_TEXT);

    GUIText* textElement = dynamic_cast<GUIText*>(element);
    textElement->_text = fmt_text;
    fmt_text.empty();
    return textElement;
}

};