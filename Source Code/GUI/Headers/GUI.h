/*
   Copyright (c) 2016 DIVIDE-Studio
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

#include "Core/Math/Headers/MathMatrices.h"
#include "GUI/GUIEditor/Headers/GUIEditor.h"
#include "GUI/CEGUIAddons/Headers/CEGUIInput.h"
#include "Platform/Input/Headers/InputAggregatorInterface.h"

namespace CEGUI {
class Renderer;
};

namespace Divide {

namespace Font {
const static char* DIVIDE_DEFAULT = "DroidSerif-Regular.ttf"; /*"Test.ttf"*/
const static char* BATANG = "Batang.ttf";
const static char* DEJA_VU = "DejaVuSans.ttf";
const static char* DROID_SERIF = "DroidSerif-Regular.ttf";
const static char* DROID_SERIF_ITALIC = "DroidSerif-Italic.ttf";
const static char* DROID_SERIF_BOLD = "DroidSerif-Bold.ttf";
};

class GUIConsole;
class GUIElement;
class RenderStateBlock;

FWD_DECLARE_MANAGED_CLASS(ShaderProgram);

#define CEGUI_DEFAULT_CTX CEGUI::System::getSingleton().getDefaultGUIContext()

class GUIText;
class GUIFlash;
class GUIButton;
class GUIMessageBox;
class Scene;

/// Graphical User Interface

DEFINE_SINGLETON_EXT1(GUI, Input::InputAggregatorInterface)
    typedef hashMapImpl<ULL, GUIElement*> GUIMap;
    typedef hashMapImpl<I64, GUIMap> GUIMapPerScene;

    typedef DELEGATE_CBK_PARAM<I64> ButtonCallback;

  public:
    /// Create the GUI
    bool init(const vec2<U16>& renderResolution);
    void onChangeResolution(U16 w, U16 h);
    void onChangeScene(I64 newSceneGUID);
    I64  activeSceneGUID() const;
    /// Main update call
    void update(const U64 deltaTime);
    /// Add a text label
    GUIText* addText(ULL ID,
                     const vec2<I32>& position,
                     const stringImpl& font,
                     const vec4<U8>& color,
                     const stringImpl& text,
                     U32 fontSize = 16);
    GUIText* addGlobalText(ULL ID,
                           const vec2<I32>& position,
                           const stringImpl& font,
                           const vec4<U8>& color,
                           const stringImpl& text,
                           U32 fontSize = 16);
    /// Modify a text label
    GUIText* modifyText(ULL ID, const stringImpl& text);
    GUIText* modifyGlobalText(ULL ID, const stringImpl& text);

    GUIMessageBox* addMsgBox(ULL ID, 
                             const stringImpl& title,
                             const stringImpl& message,
                             const vec2<I32>& offsetFromCentre = vec2<I32>(0));
    GUIMessageBox* addGlobalMsgBox(ULL ID,
                                   const stringImpl& title,
                                   const stringImpl& message,
                                   const vec2<I32>& offsetFromCentre = vec2<I32>(0));
    /// Add a button with a specific callback.
    /// The root of the window positioning system is bottom left, so 100,60 will
    /// place the button 100 pixels to the right and 60 up
    /// from the bottom
    GUIButton* addButton(ULL ID,
                         const stringImpl& text,
                         const vec2<I32>& position,
                         const vec2<U32>& dimensions,
                         ButtonCallback callback,
                         const stringImpl& rootSheetID = "");
    GUIButton* addGlobalButton(ULL ID,
                               const stringImpl& text,
                               const vec2<I32>& position,
                               const vec2<U32>& dimensions,
                               ButtonCallback callback,
                               const stringImpl& rootSheetID = "");
    /// Add a flash element -DEPRECATED-
    GUIFlash* addFlash(ULL ID, 
                       stringImpl movie,
                       const vec2<U32>& position,
                       const vec2<U32>& extent);
    GUIFlash* addGlobalFlash(ULL ID,
                             stringImpl movie,
                             const vec2<U32>& position,
                             const vec2<U32>& extent);
    /// Get a pointer to our console window
    inline GUIConsole* const getConsole() { return _console; }
    inline const GUIEditor& getEditor() { return GUIEditor::instance(); }
    /// Get a pointer to an element by name/id
    template<typename T = GUIElement>
    inline T* getGuiElement(I64 sceneID, ULL elementName) { 
        static_assert(std::is_base_of<GUIElement, T>::value, "getGuiElement error: Target is not a valid GUI item");
        return static_cast<T*>(_guiStack[sceneID][elementName]);
    }
    template<typename T = GUIElement>
    inline T* getGuiElement(I64 sceneID, I64 elementID) {
        static_assert(std::is_base_of<GUIElement, T>::value, "getGuiElement error: Target is not a valid GUI item");
        const GUIMap& guiStackForScene = _guiStack[sceneID];

        GUIElement* element = nullptr;
        for (GUIMap::value_type it : guiStackForScene) {
            if (it.second->getGUID() == elementID) {
                element = it.second;
                break;
            }
        }
        return static_cast<T*>(element);
    }
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
    void setCursorPosition(I32 x, I32 y) const;
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

    // GUI may render at a different resoltuion
    inline const vec2<U16>& getDisplayResolution() const {
        return _resolutionCache;
    }

  private:
    GUI();            //< Constructor
    ~GUI();           //< Destructor
    void draw() const;
    void addElement(ULL id, I64 sceneID, GUIElement* element);

  private:
    bool _init;              //< Set to true when the GUI has finished loading
                             /// Toggle CEGUI rendering on/off (e.g. to check raw application rendering
                             /// performance)
    CEGUIInput _ceguiInput;  //< Used to implement key repeat
    GUIConsole* _console;    //< Pointer to the GUIConsole object
    GUIMessageBox* _defaultMsgBox;  //< Pointer to a default message box used for
                                    //general purpose messages
    U64 _textRenderInterval;  //< We should avoid rendering text as fast as possible
                              //for performance reasons
    CEGUI::Window* _rootSheet;  //< gui root Window
    stringImpl _defaultGUIScheme;
    ShaderProgram_ptr _guiShader;  //<Used to apply color for text for now

    /// Each scene has its own gui elements! (0 = global)
    I64 _activeSceneGUID;
    vec2<U16> _resolutionCache;
    bool _enableCEGUIRendering;

    U32 _debugVarCacheCount;
    // GROUP, VAR
    vectorImpl<std::pair<I64, I64>> _debugDisplayEntries;

    /// All the GUI elements created per scene
    GUIMapPerScene _guiStack;               
END_SINGLETON

};  // namespace Divide
#endif
