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
#include "Rendering/Camera/Headers/CameraSnapshot.h"

#include "Editor/Widgets/Headers/Gizmo.h"

#include "Platform/Headers/DisplayWindow.h"
#include "Platform/Input/Headers/InputAggregatorInterface.h"

#include <imgui/addons/imguistyleserializer/imguistyleserializer.h>

struct ImDrawData;

namespace Divide {

namespace Attorney {
    class EditorGizmo;
    class EditorMenuBar;
    class EditorOutputWindow;
    class EditorGeneralWidget;
    class EditorWindowManager;
    class EditorPropertyWindow;
    class EditorSceneViewWindow;
    class EditorSolutionExplorerWindow;
};

class Gizmo;
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
class ContentExplorerWindow;
class SolutionExplorerWindow;

FWD_DECLARE_MANAGED_CLASS(Mesh);
FWD_DECLARE_MANAGED_CLASS(Texture);
FWD_DECLARE_MANAGED_CLASS(ShaderProgram);

struct SizeChangeParams;
struct TransformSettings;

void InitBasicImGUIState(ImGuiIO& io);

class Editor : public PlatformContextComponent,
               public FrameListener,
               public Input::InputAggregatorInterface {

    friend class Attorney::EditorGizmo;
    friend class Attorney::EditorMenuBar;
    friend class Attorney::EditorOutputWindow;
    friend class Attorney::EditorGeneralWidget;
    friend class Attorney::EditorWindowManager;
    friend class Attorney::EditorPropertyWindow;
    friend class Attorney::EditorSceneViewWindow;
    friend class Attorney::EditorSolutionExplorerWindow;

  public:
    enum class WindowType : U8 {
        SolutionExplorer = 0,
        Properties,
        ContentExplorer,
        Output,
        SceneView,
        COUNT
    };

  public:
    explicit Editor(PlatformContext& context,
                    ImGuiStyleEnum theme = ImGuiStyleEnum::ImGuiStyle_GrayCodz01,
                    ImGuiStyleEnum dimmedTheme = ImGuiStyleEnum::ImGuiStyle_EdinBlack);
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

    const Rect<I32>& scenePreviewRect(bool globalCoords) const;
    bool scenePreviewFocused() const;

    const Rect<I32>& getTargetViewport() const;

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
    bool mouseMoved(const Input::MouseMoveEvent& arg);
    /// Mouse button pressed: return true if input was consumed
    bool mouseButtonPressed(const Input::MouseButtonEvent& arg);
    /// Mouse button released: return true if input was consumed
    bool mouseButtonReleased(const Input::MouseButtonEvent& arg);

    bool joystickButtonPressed(const Input::JoystickEvent &arg) override;
    bool joystickButtonReleased(const Input::JoystickEvent &arg) override;
    bool joystickAxisMoved(const Input::JoystickEvent &arg) override;
    bool joystickPovMoved(const Input::JoystickEvent &arg) override;
    bool joystickBallMoved(const Input::JoystickEvent &arg) override;
    bool joystickAddRemove(const Input::JoystickEvent &arg) override;
    bool joystickRemap(const Input::JoystickEvent &arg) override;
    bool onUTF8(const Input::UTF8Event& arg) override;
        
    void saveToXML() const;
    void loadFromXML();

  protected:
    bool render(const U64 deltaTime);
    void updateMousePosAndButtons();

    void scenePreviewFocused(bool state);
    ImGuiViewport* findViewportByPlatformHandle(ImGuiContext* context, DisplayWindow* window);
    ImGuiContext& imguiContext();
    ImGuiContext& imguizmoContext();

  protected: // attorney
    void renderDrawList(ImDrawData* pDrawData, bool overlayOnScene, I64 windowGUID);
    void drawMenuBar();
    void setSelectedCamera(Camera* camera);
    Camera* getSelectedCamera() const;
    bool hasUnsavedElements() const;
    void saveElement(I64 elementGUID);
    void toggleMemoryEditor(bool state);
    void updateCameraSnapshot();
    // Returns true if the modal window was closed
    bool modalTextureView(const char* modalName, const Texture_ptr& tex, const vec2<F32>& dimensions, bool preserveAspect);
    // Returns true if the modal window was closed
    bool modalModelSpawn(const char* modalName, const Mesh_ptr& mesh);
    // Return true if the model was spawned as a scene node
    bool spawnGeometry(const Mesh_ptr& mesh, const vec3<F32>& scale, const stringImpl& name);

  private:
    ImGuiStyleEnum _currentTheme;
    ImGuiStyleEnum _currentDimmedTheme;

    std::unique_ptr<MenuBar> _menuBar;
    std::unique_ptr<Gizmo> _gizmo;
    Rect<I32>         _targetViewport;
    bool              _showSampleWindow;
    bool              _showMemoryEditor;
    bool              _running;
    bool              _sceneHovered;
    bool              _scenePreviewFocused;
    Camera*           _selectedCamera;
    DisplayWindow*    _mainWindow;
    ImGuiContext*     _imguiContext;
    Texture_ptr       _fontTexture;
    ShaderProgram_ptr _imguiProgram;
    Time::ProfileTimer& _editorUpdateTimer;
    Time::ProfileTimer& _editorRenderTimer;

    std::pair<bufferPtr, size_t> _memoryEditorData;

    std::vector<I64> _unsavedElements;
    std::array<DockedWindow*, to_base(WindowType::COUNT)> _dockedWindows;

    hashMap<I64, CameraSnapshot> _cameraSnapshots;

}; //Editor

namespace Attorney {
    class EditorGizmo {
    private:
        static void renderDrawList(Editor& editor, ImDrawData* pDrawData, bool overlayOnScene, I64 windowGUID) {
            editor.renderDrawList(pDrawData, overlayOnScene, windowGUID);
        }

        static ImGuiViewport* findViewportByPlatformHandle(Editor& editor, ImGuiContext* context, DisplayWindow* window) {
            return editor.findViewportByPlatformHandle(context, window);
        }

        friend class Divide::Gizmo;
    };

    class EditorSceneViewWindow {
    private:
        static bool editorEnabledGizmo(const Editor& editor) {
            return editor._gizmo->enabled();
        }

        static void editorEnableGizmo(const Editor& editor, bool state) {
            editor._gizmo->enable(state);
        }

        static void updateCameraSnapshot(Editor& editor) {
            editor.updateCameraSnapshot();
        }

        friend class Divide::SceneViewWindow;
    };

    class EditorSolutionExplorerWindow {
    private :
        static void setSelectedCamera(Editor& editor, Camera* camera) {
            editor.setSelectedCamera(camera);
        }

        static Camera* getSelectedCamera(const Editor& editor) {
            return editor.getSelectedCamera();
        }

        static bool editorEnableGizmo(const Editor& editor) {
            return editor._gizmo->enabled();
        }

        static void editorEnableGizmo(const Editor& editor, bool state) {
            editor._gizmo->enable(state);
        }

        friend class Divide::SolutionExplorerWindow;
    };

    class EditorPropertyWindow {
    private :

        static void setSelectedCamera(Editor& editor, Camera* camera) {
            editor.setSelectedCamera(camera);
        }

        static Camera* getSelectedCamera(const Editor& editor) {
            return editor.getSelectedCamera();
        }

        friend class Divide::PropertyWindow;
    };

    class EditorMenuBar {
    private:
        static void toggleMemoryEditor(Editor& editor, bool state) {
            editor.toggleMemoryEditor(state);
        }

        static bool memoryEditorEnabled(const Editor& editor) noexcept {
            return editor._showMemoryEditor;
        }

        static bool& sampleWindowEnabled(Editor& editor) noexcept {
            return editor._showSampleWindow;
        }

        friend class Divide::MenuBar;
    };

    class EditorGeneralWidget {
      private:
        static void setTransformSettings(Editor& editor, const TransformSettings& settings) {
            editor.setTransformSettings(settings);
        }
        static const TransformSettings& getTransformSettings(const Editor& editor) {
            return editor.getTransformSettings();
        }
        static void enableGizmo(const Editor& editor, bool state) {
            return editor._gizmo->enable(state);
        }
      
        static bool enableGizmo(const Editor& editor) {
            return editor._gizmo->enabled();
        }
        static bool hasUnsavedElements(const Editor& editor) {
            return editor.hasUnsavedElements();
        }
        static void saveElement(Editor& editor, I64 elementGUID = -1) {
            editor.saveElement(elementGUID);
        }

        static void inspectMemory(Editor& editor, std::pair<bufferPtr, size_t> data) noexcept {
            editor._memoryEditorData = data;
        }

        static bool modalTextureView(Editor& editor, const char* modalName, const Texture_ptr& tex, const vec2<F32>& dimensions, bool preserveAspect) {
            return editor.modalTextureView(modalName, tex, dimensions, preserveAspect);
        }

        static bool modalModelSpawn(Editor& editor, const char* modalName, const Mesh_ptr& mesh) {
            return editor.modalModelSpawn(modalName, mesh);
        }

        static ImGuiContext& imguiContext(Editor& editor) {
            return editor.imguiContext();
        }

        static ImGuiContext& imguizmoContext(Editor& editor) {
            return editor.imguizmoContext();
        }

        friend class Divide::Gizmo;
        friend class Divide::MenuBar;
        friend class Divide::PropertyWindow;
        friend class Divide::ContentExplorerWindow;
        friend class Divide::SolutionExplorerWindow;
    };
};


}; //namespace Divide

#endif //_DIVIDE_EDITOR_H_