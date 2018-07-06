#include <stdarg.h>
#include "Headers/GUI.h"

#include "CEGUI.h"

#include "Headers/GUIFlash.h"
#include "Headers/GUIText.h"
#include "Headers/GUIButton.h"
#include "Core/Headers/ParamHandler.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Input/Headers/InputInterface.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"

GUI::GUI() : _prevElapsedTime(0),
			 _init(false),
             _rootSheet(NULL),
			 _console(New GUIConsole())
{
    //500ms
    _input.setInitialDelay(0.500f);
	RenderStateBlockDescriptor CEGUIStateBlock;
	CEGUIStateBlock.setCullMode(CULL_MODE_NONE);
	CEGUIStateBlock._fixedLighting = false;
	//CEGUIStateBlock.setZReadWrite(false,false);
	CEGUIStateBlock.setFillMode(FILL_MODE_SOLID);
	CEGUIStateBlock.setZEnable(false);
	CEGUIStateBlock.setCullMode(CULL_MODE_NONE);
	_CEGUIStateBlock = GFX_DEVICE.createStateBlock(CEGUIStateBlock);

	
}

GUI::~GUI()
{
	close();
}

void GUI::onResize(const vec2<U16>& newResolution){
	//CEGUI handles it's own init checks
	CEGUI::System::getSingleton().notifyDisplaySizeChanged(CEGUI::Size(newResolution.width,newResolution.height));

	if(!_init) return;
	vec2<I32> difDimensions(_cachedResolution.width - newResolution.width,
							_cachedResolution.height - newResolution.height);

	for_each(guiMap::value_type& guiStackIterator,_guiStack){
		guiStackIterator.second->onResize(difDimensions);
	}
	_cachedResolution = newResolution;
}

void GUI::draw(U32 timeElapsed){
	if(!_init) return;
	GFXDevice& gfx = GFX_DEVICE;
	gfx.toggle2D(true);
    //------------------------------------------------------------------------
		for_each(guiMap::value_type& guiStackIterator,_guiStack){
			GUIElement* guiElement = guiStackIterator.second;
			if(!guiElement->isVisible()) continue;
			SET_STATE_BLOCK(guiElement->_guiSB);
			gfx.renderGUIElement(guiElement);
			
		}
	//------------------------------------------------------------------------
	gfx.toggle2D(false);
	
	if(timeElapsed == 0){
		timeElapsed = GETMSTIME();
	}
	if(_prevElapsedTime == 0){
		_prevElapsedTime = timeElapsed - 1;//<1 MS difference
	}
	U32 timeDiffInMs = timeElapsed - _prevElapsedTime;
	F32 timeElapsedSec = getMsToSec(timeDiffInMs);
	_input.update(timeElapsedSec);
	CEGUI::System::getSingleton().injectTimePulse(timeElapsedSec);
	SET_STATE_BLOCK(_CEGUIStateBlock);
	GFX_DEVICE.updateStates();
	CEGUI::System::getSingleton().renderGUI();
	_prevElapsedTime = timeElapsed;
}

bool GUI::init(){
	if(_init) {
		D_ERROR_FN(Locale::get("ERROR_GUI_DOUBLE_INIT"));
		return false;
	}
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
 	rp->setResourceGroupDirectory( "schemas",    CEGUIInstallSharePath + "xml_schemas/" ) ;
 	rp->setResourceGroupDirectory( "animations", CEGUIInstallSharePath + "animations/" ) ;
 
	// Sets the default resource groups to be used:
	CEGUI::Imageset::setDefaultResourceGroup( "imagesets" ) ;
	CEGUI::Font::setDefaultResourceGroup( "fonts" ) ;
	CEGUI::Scheme::setDefaultResourceGroup( "schemes" ) ;
	CEGUI::WidgetLookManager::setDefaultResourceGroup( "looknfeels" ) ;
	CEGUI::WindowManager::setDefaultResourceGroup( "layouts" ) ;
	CEGUI::ScriptModule::setDefaultResourceGroup( "lua_scripts" ) ;
	CEGUI::AnimationManager::setDefaultResourceGroup( "animations" ) ;
 
	// Set-up default group for validation schemas:
	CEGUI::XMLParser * parser = CEGUI::System::getSingleton().getXMLParser() ;
	if ( parser->isPropertyPresent( "SchemaDefaultResourceGroup" ) ){
		parser->setProperty( "SchemaDefaultResourceGroup", "schemas" ) ;
	}
	_defaultGUIScheme = ParamHandler::getInstance().getParam<std::string>("GUI.defaultScheme");
	CEGUI::SchemeManager::getSingleton().create(  _defaultGUIScheme + ".scheme") ;

	_rootSheet = CEGUI::WindowManager::getSingleton().createWindow( "DefaultWindow","root_window");
	CEGUI::System::getSingleton().setGUISheet( _rootSheet );
    CEGUI::System::getSingleton().setDefaultTooltip(_defaultGUIScheme + "/Tooltip" );
	assert(_console);
	_console->CreateCEGUIWindow();
	_init = true;
	return true;
}

void GUI::close(){
	for_each(guiMap::value_type it, _guiStack) {
		SAFE_DELETE(it.second);
	}
	_guiStack.clear();
	SAFE_DELETE(_console);
	CEGUI::System::destroy();
	SAFE_DELETE(_CEGUIStateBlock);
}

bool GUI::checkItem(const OIS::MouseEvent& arg ){
	if(!_init) return true;
	GUIEvent event;
	event.mousePoint.x = arg.state.X.abs;
	event.mousePoint.y = arg.state.Y.abs;

	for_each(guiMap::value_type& guiStackIterator,_guiStack) {
		guiStackIterator.second->onMouseMove(event);
	}
	CEGUI::System::getSingleton().injectMousePosition(arg.state.X.abs, arg.state.Y.abs);
	CEGUI::System::getSingleton().injectMouseWheelChange(arg.state.Z.abs);
	return true;
}

bool GUI::keyCheck(OIS::KeyEvent key, bool pressed) {
	if(!_init) return true;

	if(key.key == OIS::KC_GRAVE && !pressed){
		_console->setVisible(!_console->isVisible());
	}
	_input.injectOISKey(pressed,key);
	
	return !_console->isVisible();
}

bool GUI::clickCheck(OIS::MouseButtonID button, bool pressed) {
	if(!_init) return true;

	if(pressed){
		switch (button){
			case OIS::MB_Left:{
				CEGUI::System::getSingleton().injectMouseButtonDown(CEGUI::LeftButton);
				GUIEvent event;
				event.mouseClickCount = 0;
				for_each(guiMap::value_type& guiStackIterator,_guiStack) {
					guiStackIterator.second->onMouseDown(event);
				}
				}break;
			case OIS::MB_Middle:
				CEGUI::System::getSingleton().injectMouseButtonDown(CEGUI::MiddleButton);
				break;
			case OIS::MB_Right:
				CEGUI::System::getSingleton().injectMouseButtonDown(CEGUI::RightButton);
				break;
			case OIS::MB_Button3:
				CEGUI::System::getSingleton().injectMouseButtonDown(CEGUI::X1Button);
				break;
			case OIS::MB_Button4:
				CEGUI::System::getSingleton().injectMouseButtonDown(CEGUI::X2Button);
				break;
			default:	
				break;
 		}
	}else{
		switch (button){
			case OIS::MB_Left:{
				CEGUI::System::getSingleton().injectMouseButtonUp(CEGUI::LeftButton);
				GUIEvent event;
				event.mouseClickCount = 1;
				for_each(guiMap::value_type& guiStackIterator,_guiStack) {
					guiStackIterator.second->onMouseUp(event);
				}
				}break;
			case OIS::MB_Middle:
				CEGUI::System::getSingleton().injectMouseButtonUp(CEGUI::MiddleButton);
				break;
			case OIS::MB_Right:
				CEGUI::System::getSingleton().injectMouseButtonUp(CEGUI::RightButton);
				break;
			case OIS::MB_Button3:
				CEGUI::System::getSingleton().injectMouseButtonUp(CEGUI::X1Button);
				break;
			case OIS::MB_Button4:
				CEGUI::System::getSingleton().injectMouseButtonUp(CEGUI::X2Button);
				break;
			default:	
				break;
		}
	}
	return !_console->isVisible();
}

GUIElement* GUI::addButton(const std::string& id, const std::string& text,
                    const vec2<U32>& position,const vec2<U32>& dimensions,const vec3<F32>& color,
                    ButtonCallback callback,const std::string& rootSheetId){
    
    CEGUI::Window* parent = NULL;
    if(!rootSheetId.empty()){
        parent = CEGUI::WindowManager::getSingletonPtr()->getWindow(rootSheetId);
    }
    if(!parent) parent = _rootSheet;
    GUIButton* btn = New GUIButton(id,text,_defaultGUIScheme,position,dimensions,color,parent,callback); 
    _guiStack[id] = btn;
    return btn;
}

GUIElement* GUI::addText(const std::string& id,const vec2<U32> &position, const std::string& font,const vec3<F32> &color, char* format, ...){

	va_list args;
	std::string fmt_text;

    va_start(args, format);
    I32 len = _vscprintf(format, args) + 1;
    char *text = new char[len];
    vsprintf_s(text, len, format, args);
	fmt_text.append(text);
	SAFE_DELETE_ARRAY(text);
    va_end(args);

	GUIElement *t = New GUIText(id,fmt_text,position,font,color,_rootSheet);
	_resultGuiElement = _guiStack.insert(make_pair(id,t));
	if(!_resultGuiElement.second) (_resultGuiElement.first)->second = t;
	fmt_text.empty();

    return t;
}

GUIElement* GUI::addFlash(const std::string& id, std::string movie, const vec2<U32>& position, const vec2<U32>& extent){

	GUIFlash *flash = New GUIFlash(_rootSheet);
	_resultGuiElement = _guiStack.insert(make_pair(id,flash));
	if(!_resultGuiElement.second) (_resultGuiElement.first)->second = flash;

    return flash;
}

GUIElement* GUI::modifyText(const std::string& id, char* format, ...){
    if(_guiStack.find(id) == _guiStack.end()) return NULL;

	va_list args;   
	std::string fmt_text;

    va_start(args, format);
    I32 len = _vscprintf(format, args) + 1;
    char * text = new char[len];
    vsprintf_s(text, len, format, args);
	fmt_text.append(text);
	SAFE_DELETE_ARRAY(text);
    va_end(args);

    GUIElement* element = _guiStack[id];
	if(element->getGuiType() == GUI_TEXT)
		dynamic_cast<GUIText*>(element)->_text = fmt_text;
	fmt_text.empty();
    return element;
}

/*   void onRightMouseUp(const GuiEvent &event);
     void onRightMouseDown(const GuiEvent &event);
     bool onKeyUp(const GuiEvent &event);
     bool onKeyDown(const GuiEvent &event);
*/

bool GUI::bindRenderer(CEGUI::Renderer& renderer){
	CEGUI::System::create(renderer);
	return true;
}