/*“Copyright 2009-2012 DIVIDE-Studio”*/
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

#ifndef _GUI_ELEMENT_H_
#define _GUI_ELEMENT_H_
#include "core.h"

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

enum GUIType
{
	GUI_TEXT				= 0x0000,
	GUI_BUTTON				= 0x0001,
	GUI_FLASH				= 0x0002,
	GUI_CONSOLE             = 0x0003,
	GUI_PLACEHOLDER			= 0x0004
};


struct GUIEvent {

   U16                  ascii;            ///< ascii character code 'a', 'A', 'b', '*', etc (if device==keyboard) - possibly a uchar or something
   U8                   modifier;         ///< SI_LSHIFT, etc
   U16                  keyCode;          ///< for unprintables, 'tab', 'return', ...
   vec2<F32>            mousePoint;       ///< for mouse events
   U8                   mouseClickCount;
};

class RenderStateBlock;
class GUIElement{
	typedef GUIElement Parent;
	friend class GUI;

public:
	GUIElement() ;
	virtual ~GUIElement();
	inline const std::string& getName() const {return _name;}
	inline const vec2<F32>&   getPosition()  const {return _position;}
	inline void  setPosition(vec2<F32>& pos)        {_position = pos;}
	inline const GUIType getGuiType()   const {return _guiType;}

	inline const bool isActive()  const {return _active;}
	inline const bool isVisible() const {return _visible;}

	inline void    setName(const std::string& name) {_name = name;}
	inline void    setVisible(bool visible)		    {_visible = visible;}
	inline void    setActive(bool active)			{_active = active;}

	inline void    addChildElement(GUIElement* child)    {}

	virtual void onResize(const vec2<F32>& newSize){_position -= newSize;}

	virtual void onMouseMove(const GUIEvent &event){};
	virtual void onMouseUp(const GUIEvent &event){};
	virtual void onMouseDown(const GUIEvent &event){};
/*  virtual void onRightMouseUp(const GUIEvent &event);
    virtual void onRightMouseDown(const GUIEvent &event);
    virtual bool onKeyUp(const GUIEvent &event);
    virtual bool onKeyDown(const GUIEvent &event);
*/
protected:
	vec2<F32> _position;
	GUIType   _guiType;
	Parent*   _parent;
	RenderStateBlock* _guiSB;

private:
	std::string _name;
	bool	    _visible;
	bool		_active;
};

#endif