/*“Copyright 2009-2013 DIVIDE-Studio”*/
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
	GUIText(const std::string& id,
		    const std::string& text,
			const vec2<I32>& position,
			const std::string& font,
			const vec3<F32>& color,
            CEGUI::Window* parent,
			U32 textHeight = 16) : GUIElement(parent,GUI_TEXT,position),
      _width(1.0f),
	  _text(text),
	  _font(font),
	  _height(textHeight),
	  _color(color){}

	std::string _text;
	std::string _font;
	U32         _height;
    U32         _width;
	vec4<F32>   _color;

     void onMouseMove(const GUIEvent &event);
     void onMouseUp(const GUIEvent &event);
     void onMouseDown(const GUIEvent &event);
	 void onResize(const vec2<I32>& newSize);
/*   void onRightMouseUp(const GUIEvent &event);
     void onRightMouseDown(const GUIEvent &event);
     bool onKeyUp(const GUIEvent &event);
     bool onKeyDown(const GUIEvent &event);
*/
};

#endif