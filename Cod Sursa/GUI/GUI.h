#ifndef GUI_H_
#define GUI_H_
#include "resource.h"
#include <unordered_map>
#include "Utility/Headers/Singleton.h"
#include <boost/function.hpp>

//typedef void (*ButtonCallback)();
typedef boost::function0<void> ButtonCallback;


class Text 
{
friend class GUI;
public:
	Text(string& id,string& text,vec3& position, void* font, vec3& color) :
	  _text(text),
	  _position(position),
	  _font(font),
	  _color(color){};

	string _text;
	vec3 _position;
	void* _font;
	vec3 _color;
};

class Button
{

friend class GUI;
public:
	Button(string& id,string& text,vec2& position, vec2& dimensions, vec3& color/*, Texture2D& image*/,ButtonCallback callback) :
		_text(text),
		_position(position),
		_dimensions(dimensions),
		_color(color),
		_callbackFunction(callback)
		/*_image(image)*/{_pressed = false; _highlight = false;};

	string _text;
	vec2 _position;
	vec2 _dimensions;
	vec3 _color;
	bool _pressed;
	bool _highlight;
	ButtonCallback _callbackFunction;	/* A pointer to a function to call if the button is pressed */
	//Texture2D image;
	
};

class InputText
{
friend class GUI;
public:
	InputText(){}
};
SINGLETON_BEGIN( GUI )

public:
	void draw();
	void close();
	void addText(string id, vec3& position, void* font, vec3& color, char* format, ...);
	void addButton(string id, string text, vec2& position, vec2& dimensions, vec3& color,ButtonCallback callback);
	void modifyText(string id, char* format, ...);

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