#include "Headers/GUI.h"
#include "Headers/guiFlash.h"

#include <stdarg.h>
#include "Hardware/Video/GFXDevice.h"
#include "Core/Headers/Application.h"

using namespace std;

void GUI::onResize(F32 newWidth, F32 newHeight){

	vec2 difDimensions = Application::getInstance().getWindowDimensions() - vec2(newWidth,newHeight);
	for_each(guiMap::value_type& guiStackIterator,_guiStack){
		guiStackIterator.second->onResize(difDimensions);
	}
}

void GUI::draw(){

	GFXDevice& gfx = GFXDevice::getInstance();
	gfx.toggle2D(true);
    //------------------------------------------------------------------------
		for_each(guiMap::value_type& guiStackIterator,_guiStack){
			GuiElement* guiElement = guiStackIterator.second;
			if(!guiElement->isVisible()) continue;
			switch(guiElement->getGuiType())
			{
				case GUI_TEXT:
					gfx.drawTextToScreen(dynamic_cast<Text*>(guiElement));
					break;
				case GUI_BUTTON:
					gfx.drawButton(dynamic_cast<Button*>(guiElement));
					break;
				case GUI_FLASH:
					gfx.drawFlash(dynamic_cast<GuiFlash*>(guiElement));
					break;
				default:
					break;
			};
			
		}
	//------------------------------------------------------------------------

	gfx.toggle2D(false);
		
}

GUI::~GUI(){

	close();
}

void GUI::close(){

	_guiStack.clear();
}

void GUI::checkItem(U16 x, U16 y){

	GuiEvent event;
	event.mousePoint.x = x;
	event.mousePoint.y = y;

	for_each(guiMap::value_type& guiStackIterator,_guiStack){

		GuiElement* gui = guiStackIterator.second;
		switch(gui->getGuiType()){

			case GUI_BUTTON :
			{
				Button *b = dynamic_cast<Button*>(gui);
				b->onMouseMove(event);
				
			}break;
			case GUI_FLASH:
			{
				GuiFlash* f = dynamic_cast<GuiFlash*>(gui);
				f->onMouseMove(event);
			}break;
			case GUI_TEXT:
			{
				Text *t = dynamic_cast<Text*>(gui);
				t->onMouseMove(event);
			}break;
			default:
				break;
		}
	}
}

void GUI::clickCheck(){

	GuiEvent event;
	event.mouseClickCount = 0;

	for_each(guiMap::value_type& guiStackIterator,_guiStack){

		GuiElement* gui = guiStackIterator.second;
		switch(gui->getGuiType()){

			case GUI_BUTTON :
			{
				Button *b = dynamic_cast<Button*>(gui);
				b->onMouseDown(event);
			}break;
			case GUI_FLASH:
			{
				GuiFlash* f = dynamic_cast<GuiFlash*>(gui);
				f->onMouseDown(event);
			}break;
			case GUI_TEXT:
			{
				Text *t = dynamic_cast<Text*>(gui);
				t->onMouseDown(event);
			}break;
			default:
				break;
		}
	}
}

void GUI::clickReleaseCheck()
{
	GuiEvent event;
	event.mouseClickCount = 1;

	for_each(guiMap::value_type& guiStackIterator,_guiStack){

		GuiElement* gui = guiStackIterator.second;
		switch(gui->getGuiType()){
			case GUI_BUTTON :
			{
				Button *b = dynamic_cast<Button*>(gui);
				b->onMouseUp(event);
			}break;
			case GUI_FLASH:
			{
				GuiFlash* f = dynamic_cast<GuiFlash*>(gui);
				f->onMouseUp(event);
			}break;
			case GUI_TEXT:
			{
				Text* t = dynamic_cast<Text*>(gui);
				t->onMouseUp(event);
			}break;
			default:
				break;
		}
	}
	
}

void GUI::addButton(const string& id, string text,const vec2& position,const vec2& dimensions,const vec3& color,ButtonCallback callback){

	_guiStack[id] = New Button(id,text,position,dimensions,color,callback);
}

void GUI::addText(const string& id,const vec3 &position, Font font,const vec3 &color, char* format, ...){

	va_list args;
	string fmt_text;

    va_start(args, format);
    I32 len = _vscprintf(format, args) + 1;
    char *text = new char[len];
    vsprintf_s(text, len, format, args);
	fmt_text.append(text);
	delete[] text;
	text = NULL;
    va_end(args);

	GuiElement *t = New Text(id,fmt_text,position,(void*)font,color);
	_resultGuiElement = _guiStack.insert(make_pair(id,t));
	if(!_resultGuiElement.second) (_resultGuiElement.first)->second = t;
	fmt_text.empty();
}

void GUI::addFlash(const string& id, string movie, const vec2& position, const vec2& extent){

	GuiFlash *flash = New GuiFlash();
	_resultGuiElement = _guiStack.insert(make_pair(id,dynamic_cast<GuiElement*>(flash)));
	if(!_resultGuiElement.second) (_resultGuiElement.first)->second = dynamic_cast<GuiElement*>(flash);
}

void GUI::modifyText(const string& id, char* format, ...){

	va_list args;   
	string fmt_text;

    va_start(args, format);
    I32 len = _vscprintf(format, args) + 1;
    char * text = new char[len];
    vsprintf_s(text, len, format, args);
	fmt_text.append(text);
	delete[] text;
	text = NULL;
    va_end(args);

	if(_guiStack[id]->getGuiType() == GUI_TEXT)
		dynamic_cast<Text*>(_guiStack[id])->_text = fmt_text;
	fmt_text.empty();
}


void Button::onMouseMove(const GuiEvent &event){

	if(event.mousePoint.x > _position.x   &&  event.mousePoint.x < _position.x+_dimensions.x &&
	   event.mousePoint.y > _position.y   &&  event.mousePoint.y < _position.y+_dimensions.y )	{
		if(isActive()) _highlight = true;
	}else{
		_highlight = false;
	}
}

void Button::onMouseUp(const GuiEvent &event){

	if (_pressed){
		if (_callbackFunction) 	_callbackFunction();
		_pressed = false;
	}
}

void Button::onMouseDown(const GuiEvent &event){

	if (_highlight) _pressed = true;
	else _pressed = false;
}

void Text::onMouseMove(const GuiEvent &event){

}

void Text::onMouseUp(const GuiEvent &event){

}

void Text::onMouseDown(const GuiEvent &event){

}

/*   void onRightMouseUp(const GuiEvent &event);
     void onRightMouseDown(const GuiEvent &event);
     bool onKeyUp(const GuiEvent &event);
     bool onKeyDown(const GuiEvent &event);
*/