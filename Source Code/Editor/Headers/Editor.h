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

#ifndef _DIVIDE_EDITOR_H_
#define _DIVIDE_EDITOR_H_

#include "Core/Math/Headers/MathVectors.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Core/Headers/PlatformContextComponent.h"

#include "Rendering/Headers/FrameListener.h"

#include "Platform/Headers/DisplayWindow.h"
#include "Platform/Input/Headers/InputAggregatorInterface.h"

#include <imgui/addons/imguigizmo/ImGuizmo.h>

struct ImDrawData;
namespace Divide {

namespace Attorney {
    class EditorWindowManager;
    class EditorPanelManager;
    class EditorOutputWindow;
};

class MenuBar;
class OutputWindow;
class PanelManager;
class DisplayWindow;
class SceneGraphNode;
class PlatformContext;
class ApplicationOutput;

FWD_DECLARE_MANAGED_CLASS(Texture);
FWD_DECLARE_MANAGED_CLASS(ShaderProgram);

struct SizeChangeParams;

struct TransformSettings {
    ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::ROTATE;
    ImGuizmo::MODE currentGizmoMode = ImGuizmo::WORLD;
    bool useSnap = false;
    F32 snap[3] = { 1.f, 1.f, 1.f };
};

class Editor : public PlatformContextComponent,
               public FrameListener,
               public Input::InputAggregatorInterface {

    friend class Attorney::EditorWindowManager;
    friend class Attorney::EditorPanelManager;
    friend class Attorney::EditorOutputWindow;

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

    bool init(const vec2<U16>& renderResolution);
    void close();
    void idle();
    void update(const U64 deltaTimeUS);

    void toggle(const bool state);
    bool running() const;

    bool shouldPauseSimulation() const;

    inline const Rect<I32>& getScenePreviewRect() const { return _scenePreviewRect; }

    void onSizeChange(const SizeChangeParams& params);
    void selectionChangeCallback(PlayerIndex idx, SceneGraphNode* node);

  protected: //frame listener
    bool frameStarted(const FrameEvent& evt);
    bool framePreRenderStarted(const FrameEvent& evt);
    bool framePreRenderEnded(const FrameEvent& evt);
    bool frameSceneRenderEnded(const FrameEvent& evt);
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

  protected:
    void drawIMGUIDebug(const U64 deltaTime);
    bool renderGizmos(const U64 deltaTime);
    bool renderMinimal(const U64 deltaTime);
    bool renderFull(const U64 deltaTime);
    bool needInput();


  protected: // window events
    bool OnClose();
    void OnFocus(bool bHasFocus);
    void OnSize(int iWidth, int iHeight);
    void OnUTF8(const char* text);
    void dim(bool hovered, bool focused);
    bool toggleScenePreview(bool state);
    void setScenePreviewRect(const Rect<I32>& rect, bool hovered);

  protected: // attorney
    void renderDrawList(ImDrawData* pDrawData, I64 windowGUID);
    void savePanelLayout() const;
    void loadPanelLayout();
    void saveTabLayout() const;
    void loadTabLayout();
    void drawOutputWindow();
    void drawMenuBar();
    void showDebugWindow(bool state);
    void showSampleWindow(bool state);
    bool showDebugWindow() const;
    bool showSampleWindow() const;
    void setTransformSettings(const TransformSettings& settings);

  private:
    Theme _currentTheme;
    Theme _currentLostFocusTheme;
    Theme _currentDimmedTheme;

    Rect<I32> _scenePreviewRect;

    TransformSettings _transformSettings;
    I64 _activeWindowGUID = -1;
    std::unique_ptr<MenuBar> _menuBar;
    std::unique_ptr<PanelManager> _panelManager;
    std::unique_ptr<ApplicationOutput> _applicationOutput;

    bool              _running;
    bool              _sceneHovered;
    bool              _sceneWasHovered;
    bool              _scenePreviewFocused;
    bool              _scenePreviewWasFocused;
    bool              _showDebugWindow;
    bool              _showSampleWindow;
    DisplayWindow*    _mainWindow;
    Texture_ptr       _fontTexture;
    ShaderProgram_ptr _imguiProgram;
    ImGuiContext*     _imguiContext;
    Time::ProfileTimer& _editorUpdateTimer;
    Time::ProfileTimer& _editorRenderTimer;

    std::array<vectorImpl<I64>, to_base(WindowEvent::COUNT)> _windowListeners;
    vectorImpl<SceneGraphNode*> _selectedNodes;

    size_t _consoleCallbackIndex;
}; //Editor

namespace Attorney {
    class EditorOutputWindow {
        private:
        static void drawOutputWindow(Editor& editor) {
            editor.drawOutputWindow();
        }

        friend class Divide::OutputWindow;
    };

    class EditorPanelManager {
        //private:
        public: //ToDo: fix this -Ionut
        static void setScenePreviewRect(Editor& editor, const Rect<I32>& rect, bool hovered) {
            editor.setScenePreviewRect(rect, hovered);
        }

        static void setTransformSettings(Editor& editor, const TransformSettings& settings) {
            editor.setTransformSettings(settings);
        }

        static void savePanelLayout(const Editor& editor) {
            editor.savePanelLayout();
        }
        static void loadPanelLayout(Editor& editor) {
            editor.loadPanelLayout();
        }
        static void saveTabLayout(const Editor& editor) {
            editor.saveTabLayout();
        }
        static void loadTabLayout(Editor& editor) {
            editor.loadTabLayout();
        }
        static void showDebugWindow(Editor& editor, bool state) {
            editor.showDebugWindow(state);
        }
        static void showSampleWindow(Editor& editor, bool state) {
            editor.showSampleWindow(state);
        }
        static bool showDebugWindow(Editor& editor) {
            return editor.showDebugWindow();
        }
        static bool showSampleWindow(Editor& editor) {
            return editor.showSampleWindow();
        }

        friend class Divide::MenuBar;
        friend class Divide::PanelManager;
    };
};


}; //namespace Divide

#endif //_DIVIDE_EDITOR_H_