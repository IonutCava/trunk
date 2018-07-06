#ifndef GUI_H_
#define GUI_H_
#include "resource.h"
#include "Utility/Headers/Singleton.h"
//#include "Hardware/Input/InputManager.h"
#include <boost/function.hpp>

//typedef void (*ButtonCallback)();
typedef boost::function0<void> ButtonCallback;

enum GuiType
{
	GUI_TEXT				= 0x0000,
	GUI_BUTTON				= 0x0001,
	GUI_FLASH				= 0x0002,
	GUI_PLACEHOLDER			= 0x0003,
};


struct GuiEvent
{
   U16                  ascii;            ///< ascii character code 'a', 'A', 'b', '*', etc (if device==keyboard) - possibly a uchar or something
   U8                   modifier;         ///< SI_LSHIFT, etc
   U16                  keyCode;          ///< for unprintables, 'tab', 'return', ...
   vec2                 mousePoint;       ///< for mouse events
   U8                   mouseClickCount;
};
//<< END TORQUE CODE. Please replace. Only for reference

class GuiElement
{
	typedef GuiElement Parent;
	friend class GUI;

public:
	GuiElement(){_name = "defaultGuiControl";_visible = true;}
	virtual ~GuiElement(){}
	const string& getName()     const {return _name;}
	const vec2&   getPosition() const {return _position;}
	const GuiType getGuiType()  const {return _guiType;}

	const bool isActive()  const {return _active;}
	const bool isVisible() const {return _visible;}

	void    setName(const string& name)    {_name = name;}
	void    setVisible(bool visible)       {_visible = visible;}
	void    setActive(bool active)         {_active = active;}

	void    addChildElement(GuiElement* child)    {}

	virtual void onMouseMove(const GuiEvent &event){};
	virtual void onMouseUp(const GuiEvent &event){};
	virtual void onMouseDown(const GuiEvent &event){};
/*  virtual void onRightMouseUp(const GuiEvent &event);
    virtual void onRightMouseDown(const GuiEvent &event);
    virtual bool onKeyUp(const GuiEvent &event);
    virtual bool onKeyDown(const GuiEvent &event);
*/
protected:
	vec2	_position;
	GuiType _guiType;
	Parent* _parent;

private:
	string _name;
	bool   _visible;
	bool   _active;
};

class Text : public GuiElement
{
friend class GUI;
public:
	Text(const string& id,string& text,const vec2& position, void* font,const vec3& color) :
	  _text(text),
	  _font(font),
	  _color(color){_position = position; _guiType = GUI_TEXT;};

	string _text;
	void* _font;
	vec3 _color;

     void onMouseMove(const GuiEvent &event);
     void onMouseUp(const GuiEvent &event);
     void onMouseDown(const GuiEvent &event);
/*   void onRightMouseUp(const GuiEvent &event);
     void onRightMouseDown(const GuiEvent &event);
     bool onKeyUp(const GuiEvent &event);
     bool onKeyDown(const GuiEvent &event);
*/
};

class Button : public GuiElement
{

friend class GUI;
public:
	Button(const string& id,string& text,const vec2& position,const vec2& dimensions,const vec3& color/*, Texture2D& image*/,ButtonCallback callback) :
		_text(text),
		_dimensions(dimensions),
		_color(color),
		_callbackFunction(callback)
		/*_image(image)*/{_position = position;_pressed = false; _highlight = false; _guiType = GUI_BUTTON;};

	string _text;
	vec2 _dimensions;
	vec3 _color;
	bool _pressed;
	bool _highlight;
	ButtonCallback _callbackFunction;	/* A pointer to a function to call if the button is pressed */
	//Texture2D image;

     void onMouseMove(const GuiEvent &event);
     void onMouseUp(const GuiEvent &event);
     void onMouseDown(const GuiEvent &event);
/*   void onRightMouseUp(const GuiEvent &event);
     void onRightMouseDown(const GuiEvent &event);
     bool onKeyUp(const GuiEvent &event);
     bool onKeyDown(const GuiEvent &event);
*/
};

class InputText : public GuiElement
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
	void addText(const string& id,const vec3& position, Font font,const vec3& color, char* format, ...);
	void addButton(const string& id, string text,const vec2& position,const vec2& dimensions,const vec3& color,ButtonCallback callback);
	void addFlash(const string& id, string movie, const vec2& position, const vec2& extent);
	void modifyText(const string& id, char* format, ...);
	void resize(int newWidth, int newHeight);
	void clickCheck();
	void clickReleaseCheck();
	void checkItem(int x, int y);

	GuiElement* getGuiElement(const string& id){return _guiStack[id];}

private:
	~GUI();
	void drawText();
	void drawButtons();

	tr1::unordered_map<string,GuiElement*> _guiStack;
	tr1::unordered_map<string, GuiElement*>::iterator _guiStackIterator;
	pair<tr1::unordered_map<string, GuiElement*>::iterator, bool > _resultGuiElement;

SINGLETON_END()
#endif
