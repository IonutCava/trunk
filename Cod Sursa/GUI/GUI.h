#ifndef GUI_H_
#define GUI_H_
#include "resource.h"
#include "Utility/Headers/Singleton.h"
#include <boost/function.hpp>

//typedef void (*ButtonCallback)();
typedef boost::function0<void> ButtonCallback;

class GuiControl
{
	friend class GUI;
public:
	GuiControl(){_name = "defaultGuiControl";}
	const string& getName()     const {return _name;}
	const vec2&   getPosition() const {return _position;}

	const bool isActive()  const {return _active;}
	const bool isVisible() const {return _visible;}

	void    setName(const string& name)    {_name = name;}
	void    setVisible(bool visible)       {_visible = visible;}
	void    setActive(bool active)         {_active = active;}
protected:
	vec2 _position;

private:
	string _name;
	bool   _visible;
	bool   _active;
};

class Text : public GuiControl
{
friend class GUI;
public:
	Text(string& id,string& text,const vec2& position, void* font,const vec3& color) :
	  _text(text),
	  _font(font),
	  _color(color){_position = position;};

	string _text;
	void* _font;
	vec3 _color;
};

class Button : public GuiControl
{

friend class GUI;
public:
	Button(string& id,string& text,const vec2& position,const vec2& dimensions,const vec3& color/*, Texture2D& image*/,ButtonCallback callback) :
		_text(text),
		_dimensions(dimensions),
		_color(color),
		_callbackFunction(callback)
		/*_image(image)*/{_position = position;_pressed = false; _highlight = false;};

	string _text;
	vec2 _dimensions;
	vec3 _color;
	bool _pressed;
	bool _highlight;
	ButtonCallback _callbackFunction;	/* A pointer to a function to call if the button is pressed */
	//Texture2D image;
};

class InputText : public GuiControl
{
friend class GUI;
public:
	InputText(){}
};

enum Font
{
	STROKE_ROMAN            =   0x0000,
    STROKE_MONO_ROMAN       =   0x0001,
    BITMAP_9_BY_15          =   0x0002,
    BITMAP_8_BY_13          =   0x0003,
    BITMAP_TIMES_ROMAN_10   =   0x0004,
    BITMAP_TIMES_ROMAN_24   =   0x0005,
    BITMAP_HELVETICA_10     =   0x0006,
    BITMAP_HELVETICA_12     =   0x0007,
    BITMAP_HELVETICA_18     =   0x0008
};


SINGLETON_BEGIN( GUI )

public:
	void draw();
	void close();
	void addText(string id,const vec3& position, Font font,const vec3& color, char* format, ...);
	void addButton(string id, string text,const vec2& position,const vec2& dimensions,const vec3& color,ButtonCallback callback);
	void modifyText(string id, char* format, ...);
	void resize(int newWidth, int newHeight);
	void clickCheck();
	void clickReleaseCheck();
	void checkItem(int x, int y);
	
	~GUI();

private:
	void drawText();
	void drawButtons();

	tr1::unordered_map<string,Text*> _text;
	tr1::unordered_map<string, Text*>::iterator _textIterator;

	tr1::unordered_map<string,Button*> _button;
	tr1::unordered_map<string, Button*>::iterator _buttonIterator;

	pair<tr1::unordered_map<string, Text*>::iterator, bool > _resultText;
	pair<tr1::unordered_map<string, Button*>::iterator, bool > _resultButton;
SINGLETON_END()
#endif