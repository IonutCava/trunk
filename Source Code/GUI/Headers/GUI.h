/*
   Copyright (c) 2015 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef GUI_H_
#define GUI_H_

#include "Utility/Headers/HashMap.h"
#include "Core/Math/Headers/MathMatrices.h"
#include "GUI/GUIEditor/Headers/GUIEditor.h"
#include "GUI/CEGUIAddons/Headers/CEGUIInput.h"
#include "Platform/Input/Headers/InputAggregatorInterface.h"

namespace CEGUI {
class Renderer;
};

namespace Divide {

namespace Font {
const static stringImpl DIVIDE_DEFAULT("DroidSerif-Regular.ttf" /*"Test.ttf"*/);
const static stringImpl BATANG("Batang.ttf");
const static stringImpl DEJA_VU("DejaVuSans.ttf");
const static stringImpl DROID_SERIF("DroidSerif-Regular.ttf");
const static stringImpl DROID_SERIF_ITALIC("DroidSerif-Italic.ttf");
const static stringImpl DROID_SERIF_BOLD("DroidSerif-Bold.ttf");
};

class GUIConsole;
class GUIElement;
class ShaderProgram;
class RenderStateBlock;

#define CEGUI_DEFAULT_CTX \
    CEGUI::System::getSingleton().getDefaultGUIContext()

class GUIText;
class GUIFlash;
class GUIButton;
class GUIMessageBox;
class Scene;

/// Graphical User Interface

DEFINE_SINGLETON_EXT1(GUI, Input::InputAggregatorInterface)
    typedef hashMapImpl<stringImpl, GUIElement*> guiMap;
    typedef DELEGATE_CBK<> ButtonCallback;

  public:
    /// Main update call
    void update(const U64 deltaTime);
    /// Add a text label
    GUIText* addText(const stringImpl& id, const vec2<I32>& position,
                     const stringImpl& font, const vec3<F32>& color, char* format,
                     ...);
    /// Modify a text label
    GUIText* modifyText(const stringImpl& id, char* format, ...);
    GUIMessageBox* addMsgBox(const stringImpl& id, const stringImpl& title,
                             const stringImpl& message,
                             const vec2<I32>& offsetFromCentre = vec2<I32>(0));
    /// Add a button with a specific callback.
    /// The root of the window positioning system is bottom left, so 100,60 will
    /// place the button 100 pixels to the right and 60 up
    /// from the bottom
    GUIButton* addButton(const stringImpl& ID, const stringImpl& text,
                         const vec2<I32>& position, const vec2<U32>& dimensions,
                         const vec3<F32>& color, ButtonCallback callback,
                         const stringImpl& rootSheetID = "");
    /// Add a flash element -DEPRECATED-
    GUIFlash* addFlash(const stringImpl& id, stringImpl movie,
                       const vec2<U32>& position, const vec2<U32>& extent);
    /// Called on window resize to adjust the dimensions of all the GUI elements
    void onResize(const vec2<U16>& newResolution);
    /// Get a pointer to our console window
    inline GUIConsole* const getConsole() { return _console; }
    inline const GUIEditor& getEditor() { return GUIEditor::getInstance(); }
    /// Get a const pointer to an element by name/id
    inline GUIElement* const getItem(const stringImpl& id) { return _guiStack[id]; }
    /// Get a pointer to an element by name/id
    inline GUIElement* getGuiElement(const stringImpl& id) { return _guiStack[id]; }
    /// Create the GUI
    bool init(const vec2<U16>& resolution);
    /// Used by CEGUI to setup rendering (D3D/OGL/OGRE/etc)
    bool bindRenderer(CEGUI::Renderer& renderer);
    void selectionChangeCallback(Scene* const activeScene);
    /// Return a pointer to the default, general purpose message box
    inline GUIMessageBox* const getDefaultMessageBox() const {
        return _defaultMsgBox;
    }
    /// Used to prevent text updating every frame
    inline void setTextRenderTimer(const U64 renderIntervalUs) {
        _textRenderInterval = renderIntervalUs;
    }
    /// Mouse cursor forced to a certain position
    void setCursorPosition(U16 x, U16 y) const;
    /// Key pressed
    bool onKeyDown(const Input::KeyEvent& key);
    /// Key released
    bool onKeyUp(const Input::KeyEvent& key);
    /// Joystick axis change
    bool joystickAxisMoved(const Input::JoystickEvent& arg, I8 axis);
    /// Joystick direction change
    bool joystickPovMoved(const Input::JoystickEvent& arg, I8 pov);
    /// Joystick button pressed
    bool joystickButtonPressed(const Input::JoystickEvent& arg,
                               Input::JoystickButton button);
    /// Joystick button released
    bool joystickButtonReleased(const Input::JoystickEvent& arg,
                                Input::JoystickButton button);
    bool joystickSliderMoved(const Input::JoystickEvent& arg, I8 index);
    bool joystickVector3DMoved(const Input::JoystickEvent& arg, I8 index);
    /// Mouse moved
    bool mouseMoved(const Input::MouseEvent& arg);
    /// Mouse button pressed
    bool mouseButtonPressed(const Input::MouseEvent& arg,
                            Input::MouseButton button);
    /// Mouse button released
    bool mouseButtonReleased(const Input::MouseEvent& arg,
                             Input::MouseButton button);

  private:
    GUI();            //< Constructor
    ~GUI();           //< Destructor
    void draw() const;

  private:
    bool _init;              //< Set to true when the GUI has finished loading
    CEGUIInput _ceguiInput;  //< Used to implement key repeat
    GUIConsole* _console;    //< Pointer to the GUIConsole object
    GUIMessageBox* _defaultMsgBox;  //< Pointer to a default message box used for
                                    //general purpose messages
    guiMap _guiStack;               //< All the GUI elements created
    vec2<U16> _cachedResolution;    //< We keep a cache of the current resolution to
                                    //avoid useless queries
    U64 _textRenderInterval;  //< We should avoid rendering text as fast as possible
                              //for performance reasons
    CEGUI::Window* _rootSheet;  //< gui root Window
    stringImpl _defaultGUIScheme;
    ShaderProgram* _guiShader;  //<Used to apply color for text for now

END_SINGLETON

};  // namespace Divide
#endif
