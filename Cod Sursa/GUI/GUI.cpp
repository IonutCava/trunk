#include "GUI.h"
#include "Hardware/Video/GFXDevice.h"

bool ButtonClickTest(Button* b,int x,int y) 
{
	if( b) 
	{
		if( x > b->_position.x      && 
			x < b->_position.x+b->_dimensions.x &&
			y > b->_position.y      && 
			y < b->_position.y+b->_dimensions.y ) {
				return true;
		}
	}
	return false;
}

void GUI::draw()
{
	GFXDevice::getInstance().loadOrtographicView();
	
    //------------------------------------------------------------------------
		drawText();
		drawButtons();
	//------------------------------------------------------------------------

	GFXDevice::getInstance().loadModelView();
		
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

void GUI::addButton(string id, string text, vec2& position, vec2& dimensions, vec3& color,ButtonCallback callback)
{
	_button[id] = new Button(id,text,position,dimensions,color,callback);
}

void GUI::addText(string id, vec3 &position, void *font, vec3 &color, char* format, ...)
{
	va_list args;
	string fmt_text;

    va_start(args, format);
    int len = _vscprintf(format, args) + 1;
    char *text = new char[len];
    vsprintf_s(text, len, format, args);
	fmt_text.append(text);
	delete[] text;
    va_end(args);

	Text *t = new Text(id,fmt_text,position,font,color);
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
    va_end(args);

	_text[id]->_text = fmt_text;
	fmt_text.empty();
}