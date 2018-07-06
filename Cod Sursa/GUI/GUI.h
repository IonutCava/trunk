#ifndef GUI_H_
#define GUI_H_
#include "resource.h"
#include "Utility/Headers/Singleton.h"

class Text 
{
friend class GUI;
public:
	Text(string& id,string& text,vec3& position, void* font, vec3& color) :
	  _id(id),
	  _text(text),
	  _position(position),
	  _font(font),
	  _color(color){};
private:
	string _id;
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
		_id(id),
		_text(text),
		_position(position),
		_dimensions(dimensions),
		_color(color)
		/*_image(image)*/{};
private:
	string _id;
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

	vector<Text*> _text;
    vector<Text*>::iterator _textIterator;
	vector<Button*> _button;
    vector<Button*>::iterator _buttonIterator;
SINGLETON_END()
#endif