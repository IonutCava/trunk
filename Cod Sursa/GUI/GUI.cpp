#include "GUI.h"
#include <stdarg.h>
#include "Hardware/Video/GFXDevice.h"
#include "Rendering/common.h"
#include "guiFlash.h"
using namespace std;

void GUI::onResize(F32 newWidth, F32 newHeight)
{
	vec2 difDimensions = Engine::getInstance().getWindowDimensions() - vec2(newWidth,newHeight);
	for(_guiStackIterator = _guiStack.begin(); _guiStackIterator != _guiStack.end(); _guiStackIterator++)
		(_guiStackIterator->second)->onResize(difDimensions);
}

void GUI::draw()
{
	GFXDevice::getInstance().toggle2D(true);
	
    //------------------------------------------------------------------------
		for(_guiStackIterator = _guiStack.begin(); _guiStackIterator != _guiStack.end(); _guiStackIterator++)
		{
			GuiElement* _guiElement = (*_guiStackIterator).second;
			switch(_guiElement->getGuiType())
			{
				case GUI_TEXT:
					GFXDevice::getInstance().drawTextToScreen(dynamic_cast<Text*>(_guiElement));
					break;
				case GUI_BUTTON:
					GFXDevice::getInstance().drawButton(dynamic_cast<Button*>(_guiElement));
					break;
				case GUI_FLASH:
					GFXDevice::getInstance().drawFlash(dynamic_cast<GuiFlash*>(_guiElement));
					break;
				default:
					break;
			};
			
		}
	//------------------------------------------------------------------------

	GFXDevice::getInstance().toggle2D(false);
		
}

GUI::~GUI()
{
	close();
}

void GUI::close()
{
	_guiStack.clear();
}

void GUI::checkItem(int x, int y)
{
	GuiEvent event;
	event.mousePoint.x = x;
	event.mousePoint.y = y;
	for(_guiStackIterator = _guiStack.begin(); _guiStackIterator != _guiStack.end(); _guiStackIterator++)
	{
		GuiElement* _gui = (*_guiStackIterator).second;
		switch(_gui->getGuiType())
		{
			case GUI_BUTTON :
			{
				Button *b = dynamic_cast<Button*>(_gui);
				b->onMouseMove(event);
				
			}break;
			case GUI_FLASH:
			{
				GuiFlash* f = dynamic_cast<GuiFlash*>(_gui);
				f->onMouseMove(event);
			}break;
			case GUI_TEXT:
			{
				Text *t = dynamic_cast<Text*>(_gui);
				t->onMouseMove(event);
			}break;
			default:
				break;
		}
	}
}

void GUI::clickCheck()
{
	GuiEvent event;
	event.mouseClickCount = 0;
	for(_guiStackIterator = _guiStack.begin(); _guiStackIterator != _guiStack.end(); _guiStackIterator++)
	{
		GuiElement* _gui = (*_guiStackIterator).second;
		switch(_gui->getGuiType())
		{
			case GUI_BUTTON :
			{
				Button *b = dynamic_cast<Button*>(_gui);
				b->onMouseDown(event);
			}break;
			case GUI_FLASH:
			{
				GuiFlash* f = dynamic_cast<GuiFlash*>(_gui);
				f->onMouseDown(event);
			}break;
			case GUI_TEXT:
			{
				Text *t = dynamic_cast<Text*>(_gui);
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
	for(_guiStackIterator = _guiStack.begin(); _guiStackIterator != _guiStack.end(); _guiStackIterator++)
	{
		GuiElement* _gui = (*_guiStackIterator).second;
		switch(_gui->getGuiType())
		{
			case GUI_BUTTON :
			{
				Button *b = dynamic_cast<Button*>(_gui);
				b->onMouseUp(event);
			}break;
			case GUI_FLASH:
			{
				GuiFlash* f = dynamic_cast<GuiFlash*>(_gui);
				f->onMouseUp(event);
			}break;
			case GUI_TEXT:
			{
				Text* t = dynamic_cast<Text*>(_gui);
				t->onMouseUp(event);
			}break;
			default:
				break;
		}
	}
	
}

void GUI::addButton(const string& id, string text,const vec2& position,const vec2& dimensions,const vec3& color,ButtonCallback callback)
{
	_guiStack[id] = new Button(id,text,position,dimensions,color,callback);
}

void GUI::addText(const string& id,const vec3 &position, Font font,const vec3 &color, char* format, ...)
{
	va_list args;
	string fmt_text;

    va_start(args, format);
    int len = _vscprintf(format, args) + 1;
    char *text = new char[len];
    vsprintf_s(text, len, format, args);
	fmt_text.append(text);
	delete[] text;
	text = NULL;
    va_end(args);

	GuiElement *t = new Text(id,fmt_text,position,(void*)font,color);
	_resultGuiElement = _guiStack.insert(pair<string,GuiElement*>(id,t));
	if(!_resultGuiElement.second) (_resultGuiElement.first)->second = t;
	fmt_text.empty();
}

void GUI::addFlash(const string& id, string movie, const vec2& position, const vec2& extent)
{
	GuiFlash *flash = new GuiFlash();
	_resultGuiElement = _guiStack.insert(pair<string,GuiElement*>(id,dynamic_cast<GuiElement*>(flash)));
	if(!_resultGuiElement.second) (_resultGuiElement.first)->second = dynamic_cast<GuiElement*>(flash);
}

void GUI::modifyText(const string& id, char* format, ...)
{
	va_list args;   
	string fmt_text;

    va_start(args, format);
    int len = _vscprintf(format, args) + 1;
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


void Button::onMouseMove(const GuiEvent &event)
{
	if(event.mousePoint.x > _position.x   &&  event.mousePoint.x < _position.x+_dimensions.x &&
	   event.mousePoint.y > _position.y   &&  event.mousePoint.y < _position.y+_dimensions.y )	{
		if(isActive()) _highlight = true;
	}
	else
		_highlight = false;
}

void Button::onMouseUp(const GuiEvent &event)
{
	if (_pressed){
		if (_callbackFunction) 	_callbackFunction();
		_pressed = false;
	}
}

void Button::onMouseDown(const GuiEvent &event)
{
	if (_highlight) _pressed = true;
	else _pressed = false;
}

void Text::onMouseMove(const GuiEvent &event)
{

}

void Text::onMouseUp(const GuiEvent &event)
{

}

void Text::onMouseDown(const GuiEvent &event)
{
}

/*   void onRightMouseUp(const GuiEvent &event);
     void onRightMouseDown(const GuiEvent &event);
     bool onKeyUp(const GuiEvent &event);
     bool onKeyDown(const GuiEvent &event);
*/