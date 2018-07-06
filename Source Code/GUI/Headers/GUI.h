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

#ifndef GUI_H_
#define GUI_H_

#include "GUIElement.h"

enum Font;
class GUIElement;

DEFINE_SINGLETON( GUI )
	typedef unordered_map<std::string,GUIElement*> guiMap;
	typedef boost::function0<void> ButtonCallback;

public:
	void draw();
	void close();
	void addText(const std::string& id,const vec3<F32>& position, Font font,const vec3<F32>& color, char* format, ...);
	void addButton(const std::string& id, std::string text,const vec2<F32>& position,const vec2<F32>& dimensions,const vec3<F32>& color,ButtonCallback callback);
	void addFlash(const std::string& id, std::string movie, const vec2<F32>& position, const vec2<F32>& extent);
	void modifyText(const std::string& id, char* format, ...);
	void toggleConsole();
	void createConsole();
	void onResize(F32 newWidth, F32 newHeight);
	void clickCheck();
	void clickReleaseCheck();
	void checkItem(U16 x, U16 y);

	inline GUIElement* const getItem(const std::string& id) {return _guiStack[id];}
	inline GUIElement* getGuiElement(const std::string& id){return _guiStack[id];}

private:
	GUI();
	~GUI();
	void drawText();
	void drawButtons();

	guiMap _guiStack;
	std::pair<unordered_map<std::string, GUIElement*>::iterator, bool > _resultGuiElement;

END_SINGLETON
#endif
