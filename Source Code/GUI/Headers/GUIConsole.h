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

#ifndef _GUI_CONSOLE_H_
#define _GUI_CONSOLE_H_
#include "GUIElement.h"

typedef void (*console_function)(const std::vector<std::string> &); 
///Input
namespace OIS {
	class KeyEvent;
	class MouseEvent;
	enum MouseButtonID;
}

enum console_item_type_t
{
    CTYPE_UCHAR,        // variable: unsigned char
    CTYPE_CHAR,         // variable: char
    CTYPE_UINT,         // variable: unsigned char
    CTYPE_INT,          // variable: int
    CTYPE_FLOAT,        // variable: float
    CTYPE_STRING,       // variable: std::string
    CTYPE_FUNCTION      // function
};

typedef struct
{
    std::string name;

    console_item_type_t type;

    union
    {
        void *variable_pointer;
        console_function function;
    };

} console_item_t;

class Quad3D;
DEFINE_SINGLETON_EXT1(GUIConsole, GUIElement)

private:
    GUIConsole();
     ~GUIConsole();

public:
    void addItem(const std::string & strName,
    void *pointer, console_item_type_t type);

    void removeItem(const std::string & strName);

    void setDefaultCommand(console_function func);

    void print(const std::string & strText);


    void passKey(char key);

    void passBackspace();

    void passIntro();

	inline void setScreenPercentage(F32 percentage) {_screenPercentage = percentage; CLAMP(_screenPercentage,0.1f, 100.0f);}
	inline void setAnimationDuration(F32 durationInSeconds) {_animationDuration = durationInSeconds * 1000.0f;CLAMP(_animationDuration,0.0f,2000.0f); }
	inline Quad3D* getRenderQuad() {return _consoleRect;}
	inline vec2<I32>& getViewportDimensions() {return _viewportDimensions;}

	F32 getConsoleHeight();

	inline void openConsole() {_consoleOpen = true;  _animationRunning = true; _lastCheck = GETMSTIME(); }
	inline void closeConsole(){_consoleOpen = false; _animationRunning = true; _lastCheck = GETMSTIME(); }

	inline void toggleConsole() {_consoleOpen ? closeConsole() : openConsole();}

	void setConsoleBackgroundColor(const vec3<F32>& color);
	void setConsoleBlendFactor(F32 factor);

	inline bool isConsoleOpen() {return _consoleOpen;}


	///Input
	void onKeyDown(const OIS::KeyEvent& key);
	void onKeyUp(const OIS::KeyEvent& key);
	void onMouseMove(const OIS::MouseEvent& key);
	void onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button);
	void onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button);

private:
    bool parseCommandLine();

private:
    std::vector<std::string> m_commandBuffer;

    std::list<console_item_t> m_itemList;

    console_function defaultCommand;


protected:
    std::list<std::string> m_textBuffer;

    std::string	m_commandLine;
	Quad3D* _consoleRect;
	vec2<I32> _viewportDimensions;
	F32 _lastCheck;
	F32 _animationDuration;
	F32 _screenPercentage;
	bool _consoleOpen;
	bool _animationRunning;

END_SINGLETON

#endif