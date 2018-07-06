#include <stdarg.h>

#include "Headers/GUI.h"
#include "Headers/GUIFlash.h"
#include "Headers/GUIText.h"
#include "Headers/GUIButton.h"
#include "Headers/GUIConsole.h"
#include "Hardware/Video/GFXDevice.h"
#include "Core/Headers/Application.h"
#include "Hardware/Video/RenderStateBlock.h"

using namespace std;


void GUI::onResize(F32 newWidth, F32 newHeight){

	vec2<F32> difDimensions = Application::getInstance().getWindowDimensions() - vec2<F32>(newWidth,newHeight);
	for_each(guiMap::value_type& guiStackIterator,_guiStack){
		guiStackIterator.second->onResize(difDimensions);
	}
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
		
}

GUI::GUI(){
}

GUI::~GUI(){

	close();
}

void GUI::close(){

	_guiStack.clear();
}

void GUI::createConsole() {
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

void GUI::addButton(const string& id, string text,const vec2<F32>& position,const vec2<F32>& dimensions,const vec3<F32>& color,ButtonCallback callback){

	_guiStack[id] = New GUIButton(id,text,position,dimensions,color,callback);
}

void GUI::addText(const string& id,const vec3<F32> &position, Font font,const vec3<F32> &color, char* format, ...){

	va_list args;
	string fmt_text;

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

void GUI::addFlash(const string& id, string movie, const vec2<F32>& position, const vec2<F32>& extent){

	GUIFlash *flash = New GUIFlash();
	_resultGuiElement = _guiStack.insert(make_pair(id,flash));
	if(!_resultGuiElement.second) (_resultGuiElement.first)->second = flash;
}

void GUI::modifyText(const string& id, char* format, ...){

	va_list args;   
	string fmt_text;

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