#include <stdarg.h>
#include "Headers/GUI.h"
#include <CEGUI/CEGUI.h>

#include "Headers/GUIFlash.h"
#include "Headers/GUIText.h"
#include "Headers/GUIButton.h"
#include "Headers/GUIConsole.h"
#include "GUIEditor/Headers/GUIEditor.h"

#include "Scenes/Headers/Scene.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Input/Headers/InputInterface.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"

GUI::GUI() : _init(false),
             _rootSheet(nullptr),
             _console(New GUIConsole()),
             _textRenderInterval(getMsToUs(10))
{
    //500ms
    _input.setInitialDelay(0.500f);
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
}

void GUI::update(const U64 deltaTime) {
    if (!_init) {
        return;
    }

    if (_console) {
        _console->update(deltaTime);
    }

    _input.update(deltaTime);

    CEGUI::System::getSingleton().injectTimePulse(getUsToSec(deltaTime));
    GUIEditor::getInstance().update(deltaTime);
}

bool GUI::init(const vec2<U16>& resolution) {
    if (_init) {
        D_ERROR_FN(Locale::get("ERROR_GUI_DOUBLE_INIT"));
        return false;
    }
    _cachedResolution = resolution;

#ifdef _DEBUG
    CEGUI::Logger::getSingleton().setLoggingLevel(CEGUI::Insane);
#endif
    CEGUI::DefaultResourceProvider* rp = static_cast<CEGUI::DefaultResourceProvider*>( CEGUI::System::getSingleton().getResourceProvider() ) ;
    std::string CEGUIInstallSharePath = ParamHandler::getInstance().getParam<std::string>("assetsLocation");
    CEGUIInstallSharePath += "/GUI/" ;
    rp->setResourceGroupDirectory( "schemes",    CEGUIInstallSharePath + "schemes/" ) ;
    rp->setResourceGroupDirectory( "imagesets",  CEGUIInstallSharePath + "imagesets/" ) ;
    rp->setResourceGroupDirectory( "fonts",      CEGUIInstallSharePath + "fonts/" ) ;
    rp->setResourceGroupDirectory( "layouts",    CEGUIInstallSharePath + "layouts/" ) ;
    rp->setResourceGroupDirectory( "looknfeels", CEGUIInstallSharePath + "looknfeel/" ) ;
    rp->setResourceGroupDirectory( "lua_scripts",CEGUIInstallSharePath + "lua_scripts/" ) ;
    //rp->setResourceGroupDirectory( "schemas",    CEGUIInstallSharePath + "xml_schemas/" ) ;
    //rp->setResourceGroupDirectory( "animations", CEGUIInstallSharePath + "animations/" ) ;

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
    CEGUI::FontManager::getSingleton().createFromFile("DejaVuSans-12.font");
    _defaultGUIScheme = ParamHandler::getInstance().getParam<std::string>("GUI.defaultScheme");
    CEGUI::SchemeManager::getSingleton().createFromFile(  _defaultGUIScheme + ".scheme") ;

    _rootSheet = CEGUI::WindowManager::getSingleton().createWindow( "DefaultWindow","root_window");
    CEGUI_DEFAULT_CONTEXT.setRootWindow( _rootSheet );
    CEGUI_DEFAULT_CONTEXT.setDefaultTooltipType( _defaultGUIScheme + "/Tooltip" );

    assert(_console);
    //_console->CreateCEGUIWindow();
    GUIEditor::getInstance().init();

    ResourceDescriptor immediateModeShader("ImmediateModeEmulation.GUI");
    immediateModeShader.setThreadedLoading(false);
    _guiShader = CreateResource<ShaderProgram>(immediateModeShader);

    GFX_DEVICE.add2DRenderFunction(DELEGATE_BIND(&GUI::draw2D, this), std::numeric_limits<U32>::max() - 1);

    _init = true;
    return true;
}

void GUI::selectionChangeCallback(Scene* const activeScene) {
    GUIEditor::getInstance().Handle_ChangeSelection(activeScene->getCurrentSelection());
}

bool GUI::checkItem(const OIS::MouseEvent& arg) {
    if (!_init) { 
        return true;
    }

    GUIEvent event;
    event.mousePoint.x = arg.state.X.abs;
    event.mousePoint.y = arg.state.Y.abs;

    FOR_EACH(guiMap::value_type& guiStackIterator,_guiStack) {
        guiStackIterator.second->onMouseMove(event);
    }

    CEGUI_DEFAULT_CONTEXT.injectMousePosition(arg.state.X.abs, arg.state.Y.abs);
    CEGUI_DEFAULT_CONTEXT.injectMouseWheelChange(arg.state.Z.abs);

    return true;
}

bool GUI::keyCheck(OIS::KeyEvent key, bool pressed) {
    if (!_init) {
        return true;
    }

    if (key.key == OIS::KC_GRAVE && !pressed) {
        _console->setVisible(!_console->isVisible());
    }

#   ifdef _DEBUG
        if (key.key == OIS::KC_F11 && !pressed) {
            GUIEditor::getInstance().setVisible(!GUIEditor::getInstance().isVisible());
        }
#   endif
    _input.injectOISKey(pressed,key);

    return (!_console->isVisible() && !GUIEditor::getInstance().isVisible());
}

bool GUI::clickCheck(OIS::MouseButtonID button, bool pressed) {
    if (!_init) {
        return true;
    }

    if (pressed) {
        switch (button) {
            case OIS::MB_Left: {
                CEGUI_DEFAULT_CONTEXT.injectMouseButtonDown(CEGUI::LeftButton);
                GUIEvent event;
                event.mouseClickCount = 0;
                FOR_EACH(guiMap::value_type& guiStackIterator,_guiStack) {
                    guiStackIterator.second->onMouseDown(event);
                }
                }break;
            case OIS::MB_Middle:
                CEGUI_DEFAULT_CONTEXT.injectMouseButtonDown(CEGUI::MiddleButton);
                break;
            case OIS::MB_Right:
                CEGUI_DEFAULT_CONTEXT.injectMouseButtonDown(CEGUI::RightButton);
                break;
            case OIS::MB_Button3:
                CEGUI_DEFAULT_CONTEXT.injectMouseButtonDown(CEGUI::X1Button);
                break;
            case OIS::MB_Button4:
                CEGUI_DEFAULT_CONTEXT.injectMouseButtonDown(CEGUI::X2Button);
                break;
            default:
                break;
        }
    } else {
        switch (button) {
            case OIS::MB_Left: {
                CEGUI_DEFAULT_CONTEXT.injectMouseButtonUp(CEGUI::LeftButton);
                GUIEvent event;
                event.mouseClickCount = 1;
                FOR_EACH(guiMap::value_type& guiStackIterator,_guiStack) {
                    guiStackIterator.second->onMouseUp(event);
                }
                }break;
            case OIS::MB_Middle:
                CEGUI_DEFAULT_CONTEXT.injectMouseButtonUp(CEGUI::MiddleButton);
                break;
            case OIS::MB_Right:
                CEGUI_DEFAULT_CONTEXT.injectMouseButtonUp(CEGUI::RightButton);
                break;
            case OIS::MB_Button3:
                CEGUI_DEFAULT_CONTEXT.injectMouseButtonUp(CEGUI::X1Button);
                break;
            case OIS::MB_Button4:
                CEGUI_DEFAULT_CONTEXT.injectMouseButtonUp(CEGUI::X2Button);
                break;
            default:
                break;
        }
    }
    return !_console->isVisible() && !GUIEditor::getInstance().wasControlClick();
}

GUIButton* GUI::addButton(const std::string& id, const std::string& text,
                    const vec2<I32>& position,const vec2<U32>& dimensions,const vec3<F32>& color,
                    ButtonCallback callback,const std::string& rootSheetId){
    CEGUI::Window* parent = nullptr;
    if (!rootSheetId.empty()) {
        parent = CEGUI_DEFAULT_CONTEXT.getRootWindow()->getChild(rootSheetId);
    }
    if(!parent) parent = _rootSheet;
    GUIButton* btn = New GUIButton(id,text,_defaultGUIScheme,position,dimensions,color,parent,callback);
    _guiStack[id] = btn;
    return btn;
}

GUIText* GUI::addText(const std::string& id,const vec2<I32> &position, const std::string& font,const vec3<F32> &color, char* format, ...){
    va_list args;
    std::string fmt_text;

    va_start(args, format);
    I32 len = _vscprintf(format, args) + 1;
    char *text = new char[len];
    vsprintf_s(text, len, format, args);
    fmt_text.append(text);
    delete [] text;
    va_end(args);

    GUIText *t = New GUIText(id,fmt_text,position,font,color,_rootSheet);
    _resultGuiElement = _guiStack.insert(make_pair(id,t));
    if(!_resultGuiElement.second) (_resultGuiElement.first)->second = t;
    fmt_text.empty();

    return t;
}

GUIFlash* GUI::addFlash(const std::string& id, std::string movie, const vec2<U32>& position, const vec2<U32>& extent){
    GUIFlash *flash = New GUIFlash(_rootSheet);
    _resultGuiElement = _guiStack.insert(make_pair(id,flash));
    if(!_resultGuiElement.second) (_resultGuiElement.first)->second = flash;

    return flash;
}

GUIText* GUI::modifyText(const std::string& id, char* format, ...){
    if(_guiStack.find(id) == _guiStack.end()) return nullptr;

    va_list args;
    std::string fmt_text;

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

/*   void onRightMouseUp(const GuiEvent &event);
     void onRightMouseDown(const GuiEvent &event);
     bool onKeyUp(const GuiEvent &event);
     bool onKeyDown(const GuiEvent &event);
*/
