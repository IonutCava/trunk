/*“Copyright 2009-2011 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

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


class GuiElement{
	typedef GuiElement Parent;
	friend class GUI;

public:
	GuiElement(){_name = "defaultGuiControl";_visible = true;}
	virtual ~GuiElement(){}
	inline const std::string& getName() const {return _name;}
	inline const vec2&   getPosition()  const {return _position;}
	inline void  setPosition(vec2& pos)        {_position = pos;}
	inline const GuiType getGuiType()   const {return _guiType;}

	inline const bool isActive()  const {return _active;}
	inline const bool isVisible() const {return _visible;}

	inline void    setName(const std::string& name) {_name = name;}
	inline void    setVisible(bool visible)		    {_visible = visible;}
	inline void    setActive(bool active)			{_active = active;}

	inline void    addChildElement(GuiElement* child)    {}

	virtual void onResize(const vec2& newSize){_position -= newSize;}

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
	std::string _name;
	bool	    _visible;
	bool		_active;
};

class Text : public GuiElement
{
friend class GUI;
public:
	Text(const std::string& id,std::string& text,const vec2& position, void* font,const vec3& color) : GuiElement(),
	  _text(text),
	  _font(font),
	  _color(color){_position = position; _guiType = GUI_TEXT;};

	std::string _text;
	void*		_font;
	vec3		_color;

     void onMouseMove(const GuiEvent &event);
     void onMouseUp(const GuiEvent &event);
     void onMouseDown(const GuiEvent &event);
	 void onResize(const vec2& newSize){}
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
	Button(const std::string& id,std::string& text,const vec2& position,const vec2& dimensions,const vec3& color/*, Texture2D& image*/,ButtonCallback callback)  : GuiElement(),
		_text(text),
		_dimensions(dimensions),
		_color(color),
		_callbackFunction(callback),
		_highlight(false),
		_pressed(false)
		/*_image(image)*/{_position = position;_guiType = GUI_BUTTON; setActive(true);}

	std::string _text;
	vec2		_dimensions;
	vec3		_color;
	bool		_pressed;
	bool		_highlight;
	ButtonCallback _callbackFunction;	/* A pointer to a function to call if the button is pressed */
	//Texture2D image;

	void onResize(const vec2& newSize){
		if(_dimensions.x - newSize.x/_dimensions.x > 0.075f &&
		   _dimensions.y - newSize.y/_dimensions.y > 0.05f){
			_position -= newSize;
			_dimensions.x -= newSize.x/_dimensions.x; 
			_dimensions.y -= newSize.y/_dimensions.y; 
		}
	}
    void onMouseMove(const GuiEvent &event);
    void onMouseUp(const GuiEvent &event);
    void onMouseDown(const GuiEvent &event);
/*  void onRightMouseUp(const GuiEvent &event);
    void onRightMouseDown(const GuiEvent &event);
    bool onKeyUp(const GuiEvent &event);
    bool onKeyDown(const GuiEvent &event);
*/
};

class InputText : public GuiElement
{
friend class GUI;
public:
	InputText() : GuiElement() {}
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


DEFINE_SINGLETON( GUI )
	typedef unordered_map<std::string,GuiElement*> guiMap;
public:
	void draw();
	void close();
	void addText(const std::string& id,const vec3& position, Font font,const vec3& color, char* format, ...);
	void addButton(const std::string& id, std::string text,const vec2& position,const vec2& dimensions,const vec3& color,ButtonCallback callback);
	void addFlash(const std::string& id, std::string movie, const vec2& position, const vec2& extent);
	void modifyText(const std::string& id, char* format, ...);
	void onResize(F32 newWidth, F32 newHeight);
	void clickCheck();
	void clickReleaseCheck();
	void checkItem(U16 x, U16 y);

	GuiElement* getGuiElement(const std::string& id){return _guiStack[id];}

private:
	~GUI();
	void drawText();
	void drawButtons();

	guiMap _guiStack;
	std::pair<unordered_map<std::string, GuiElement*>::iterator, bool > _resultGuiElement;

END_SINGLETON
#endif
