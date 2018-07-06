#include "GUI.h"
#include <stdarg.h>
#include "Hardware/Video/GFXDevice.h"
#include "Rendering/common.h"
#include "guiFlash.h"

void GUI::resize(I32 newWidth, I32 newHeight)
{
	I32 difWidth = Engine::getInstance().getWindowWidth() - newWidth;
	I32 difHeight = Engine::getInstance().getWindowHeight() - newHeight;
	for(_guiStackIterator = _guiStack.begin(); _guiStackIterator != _guiStack.end(); _guiStackIterator++)
	{
		(_guiStackIterator->second)->_position.x -= difWidth;
		(_guiStackIterator->second)->_position.y -= difHeight;
	}
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
	for(_guiStackIterator = _guiStack.begin(); _guiStackIterator != _guiStack.end(); _guiStackIterator++)
	{
		GuiElement* _gui = (*_guiStackIterator).second;
		switch(_gui->getGuiType())
		{
			case GUI_BUTTON :
			{
				Button *b = dynamic_cast<Button*>(_gui);
				if( x > b->_position.x   &&  x < b->_position.x+b->_dimensions.x &&	y > b->_position.y   &&  y < b->_position.y+b->_dimensions.y ) 
				{
					if(b->isActive()) b->_highlight = true;
				}
	    		else
					b->_highlight = false;
				
			}break;
			case GUI_TEXT:
			default:
				break;
		}
	}
}

void GUI::clickCheck()
{
	for(_guiStackIterator = _guiStack.begin(); _guiStackIterator != _guiStack.end(); _guiStackIterator++)
	{
		GuiElement* _gui = (*_guiStackIterator).second;
		switch(_gui->getGuiType())
		{
			case GUI_BUTTON :
			{
				Button *b = dynamic_cast<Button*>(_gui);
				if (b->_highlight)
					b->_pressed = true;
				else
					b->_pressed = false;
			}break;
			case GUI_TEXT:
			default:
				break;
		}
	}
}

void GUI::clickReleaseCheck()
{
	for(_guiStackIterator = _guiStack.begin(); _guiStackIterator != _guiStack.end(); _guiStackIterator++)
	{
		GuiElement* _gui = (*_guiStackIterator).second;
		switch(_gui->getGuiType())
		{
			case GUI_BUTTON :
			{
				Button *b = dynamic_cast<Button*>(_gui);
				if (b->_pressed)
				{
					if (b->_callbackFunction) 
						b->_callbackFunction();
					b->_pressed = false;
				}
			}break;
			case GUI_TEXT:
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
	flash->loadMovie(movie,position,extent);
	flash->setLooping(true);
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