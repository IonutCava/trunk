#ifndef GUI_H_
#define GUI_H_
#include "resource.h"
#include <unordered_map>
#include "Utility/Headers/Singleton.h"

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
	Button(string& id,string& text,vec3& position, vec2& dimensions, vec3& color/*, Texture2D& image*/) :
		_text(text),
		_position(position),
		_dimensions(dimensions),
		_color(color)
		/*_image(image)*/{};

	string _text;
	vec3 _position;
	vec2 _dimensions;
	vec3 _color;
	//Texture2D image;
	
};

SINGLETON_BEGIN( GUI )

public:
	void draw();
	void close();
	void addText(string id, vec3& position, void* font, vec3& color, char* format, ...);
	void addButton(string id, string text, vec3& position, vec2& dimensions, vec3& color);
	void modifyText(string id, char* format, ...);

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