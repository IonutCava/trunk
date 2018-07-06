/*
Copyright (c) 2017 DIVIDE-Studio
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

#ifndef _DIVIDE_EDITOR_H_
#define _DIVIDE_EDITOR_H_

#include "Core/Math/Headers/MathVectors.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Core/Headers/PlatformContextComponent.h"

#include "Rendering/Headers/FrameListener.h"
#include "Platform/Input/Headers/InputAggregatorInterface.h"

struct ImDrawData;
namespace Divide {

namespace Attorney {
    class EditorWindowManager;
    class EditorPanelManager;
};

class PanelManager;
class DisplayWindow;
class PlatformContext;
class ImwWindowManagerDivide;
FWD_DECLARE_MANAGED_CLASS(Texture);
FWD_DECLARE_MANAGED_CLASS(ShaderProgram);

class Editor : public PlatformContextComponent,
               public FrameListener,
               public Input::InputAggregatorInterface {

    friend class Attorney::EditorWindowManager;
    friend class Attorney::EditorPanelManager;

  public:
    // Basically, the IMGUI default themes
    enum class Theme : U8 {
        IMGUI_Default = 0,
        IMGUI_Default_Dark,
        Gray,
        OSX,
        Dark_Opaque,
        OSX_Opaque,
        Soft,
        Edin_Black,
        Edin_White,
        Maya,
        COUNT
    };

  public:
    explicit Editor(PlatformContext& context,
                    Theme theme = Theme::OSX_Opaque,
                    Theme lostFocusTheme = Theme::OSX,
                    Theme dimmedTheme = Theme::Gray);
    ~Editor();

    bool init();
    void close();
    void idle();
    void update(const U64 deltaTimeUS);

    void toggle(const bool state);
    bool running() const;

    bool shouldPauseSimulation() const;

    inline const vec4<I32>& getScenePreviewRect() const { return _scenePreviewRect; }

  protected: //frame listener
    bool frameStarted(const FrameEvent& evt);
    bool framePreRenderStarted(const FrameEvent& evt);
    bool framePreRenderEnded(const FrameEvent& evt);
    bool frameRenderingQueued(const FrameEvent& evt);
    bool framePostRenderStarted(const FrameEvent& evt);
    bool framePostRenderEnded(const FrameEvent& evt);
    bool frameEnded(const FrameEvent& evt);

  public: // input
    /// Key pressed: return true if input was consumed
    bool onKeyDown(const Input::KeyEvent& key);
    /// Key released: return true if input was consumed
    bool onKeyUp(const Input::KeyEvent& key);
    /// Mouse moved: return true if input was consumed
    bool mouseMoved(const Input::MouseEvent& arg);
    /// Mouse button pressed: return true if input was consumed
    bool mouseButtonPressed(const Input::MouseEvent& arg, Input::MouseButton button);
    /// Mouse button released: return true if input was consumed
    bool mouseButtonReleased(const Input::MouseEvent& arg, Input::MouseButton button);

    bool joystickButtonPressed(const Input::JoystickEvent &arg, Input::JoystickButton button) override;
    bool joystickButtonReleased(const Input::JoystickEvent &arg, Input::JoystickButton button) override;
    bool joystickAxisMoved(const Input::JoystickEvent &arg, I8 axis) override;
    bool joystickPovMoved(const Input::JoystickEvent &arg, I8 pov) override;
    bool joystickSliderMoved(const Input::JoystickEvent &arg, I8 index) override;
    bool joystickvector3Moved(const Input::JoystickEvent &arg, I8 index) override;

  protected: // window events
    bool OnClose();
    void OnFocus(bool bHasFocus);
    void OnSize(int iWidth, int iHeight);
    void OnUTF8(const char* text);
    void dim(bool hovered, bool focused);
    bool toggleScenePreview(bool state);
    void setScenePreviewRect(const vec4<I32>& rect, bool hovered);

  protected: // attorney
    void renderDrawList(ImDrawData* pDrawData, I64 windowGUID);

  private:
    Theme _currentTheme;
    Theme _currentLostFocusTheme;
    Theme _currentDimmedTheme;

    vec4<I32> _scenePreviewRect;

    I64 _activeWindowGUID = -1;
    std::unique_ptr<ImwWindowManagerDivide> _windowManager;
    std::unique_ptr<PanelManager> _panelManager;

    bool              _running;
    bool              _sceneHovered;
    bool              _sceneWasHovered;
    bool              _scenePreviewFocused;
    bool              _scenePreviewWasFocused;
    DisplayWindow*    _mainWindow;
    Texture_ptr       _fontTexture;
    ShaderProgram_ptr _imguiProgram;

    Time::ProfileTimer& _editorUpdateTimer;
    Time::ProfileTimer& _editorRenderTimer;

}; //Editor

namespace Attorney {
    class EditorWindowManager {
      private:
        static void renderDrawList(Editor& editor, ImDrawData* pDrawData, I64 windowGUID) {
            editor.renderDrawList(pDrawData, windowGUID);
        }

        friend class Divide::ImwWindowManagerDivide;
    };

    class EditorPanelManager {
        //private:
        public: //ToDo: fix this -Ionut
        static void setScenePreviewRect(Editor& editor, const vec4<I32>& rect, bool hovered) {
            editor.setScenePreviewRect(rect, hovered);
        }

        friend class Divide::PanelManager;
    };
};


}; //namespace Divide

#endif //_DIVIDE_EDITOR_H_