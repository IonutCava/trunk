/*
   Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef GUI_H_
#define GUI_H_

#include "GUIInterface.h"
#include "Core/Headers/KernelComponent.h"
#include "Core/Math/Headers/MathMatrices.h"
#include "GUI/CEGUIAddons/Headers/CEGUIInput.h"
#include "Platform/Video/Headers/TextureData.h"
#include "Platform/Input/Headers/InputAggregatorInterface.h"

namespace CEGUI {
class Renderer;
};

namespace Divide {

namespace GFX {
    class CommandBuffer;
};

class GUIConsole;
class GUIElement;
class ResourceCache;
class SceneGraphNode;
class PlatformContext;
class RenderStateBlock;

FWD_DECLARE_MANAGED_CLASS(ShaderProgram);

class Scene;
struct SizeChangeParams;
/// Graphical User Interface

class SceneGUIElements;
class GUI : public GUIInterface,
            public KernelComponent,
            public Input::InputAggregatorInterface {
public:
    typedef hashMap<I64, SceneGUIElements*> GUIMapPerScene;

public:
    explicit GUI(Kernel& parent);
    ~GUI();

    /// Create the GUI
    bool init(PlatformContext& context, ResourceCache& cache, const vec2<U16>& renderResolution);
    void destroy();

    void draw(GFXDevice& context, GFX::CommandBuffer& bufferInOut);

    void onSizeChange(const SizeChangeParams& params) override;
    void onChangeScene(Scene* newScene);
    void onUnloadScene(Scene* scene);

    /// Main update call
    void update(const U64 deltaTimeUS);

    template<typename T = GUIElement>
    inline T* getGUIElement(I64 sceneID, U64 elementName) {
        static_assert(std::is_base_of<GUIElement, T>::value,
            "getGuiElement error: Target is not a valid GUI item");

        return static_cast<T*>(getGUIElementImpl(sceneID, elementName, getTypeEnum<T>()));
    }

    template<typename T = GUIElement>
    inline T* getGUIElement(I64 sceneID, I64 elementID) {
        static_assert(std::is_base_of<GUIElement, T>::value,
            "getGuiElement error: Target is not a valid GUI item");

        return static_cast<T*>(getGUIElementImpl(sceneID, elementID, getTypeEnum<T>()));
    }

    /// Get a pointer to our console window
    inline GUIConsole& getConsole() { return *_console; }
    inline const GUIConsole& getConsole() const { return *_console; }

    inline CEGUI::Window* rootSheet() const { return _rootSheet; }
    inline const stringImpl& guiScheme() const { return _defaultGUIScheme; }

    void selectionChangeCallback(Scene* const activeScene, PlayerIndex idx, SceneGraphNode* node);
    /// Return a pointer to the default, general purpose message box
    inline GUIMessageBox* const getDefaultMessageBox() const {
        return _defaultMsgBox;
    }
    /// Used to prevent text updating every frame
    inline void setTextRenderTimer(const U64 renderIntervalUs) {
        _textRenderInterval = renderIntervalUs;
    }
    /// Mouse cursor forced to a certain position
    void setCursorPosition(I32 x, I32 y);
    /// Key pressed: return true if input was consumed
    bool onKeyDown(const Input::KeyEvent& key);
    /// Key released: return true if input was consumed
    bool onKeyUp(const Input::KeyEvent& key);
    /// Joystick axis change: return true if input was consumed
    bool joystickAxisMoved(const Input::JoystickEvent& arg, I8 axis);
    /// Joystick direction change: return true if input was consumed
    bool joystickPovMoved(const Input::JoystickEvent& arg, I8 pov);
    /// Joystick button pressed: return true if input was consumed
    bool joystickButtonPressed(const Input::JoystickEvent& arg, Input::JoystickButton button);
    /// Joystick button released: return true if input was consumed
    bool joystickButtonReleased(const Input::JoystickEvent& arg, Input::JoystickButton button);
    bool joystickSliderMoved(const Input::JoystickEvent& arg, I8 index);
    bool joystickvector3Moved(const Input::JoystickEvent& arg, I8 index);
    /// Mouse moved: return true if input was consumed
    bool mouseMoved(const Input::MouseEvent& arg);
    /// Mouse button pressed: return true if input was consumed
    bool mouseButtonPressed(const Input::MouseEvent& arg, Input::MouseButton button);
    /// Mouse button released: return true if input was consumed
    bool mouseButtonReleased(const Input::MouseEvent& arg, Input::MouseButton button);

    Scene* activeScene() {
        return _activeScene;
    }

    const Scene* activeScene() const {
        return _activeScene;
    }

    CEGUI::GUIContext& getCEGUIContext();
    const CEGUI::GUIContext& getCEGUIContext() const;

protected:
    GUIElement* getGUIElementImpl(I64 sceneID, U64 elementName, GUIType type) const;
    GUIElement* getGUIElementImpl(I64 sceneID, I64 elementID, GUIType type) const;
    TextureData getCEGUIRenderTextureData() const;

protected:
    friend class SceneGUIElements;
    CEGUI::Window* _rootSheet;  //< gui root Window
    stringImpl _defaultGUIScheme;
    CEGUI::GUIContext* _ceguiContext;
    CEGUI::TextureTarget* _ceguiRenderTextureTarget;

private:
    bool _init;              //< Set to true when the GUI has finished loading
                             /// Toggle CEGUI rendering on/off (e.g. to check raw application rendering
                             /// performance)
    CEGUIInput _ceguiInput;  //< Used to implement key repeat
    CEGUI::Renderer* _ceguiRenderer; //<Used to render CEGUI to a texture;
    GUIConsole* _console;    //< Pointer to the GUIConsole object
    GUIMessageBox* _defaultMsgBox;  //< Pointer to a default message box used for general purpose messages
    U64 _textRenderInterval;  //< We should avoid rendering text as fast as possible
                              //for performance reasons

    /// Each scene has its own gui elements! (0 = global)
    Scene* _activeScene;

    U32 _debugVarCacheCount;
    // GROUP, VAR
    vectorImpl<std::pair<I64, I64>> _debugDisplayEntries;

    /// All the GUI elements created per scene
    GUIMapPerScene _guiStack;
    mutable SharedLock _guiStackLock;
};

};  // namespace Divide
#endif
