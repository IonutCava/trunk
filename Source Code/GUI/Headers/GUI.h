/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef GUI_H_
#define GUI_H_

#include "Utility/Headers/UnorderedMap.h"
#include "Core/Math/Headers/MathClasses.h"
#include "GUI/GUIEditor/Headers/GUIEditor.h"
#include "GUI/CEGUIAddons/Headers/CEGUIKeyRepeat.h"

namespace Font {
    const static std::string	DIVIDE_DEFAULT ("DroidSerif-Regular.ttf"/*"Test.ttf"*/);
    const static std::string	BATANG ("Batang.ttf");
    const static std::string	DEJA_VU("DejaVuSans.ttf");
    const static std::string    DROID_SERIF("DroidSerif-Regular.ttf");
    const static std::string    DROID_SERIF_ITALIC("DroidSerif-Italic.ttf");
    const static std::string    DROID_SERIF_BOLD("DroidSerif-Bold.ttf");
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

class GUIText;
class GUIFlash;
class GUIButton;
/// Grafical User Interface
DEFINE_SINGLETON( GUI )
    typedef Unordered_map<std::string, GUIElement*> guiMap;
    typedef DELEGATE_CBK ButtonCallback;

public:
    /// Main display call
    void draw(const U64 deltaTime, const D32 interpolationFactor);
    /// Destroy items and close the GUI
    void close();
    /// Add a text label
    GUIText* addText(const std::string& id,const vec2<I32>& position, const std::string& font,const vec3<F32>& color, char* format, ...);
    /// Add a button with a specific callback. The root of the window positioning system is bottom left, so 100,60 will place the button 100 pixels to the right and 60 up from the bottom
    GUIButton* addButton(const std::string& id,const std::string& text,const vec2<I32>& position,const vec2<U32>& dimensions,const vec3<F32>& color,ButtonCallback callback,const std::string& rootSheetId = "");
    /// Add a flash element -DEPRECATED-
    GUIFlash* addFlash(const std::string& id, std::string movie, const vec2<U32>& position, const vec2<U32>& extent);
    /// Modify a text label
    GUIText* modifyText(const std::string& id, char* format, ...);
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
    /// Create the GUI
    bool init(const vec2<U16>& resolution);
    /// Used by CEGUI to setup rendering (D3D/OGL/OGRE/etc)
    bool bindRenderer(CEGUI::Renderer& renderer);
    /// Used to prevent text updating every frame
    inline void setTextRenderTimer(const U64 renderIntervalUs) {_textRenderInterval = renderIntervalUs;}

private:
    GUI();               //< Constructor
    ~GUI();              //< Destructor
    void drawText();     //< TextLabel rendering

private:
    bool _init;                     //< Set to true when the GUI has finished loading
    bool _enableCEGUIRendering;     //< Toggle CEGUI rendering on/off (e.g. to check raw application rendering performance)
    GUIInput    _input;             //< Used to implement key repeat
    GUIConsole* _console;           //< Pointer to the GUIConsole object
    guiMap      _guiStack;          //< All the GUI elements created
    vec2<U16>   _cachedResolution;  //< We keep a cache of the current resolution to avoid useless queries
    U64         _textRenderInterval;//< We should avoid rendering text as fast as possible for performance reasons
    /// Used to check if we need to add a new GUIElement or modify an existing one
    std::pair<guiMap::iterator, bool > _resultGuiElement;
    CEGUI::Window* _rootSheet; //< gui root Window
    std::string _defaultGUIScheme;
    ShaderProgram* _guiShader;//<Used to apply color for text for now

END_SINGLETON
#endif
