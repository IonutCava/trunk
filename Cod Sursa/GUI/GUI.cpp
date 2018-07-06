#include "GUI.h"
#include <stdarg.h>
#include "Hardware/Video/GFXDevice.h"
#include "Rendering/common.h"

void GUI::resize(int newWidth, int newHeight)
{
	I32 difWidth = Engine::getInstance().getWindowWidth() - newWidth;
	I32 difHeight = Engine::getInstance().getWindowHeight() - newHeight;
	for(_textIterator = _text.begin(); _textIterator != _text.end(); _textIterator++)
	{
		(_textIterator->second)->_position.x -= difWidth;
		(_textIterator->second)->_position.y -= difHeight;
	}
	for(_buttonIterator = _button.begin(); _buttonIterator != _button.end(); _buttonIterator++)
	{
		(_buttonIterator->second)->_position.x -= difWidth;
		(_buttonIterator->second)->_position.y -= difHeight;
	}
}

void GUI::draw()
{
	GFXDevice::getInstance().toggle2D(true);
	
    //------------------------------------------------------------------------
		drawText();
		drawButtons();
	//------------------------------------------------------------------------

	GFXDevice::getInstance().toggle2D(false);
		
}

GUI::~GUI()
{
	close();
}

void GUI::close()
{
	_text.clear();
	_button.clear();
}

void GUI::drawText()
{
	for(_textIterator = _text.begin(); _textIterator != _text.end(); _textIterator++)
		GFXDevice::getInstance().drawTextToScreen((*_textIterator).second);
}

void GUI::drawButtons()
{
	for(_buttonIterator = _button.begin(); _buttonIterator != _button.end(); _buttonIterator++)
		GFXDevice::getInstance().drawButton((*_buttonIterator).second);
}

void GUI::checkItem(int x, int y)
{
	for(_buttonIterator = _button.begin(); _buttonIterator != _button.end(); _buttonIterator++)
	{
		Button *b = (*_buttonIterator).second;

	    if( x > b->_position.x   &&  x < b->_position.x+b->_dimensions.x &&	y > b->_position.y   &&  y < b->_position.y+b->_dimensions.y ) 
			b->_highlight = true;
    	else
			b->_highlight = false;
	}
}

void GUI::clickCheck()
{
	for(_buttonIterator = _button.begin(); _buttonIterator != _button.end(); _buttonIterator++)
	{
		Button *b = (*_buttonIterator).second;
		if (b->_highlight)
		{
			b->_pressed = true;
		}
		else b->_pressed = false;
	}
}

void GUI::clickReleaseCheck()
{
	for(_buttonIterator = _button.begin(); _buttonIterator != _button.end(); _buttonIterator++)
	{
		Button *b = (*_buttonIterator).second;
		if (b->_pressed)
		{
			if (b->_callbackFunction) {
				b->_callbackFunction();
			}
			b->_pressed = false;
			break;
		}
	}
	
}

void GUI::addButton(string id, string text,const vec2& position,const vec2& dimensions,const vec3& color,ButtonCallback callback)
{
	_button[id] = new Button(id,text,position,dimensions,color,callback);
}

void GUI::addText(string id,const vec3 &position, Font font,const vec3 &color, char* format, ...)
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

	Text *t = new Text(id,fmt_text,position,(void*)font,color);
	_resultText = _text.insert(pair<string,Text*>(id,t));
	if(!_resultText.second) (_resultText.first)->second = t;
	fmt_text.empty();
}

void GUI::modifyText(string id, char* format, ...)
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

	_text[id]->_text = fmt_text;
	fmt_text.empty();
}