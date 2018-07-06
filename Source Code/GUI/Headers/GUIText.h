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

#ifndef _GUI_TEXT_H_
#define _GUI_TEXT_H_
#include "GUIElement.h"

class GUIText : public GUIElement{

friend class GUI;
public:
	GUIText(const std::string& id,std::string& text,const vec2<F32>& position, void* font,const vec3<F32>& color) : GUIElement(),
	  _text(text),
	  _font(font),
	  _color(color){_position = position; _guiType = GUI_TEXT;};

	std::string _text;
	void*		_font;
	vec3<F32>   _color;

     void onMouseMove(const GUIEvent &event);
     void onMouseUp(const GUIEvent &event);
     void onMouseDown(const GUIEvent &event);
	 void onResize(const vec2<U16>& newSize){}
/*   void onRightMouseUp(const GUIEvent &event);
     void onRightMouseDown(const GUIEvent &event);
     bool onKeyUp(const GUIEvent &event);
     bool onKeyDown(const GUIEvent &event);
*/
};

#endif