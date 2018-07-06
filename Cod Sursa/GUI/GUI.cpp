#include "GUI.h"
#include "Rendering/common.h"

void GUI::draw()
{
	GFXDevice::getInstance().enable_PROJECTION();
	GFXDevice::getInstance().pushMatrix();
	GFXDevice::getInstance().loadIdentityMatrix();
	gluOrtho2D(0, Engine::getInstance().getWindowWidth(), 0,Engine::getInstance().getWindowHeight());
	GFXDevice::getInstance().scale(1, -1, 1);
	GFXDevice::getInstance().translate(0, (F32)-Engine::getInstance().getWindowHeight(), 0);
	GFXDevice::getInstance().enable_MODELVIEW();

    //------------------------------------------------------------------------
		drawText();
		drawButtons();
	//------------------------------------------------------------------------

	GFXDevice::getInstance().enable_PROJECTION();
	GFXDevice::getInstance().popMatrix();
	GFXDevice::getInstance().enable_MODELVIEW();
}
GUI::~GUI()
{
	close();
}

void GUI::close()
{
	for(_textIterator = _text.begin(); _textIterator != _text.end(); _textIterator++) 
		delete (*_textIterator);
	_text.clear();
	for(_buttonIterator = _button.begin(); _buttonIterator != _button.end(); _buttonIterator++) 
		delete (*_buttonIterator);
	_button.clear();
}

void GUI::drawText()
{

	for(_textIterator = _text.begin(); _textIterator != _text.end(); _textIterator++)
	{
		
		GFXDevice::getInstance().pushMatrix();
		GFXDevice::getInstance().setColor((*_textIterator)->_color.x,(*_textIterator)->_color.y,(*_textIterator)->_color.z);
		GFXDevice::getInstance().loadIdentityMatrix();

		glRasterPos2f((*_textIterator)->_position.x,(*_textIterator)->_position.y);
		GFXDevice::getInstance().drawTextToScreen((*_textIterator)->_font, (*_textIterator)->_text);
		GFXDevice::getInstance().popMatrix();
	}
}

void GUI::drawButtons()
{
	for(_buttonIterator = _button.begin(); _buttonIterator != _button.end(); _buttonIterator++)
	{
		GFXDevice::getInstance().pushMatrix();
		GFXDevice::getInstance().setColor((*_buttonIterator)->_color.x,(*_buttonIterator)->_color.y,(*_buttonIterator)->_color.z);
		GFXDevice::getInstance().loadIdentityMatrix();
		glBegin( GL_LINE_LOOP );
			glVertex2f( 0, 0 );
			glVertex2f( (*_buttonIterator)->_dimensions.x, 0 );
			glVertex2f( (*_buttonIterator)->_dimensions.x, (*_buttonIterator)->_dimensions.y );  
			glVertex2f( 0, (*_buttonIterator)->_dimensions.y );
		glEnd();
	    glBegin( GL_LINE_LOOP );
			glVertex2f( 1, 1 );
			glVertex2f( (*_buttonIterator)->_dimensions.y-1, 1 );
			glVertex2f( (*_buttonIterator)->_dimensions.x-1, (*_buttonIterator)->_dimensions.y-1 );  
			glVertex2f( 1, (*_buttonIterator)->_dimensions.y-1 );
		glEnd();
		GFXDevice::getInstance().popMatrix();
	}
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

	_text.push_back(new Text(id,fmt_text,position,font,color));

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

	for(_textIterator = _text.begin(); _textIterator != _text.end(); _textIterator++)
		if((*_textIterator)->_id.compare(id) == 0)
		{
			(*_textIterator)->_text = fmt_text;
			return;
		}
	fmt_text.empty();
}