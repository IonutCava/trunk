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

#ifndef _GUI_BUTTON_H_
#define _GUI_BUTTON_H_

#include "GUIElement.h"
#include "GUIText.h"
#include <boost/function.hpp>

namespace CEGUI{
	class Window;
    class Font;
    class EventArgs;
};

class GUIButton : public GUIElement {

typedef boost::function0<void> ButtonCallback;
friend class GUI;
public:
	GUIButton(const std::string& id,
			  const std::string& text,
              const std::string& guiScheme,
			  const vec2<U32>& position,
			  const vec2<U32>& dimensions,
			  const vec3<F32>& color,
              CEGUI::Window* parent,
			  ButtonCallback callback);
	~GUIButton();

    void setTooltip(const std::string& tooltipText);
    void setFont(const std::string& fontName, const std::string& fontFileName, U32 size);

protected:
    bool buttonPressed(const CEGUI::EventArgs& /*e*/);

protected:
   	std::string _text;
	vec2<U32>	_dimensions;
	vec3<F32>	_color;
	bool		_pressed;
	bool		_highlight;
	ButtonCallback _callbackFunction;	/* A pointer to a function to call if the button is pressed */
    CEGUI::Window *_btnWindow;
};
#endif