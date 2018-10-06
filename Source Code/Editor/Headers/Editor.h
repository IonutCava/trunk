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
#ifndef _DIVIDE_EDITOR_H_
#define _DIVIDE_EDITOR_H_

#include "Core/Math/Headers/MathVectors.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Core/Headers/PlatformContextComponent.h"

#include "Rendering/Headers/FrameListener.h"

#include "Platform/Headers/DisplayWindow.h"
#include "Platform/Input/Headers/InputAggregatorInterface.h"

#include <imgui/addons/imguigizmo/ImGuizmo.h>
#include <imgui/addons/imguistyleserializer/imguistyleserializer.h>

struct ImDrawData;
namespace Divide {

namespace Attorney {
    class EditorPanelManager;
    class EditorOutputWindow;
    class EditorWindowManager;
    class EditorPropertyWindow;
    class EditorSceneViewWindow;
    class EditorSolutionExplorerWindow;
};

class Camera;
class MenuBar;
class DockedWindow;
class OutputWindow;
class PanelManager;
class DisplayWindow;
class PropertyWindow;
class SceneGraphNode;
class SceneViewWindow;
class PlatformContext;
class ApplicationOutput;
class SolutionExplorerWindow;

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

    friend class Attorney::EditorPanelManager;
    friend class Attorney::EditorOutputWindow;
    friend class Attorney::EditorWindowManager;
    friend class Attorney::EditorPropertyWindow;
    friend class Attorney::EditorSceneViewWindow;
    friend class Attorney::EditorSolutionExplorerWindow;

  public:
    enum class Context : U8 {
        Editor = 0,
        Gizmo,
        COUNT
    };

    enum class WindowType : U8 {
        SolutionExplorer = 0,
        Properties,
        Output,
        SceneView,
        COUNT
    };

  public:
    explicit Editor(PlatformContext& context,
                    ImGuiStyleEnum theme = ImGuiStyleEnum::ImGuiStyle_GrayCodz01,
                    ImGuiStyleEnum dimmedTheme = ImGuiStyleEnum::ImGuiStyle_GrayCodz01Inverse);
    ~Editor();

    bool init(const vec2<U16>& renderResolution);
    void close();
    void idle();
    void update(const U64 deltaTimeUS);
    bool needInput() const;

    void toggle(const bool state);
    bool running() const;

    bool shouldPauseSimulation() const;

    void onSizeChange(const SizeChangeParams& params);
    void selectionChangeCallback(PlayerIndex idx, SceneGraphNode* node);

    bool simulationPauseRequested() const;

    void setTransformSettings(const TransformSettings& settings);
    const TransformSettings& getTransformSettings() const;

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
    bool hasSceneFocus();
    bool hasSceneFocus(bool& gizmoFocus);
    bool hasGizmoFocus();
    ImGuiIO& GetIO(U8 idx);

  protected: // window events
    bool OnClose();
    void OnFocus(bool bHasFocus);
    void OnSize(I32 iWidth, I32 iHeight);
    void OnUTF8(const char* text);

  protected: // attorney
    void renderDrawList(ImDrawData* pDrawData, bool gizmo);
    void drawOutputWindow();
    void drawMenuBar();
    void showDebugWindow(bool state);
    void showSampleWindow(bool state);
    void enableGizmo(bool state);
    bool showDebugWindow() const;
    bool showSampleWindow() const;
    bool enableGizmo() const;
    void setSelectedCamera(Camera* camera);
    Camera* getSelectedCamera() const;

  private:
    ImGuiStyleEnum _currentTheme;
    ImGuiStyleEnum _currentDimmedTheme;

    TransformSettings _transformSettings;
    std::unique_ptr<MenuBar> _menuBar;
    std::unique_ptr<ApplicationOutput> _applicationOutput;

    bool              _running;
    bool              _sceneHovered;
    bool              _gizmosVisible;
    bool              _scenePreviewFocused;
    bool              _showDebugWindow;
    bool              _showSampleWindow;
    bool              _enableGizmo;
    Camera*           _selectedCamera;
    DisplayWindow*    _mainWindow;
    Texture_ptr       _fontTexture;
    ShaderProgram_ptr _imguiProgram;
    Time::ProfileTimer& _editorUpdateTimer;
    Time::ProfileTimer& _editorRenderTimer;

    std::array<ImGuiContext*, to_base(Context::COUNT)>     _imguiContext;
    std::array<vector<I64>, to_base(WindowEvent::COUNT)> _windowListeners;
    vector<SceneGraphNode*> _selectedNodes;
    size_t _consoleCallbackIndex;
    bool* _simulationPaused;
    U32 _sceneStepCount;
    std::array<DockedWindow*, to_base(WindowType::COUNT)> _dockedWindows;
}; //Editor

namespace Attorney {
    class EditorOutputWindow {
        private:
        static void drawOutputWindow(Editor& editor) {
            editor.drawOutputWindow();
        }

        friend class Divide::OutputWindow;
    };

    class EditorSceneViewWindow {
    private:

        friend class Divide::SceneViewWindow;
    };

    class EditorSolutionExplorerWindow {
    private :
        static void setSelectedCamera(Editor& editor, Camera* camera) {
            editor.setSelectedCamera(camera);
        }

        static Camera* getSelectedCamera(Editor& editor) {
            return editor.getSelectedCamera();
        }

        friend class Divide::SolutionExplorerWindow;
    };

    class EditorPropertyWindow {
    private :
        static bool editorEnableGizmo(Editor& editor) {
            return editor.enableGizmo();
        }

        static void editorEnableGizmo(Editor& editor, bool state) {
            editor.enableGizmo(state);
        }

        static void setSelectedCamera(Editor& editor, Camera* camera) {
            editor.setSelectedCamera(camera);
        }

        static Camera* getSelectedCamera(Editor& editor) {
            return editor.getSelectedCamera();
        }

        friend class Divide::PropertyWindow;
    };

    class EditorPanelManager {
        //private:
        public: //ToDo: fix this -Ionut
        static void setTransformSettings(Editor& editor, const TransformSettings& settings) {
            editor.setTransformSettings(settings);
        }

        static const TransformSettings& getTransformSettings(const Editor& editor) {
            return editor.getTransformSettings();
        }
        static void showDebugWindow(Editor& editor, bool state) {
            editor.showDebugWindow(state);
        }
        static void showSampleWindow(Editor& editor, bool state) {
            editor.showSampleWindow(state);
        }
        static void enableGizmo(Editor& editor, bool state) {
            return editor.enableGizmo(state);
        }
        static bool showDebugWindow(Editor& editor) {
            return editor.showDebugWindow();
        }
        static bool showSampleWindow(Editor& editor) {
            return editor.showSampleWindow();
        }
        static bool enableGizmo(Editor& editor) {
            return editor.enableGizmo();
        }
        
        friend class Divide::MenuBar;
        friend class Divide::PanelManager;
    };
};


}; //namespace Divide

#endif //_DIVIDE_EDITOR_H_