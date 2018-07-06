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

#ifndef GUI_H_
#define GUI_H_

#include "Utility/Headers/UnorderedMap.h"
#include "Core/Math/Headers/MathClasses.h"
#include "GUI/GUIEditor/Headers/GUIEditor.h"
#include "GUI/CEGUIAddons/Headers/CEGUIKeyRepeat.h"
#include <boost/function.hpp>

namespace Font {
	const static std::string	DIVIDE_DEFAULT ("Test.ttf");
	const static std::string	BATANG ("Batang.ttf");
	const static std::string	DEJA_VU("DejaVuSans.ttf");
};

class GUIConsole;
class GUIElement;
class ShaderProgram;
class RenderStateBlock;

namespace CEGUI{
	class Renderer;
};

namespace OIS {
	class KeyEvent;
	class MouseEvent;
	class JoyStickEvent;
	enum MouseButtonID;
}

#define CEGUI_DEFAULT_CONTEXT CEGUI::System::getSingleton().getDefaultGUIContext()

/// Grafical User Interface
DEFINE_SINGLETON( GUI )
	typedef Unordered_map<std::string, GUIElement*> guiMap;
	typedef boost::function0<void> ButtonCallback;

public:
	/// Main display call
	void draw(U32 timeElapsed = 0);
	/// Destroy items and close the GUI
	void close();
	/// Add a text label
	GUIElement* addText(const std::string& id,const vec2<I32>& position, const std::string& font,const vec3<F32>& color, char* format, ...);
	/// Add a button with a specific callback. The root of the window positioning system is bottom left, so 100,60 will place the button 100 pixels to the right and 60 up from the bottom
    GUIElement* addButton(const std::string& id,const std::string& text,const vec2<I32>& position,const vec2<U32>& dimensions,const vec3<F32>& color,ButtonCallback callback,const std::string& rootSheetId = "");
	/// Add a flash element -DEPRECATED-
	GUIElement* addFlash(const std::string& id, std::string movie, const vec2<U32>& position, const vec2<U32>& extent);
	/// Modify a text label
	GUIElement* modifyText(const std::string& id, char* format, ...);
	/// Called on window resize to adjust the dimensions of all the GUI elements
	void onResize(const vec2<U16>& newResolution);
	/// Mouse Button Up/Down callback
	bool clickCheck(OIS::MouseButtonID button, bool pressed);
	/// Key Press/Release callback
	bool keyCheck(OIS::KeyEvent key, bool pressed);
	/// Mouse move / scroll callback
	bool checkItem(const OIS::MouseEvent& arg);
	/// Get a pointer to our console window
	inline GUIConsole* const getConsole() {return _console;}
	inline const GUIEditor&  getEditor()  {return GUIEditor::getInstance(); }
	/// Get a const pointer to an element by name/id
	inline GUIElement* const getItem(const std::string& id) {return _guiStack[id];}
	/// Get a pointer to an element by name/id
	inline GUIElement* getGuiElement(const std::string& id){return _guiStack[id];}
	/// Update internal resolution cache
	inline void cacheResolution(const vec2<U16>& resolution) {_cachedResolution = resolution;}
	/// Create the GUI
	bool init();
	/// Used by CEGUI to setup rendering (D3D/OGL/OGRE/etc)
	bool bindRenderer(CEGUI::Renderer& renderer);

private:
	GUI();               //< Constructor
	~GUI();              //< Destructor
	void drawText();     //< TextLabel rendering

private:
	bool _init;                     //< Set to true when the GUI has finished loading
    bool _enableCEGUIRendering;     //< Toggle CEGUI rendering on/off (e.g. to check raw application rendering performance)
	U32 _prevElapsedTime;           //< Time when the GUI was last rendered. Used to calculate frametime
	GUIInput    _input;             //< Used to implement key repeat
	GUIConsole* _console;           //< Pointer to the GUIConsole object
	guiMap      _guiStack;          //< All the GUI elements created
	vec2<U16>   _cachedResolution;  //< We keep a cache of the current resolution to avoid useless queries
	/// Used to check if we need to add a new GUIElement or modify an existing one
	std::pair<guiMap::iterator, bool > _resultGuiElement;
    CEGUI::Window* _rootSheet; //< gui root Window
    std::string _defaultGUIScheme;
    ShaderProgram* _guiShader;//<Used to apply color for text for now
END_SINGLETON
#endif
