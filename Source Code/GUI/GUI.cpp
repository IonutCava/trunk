#include <stdarg.h>

#include "CEGUI.h"
#include "Headers/GUI.h"
#include "Headers/GUIFlash.h"
#include "Headers/GUIText.h"
#include "Headers/GUIButton.h"
#include "Headers/GUIConsole.h"
#include "Core/Headers/ParamHandler.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"

void GUI::onResize(const vec2<U16>& newResolution){

	vec2<I32> difDimensions(_cachedResolution.width - newResolution.width,
							_cachedResolution.height - newResolution.height);

	for_each(guiMap::value_type& guiStackIterator,_guiStack){
		guiStackIterator.second->onResize(difDimensions);
	}
	_cachedResolution = newResolution;
}

void GUI::draw(){

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
	//CEGUI::System::getSingleton().renderGUI();
}

bool GUI::init(){
	return true;
	CEGUI::DefaultResourceProvider & defaultResProvider =* static_cast<CEGUI::DefaultResourceProvider*>( CEGUI::System::getSingleton().getResourceProvider() ) ;
	std::string CEGUIInstallSharePath = ParamHandler::getInstance().getParam<std::string>("assetsLocation");
	CEGUIInstallSharePath += "/GUI/" ;
	defaultResProvider.setResourceGroupDirectory( "schemes",    CEGUIInstallSharePath + "schemes/" ) ;
 	defaultResProvider.setResourceGroupDirectory( "imagesets",  CEGUIInstallSharePath + "imagesets/" ) ;
 	defaultResProvider.setResourceGroupDirectory( "fonts",      CEGUIInstallSharePath + "fonts/" ) ;
 	defaultResProvider.setResourceGroupDirectory( "layouts",    CEGUIInstallSharePath + "layouts/" ) ;
 	defaultResProvider.setResourceGroupDirectory( "looknfeels", CEGUIInstallSharePath + "looknfeel/" ) ;
 	defaultResProvider.setResourceGroupDirectory( "lua_scripts",CEGUIInstallSharePath + "lua_scripts/" ) ;
 	defaultResProvider.setResourceGroupDirectory( "schemas",    CEGUIInstallSharePath + "xml_schemas/" ) ;
 	defaultResProvider.setResourceGroupDirectory( "animations", CEGUIInstallSharePath + "animations/" ) ;
 
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
	CEGUI::SchemeManager::getSingleton().create( "TaharezLook.scheme" ) ;
	CEGUI::System::getSingleton().setDefaultMouseCursor( "TaharezLook", "MouseArrow" ) ;
	CEGUI::Window* myRoot = CEGUI::WindowManager::getSingleton().loadWindowLayout( "hwTestWindow.layout" );
	CEGUI::System::getSingleton().setGUISheet( myRoot );
	return true;
}

GUI::GUI(){

}

GUI::~GUI(){

	close();
}

void GUI::close(){
	if(_guiStack.find("console") != _guiStack.end()){
		GUIConsole::getInstance().DestroyInstance();
	}
	_guiStack.clear();
}

void GUI::createConsole() {
	PRINT_FN(Locale::get("CONSOLE_CREATE"));
	if(_guiStack.find("console") == _guiStack.end()){
		///Console is a default GUI element
		_guiStack.insert(std::make_pair("console",&GUIConsole::getInstance()));
	}
}

void GUI::toggleConsole(){

	GUIConsole::getInstance().toggleConsole();
}

void GUI::checkItem(U16 x, U16 y ){

	GUIEvent event;
	event.mousePoint.x = x;
	event.mousePoint.y = y;

	for_each(guiMap::value_type& guiStackIterator,_guiStack) {

		GUIElement* gui = guiStackIterator.second;
		switch(gui->getGuiType()){

			case GUI_BUTTON :
			{
				GUIButton *b = dynamic_cast<GUIButton*>(gui);
				b->onMouseMove(event);
				
			}break;
			case GUI_FLASH:
			{
				GUIFlash* f = dynamic_cast<GUIFlash*>(gui);
				f->onMouseMove(event);
			}break;
			case GUI_TEXT:
			{
				GUIText *t = dynamic_cast<GUIText*>(gui);
				t->onMouseMove(event);
			}break;
			default:
				break;
		}
	}
}

void GUI::clickCheck() {

	GUIEvent event;
	event.mouseClickCount = 0;

	for_each(guiMap::value_type& guiStackIterator,_guiStack) {

		GUIElement* gui = guiStackIterator.second;
		switch(gui->getGuiType()){

			case GUI_BUTTON :
			{
				GUIButton *b = dynamic_cast<GUIButton*>(gui);
				b->onMouseDown(event);
			}break;
			case GUI_FLASH:
			{
				GUIFlash* f = dynamic_cast<GUIFlash*>(gui);
				f->onMouseDown(event);
			}break;
			case GUI_TEXT:
			{
				GUIText *t = dynamic_cast<GUIText*>(gui);
				t->onMouseDown(event);
			}break;
			default:
				break;
		}
	}
}

void GUI::clickReleaseCheck() {
	GUIEvent event;
	event.mouseClickCount = 1;

	for_each(guiMap::value_type& guiStackIterator,_guiStack) {

		GUIElement* gui = guiStackIterator.second;
		switch(gui->getGuiType()){
			case GUI_BUTTON :
			{
				GUIButton *b = dynamic_cast<GUIButton*>(gui);
				b->onMouseUp(event);
			}break;
			case GUI_FLASH:
			{
				GUIFlash* f = dynamic_cast<GUIFlash*>(gui);
				f->onMouseUp(event);
			}break;
			case GUI_TEXT:
			{
				GUIText* t = dynamic_cast<GUIText*>(gui);
				t->onMouseUp(event);
			}break;
			default:
				break;
		}
	}
	
}

void GUI::addButton(const std::string& id, std::string text,const vec2<F32>& position,const vec2<F32>& dimensions,const vec3<F32>& color,ButtonCallback callback){

	_guiStack[id] = New GUIButton(id,text,position,dimensions,color,callback);
}

void GUI::addText(const std::string& id,const vec3<F32> &position, Font font,const vec3<F32> &color, char* format, ...){

	va_list args;
	std::string fmt_text;

    va_start(args, format);
    I32 len = _vscprintf(format, args) + 1;
    char *text = new char[len];
    vsprintf_s(text, len, format, args);
	fmt_text.append(text);
	SAFE_DELETE_ARRAY(text);
    va_end(args);

	GUIElement *t = New GUIText(id,fmt_text,position,(void*)font,color);
	_resultGuiElement = _guiStack.insert(make_pair(id,t));
	if(!_resultGuiElement.second) (_resultGuiElement.first)->second = t;
	fmt_text.empty();
}

void GUI::addFlash(const std::string& id, std::string movie, const vec2<F32>& position, const vec2<F32>& extent){

	GUIFlash *flash = New GUIFlash();
	_resultGuiElement = _guiStack.insert(make_pair(id,flash));
	if(!_resultGuiElement.second) (_resultGuiElement.first)->second = flash;
}

void GUI::modifyText(const std::string& id, char* format, ...){

	va_list args;   
	std::string fmt_text;

    va_start(args, format);
    I32 len = _vscprintf(format, args) + 1;
    char * text = new char[len];
    vsprintf_s(text, len, format, args);
	fmt_text.append(text);
	SAFE_DELETE_ARRAY(text);
    va_end(args);

	if(_guiStack[id]->getGuiType() == GUI_TEXT)
		dynamic_cast<GUIText*>(_guiStack[id])->_text = fmt_text;
	fmt_text.empty();
}

/*   void onRightMouseUp(const GuiEvent &event);
     void onRightMouseDown(const GuiEvent &event);
     bool onKeyUp(const GuiEvent &event);
     bool onKeyDown(const GuiEvent &event);
*/