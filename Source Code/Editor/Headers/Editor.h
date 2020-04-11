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
class StatusBar;
class LightPool;
class UndoManager;
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

struct Selections;
struct SizeChangeParams;
struct TransformSettings;

template<typename T>
struct UndoEntry;

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
    static std::array<Input::MouseButton, 5> g_oisButtons;

    enum class WindowType : U8 {
        SolutionExplorer = 0,
        PostFX,
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

    void toggle(const bool state);
    void onSizeChange(const SizeChangeParams& params);
    void selectionChangeCallback(PlayerIndex idx, const vectorEASTL<SceneGraphNode*>& node);

    bool Undo();
    bool Redo();

    const Rect<I32>& scenePreviewRect(bool globalCoords) const;
    bool wantsMouse() const;
    bool wantsKeyboard() const;
    bool wantsGamepad() const;

    template<typename T>
    inline void registerUndoEntry(const UndoEntry<T>& entry);
    inline bool simulationPauseRequested() const;
    inline void setTransformSettings(const TransformSettings& settings);
    inline const TransformSettings& getTransformSettings() const;
    inline const Rect<I32>& getTargetViewport() const;
    inline bool running() const;
    inline bool scenePreviewFocused() const;
    inline bool scenePreviewHovered() const;

  protected: //frame listener
    bool frameStarted(const FrameEvent& evt) noexcept override;
    bool framePreRenderStarted(const FrameEvent& evt) noexcept override;
    bool framePreRenderEnded(const FrameEvent& evt) noexcept override;
    bool frameSceneRenderEnded(const FrameEvent& evt) noexcept override;
    bool frameRenderingQueued(const FrameEvent& evt) noexcept override;
    bool framePostRenderStarted(const FrameEvent& evt) noexcept override;
    bool framePostRenderEnded(const FrameEvent& evt) noexcept override;
    bool frameEnded(const FrameEvent& evt) noexcept override;

  public: // input
    /// Key pressed: return true if input was consumed
    bool onKeyDown(const Input::KeyEvent& key) override;
    /// Key released: return true if input was consumed
    bool onKeyUp(const Input::KeyEvent& key) override;
    /// Mouse moved: return true if input was consumed
    bool mouseMoved(const Input::MouseMoveEvent& arg) override;
    /// Mouse button pressed: return true if input was consumed
    bool mouseButtonPressed(const Input::MouseButtonEvent& arg) override;
    /// Mouse button released: return true if input was consumed
    bool mouseButtonReleased(const Input::MouseButtonEvent& arg) override;

    bool joystickButtonPressed(const Input::JoystickEvent &arg) override;
    bool joystickButtonReleased(const Input::JoystickEvent &arg) override;
    bool joystickAxisMoved(const Input::JoystickEvent &arg) override;
    bool joystickPovMoved(const Input::JoystickEvent &arg) override;
    bool joystickBallMoved(const Input::JoystickEvent &arg) override;
    bool joystickAddRemove(const Input::JoystickEvent &arg) override;
    bool joystickRemap(const Input::JoystickEvent &arg) override;
    bool onUTF8(const Input::UTF8Event& arg) override;
        
    bool saveToXML() const;
    bool loadFromXML();

  protected:
    inline bool isInit() const;
    bool render(const U64 deltaTime);
    void updateMousePosAndButtons();
    void teleportToNode(SceneGraphNode* sgn) const;

    void scenePreviewFocused(bool state);
    ImGuiViewport* findViewportByPlatformHandle(ImGuiContext* context, DisplayWindow* window);
    ImGuiContext& imguizmoContext();

    inline ImGuiContext& imguiContext();

    PROPERTY_R_IW(bool, unsavedSceneChanges, false);

  protected: // attorney
    void renderDrawList(ImDrawData* pDrawData, bool overlayOnScene, I64 windowGUID);
    void drawMenuBar();
    void drawStatusBar();

    bool saveSceneChanges();
    void updateCameraSnapshot();
    // Returns true if the window was closed
    bool modalTextureView(const char* modalName, const Texture_ptr& tex, const vec2<F32>& dimensions, bool preserveAspect, bool useModal);
    // Returns true if the modal window was closed
    bool modalModelSpawn(const char* modalName, const Mesh_ptr& mesh);
    // Return true if the model was spawned as a scene node
    bool spawnGeometry(const Mesh_ptr& mesh, const vec3<F32>& scale, const stringImpl& name);

    LightPool& getActiveLightPool();

    inline void toggleMemoryEditor(bool state);
    inline void setSelectedCamera(Camera* camera);
    inline Camera* getSelectedCamera() const;

    bool addComponent(SceneGraphNode* selection, ComponentType newComponentType) const;
    bool addComponent(const Selections& selections, ComponentType newComponentType) const;
    bool removeComponent(SceneGraphNode* selection, ComponentType newComponentType) const;
    bool removeComponent(const Selections& selections, ComponentType newComponentType) const;

  private:
    Time::ProfileTimer& _editorUpdateTimer;
    Time::ProfileTimer& _editorRenderTimer;

    std::unique_ptr<MenuBar> _menuBar = nullptr;
    std::unique_ptr<StatusBar> _statusBar = nullptr;
    std::unique_ptr<Gizmo> _gizmo = nullptr;

    Camera*           _selectedCamera = nullptr;
    DisplayWindow*    _mainWindow = nullptr;
    ImGuiContext*     _imguiContext = nullptr;
    Texture_ptr       _fontTexture = nullptr;
    ShaderProgram_ptr _imguiProgram = nullptr;
    std::unique_ptr<UndoManager>  _undoManager = nullptr;

    std::pair<bufferPtr, size_t> _memoryEditorData = { nullptr, 0 };

    std::array<DockedWindow*, to_base(WindowType::COUNT)> _dockedWindows;

    hashMap<I64, CameraSnapshot> _cameraSnapshots;

    Rect<I32>         _targetViewport = {0, 0, 1, 1};
    U32               _stepQueue = 1u;

    ImGuiStyleEnum _currentTheme = ImGuiStyle_Count;
    ImGuiStyleEnum _currentDimmedTheme = ImGuiStyle_Count;

    bool              _autoSaveCamera = false;
    bool              _showSampleWindow = false;
    bool              _showMemoryEditor = false;
    bool              _running = false;
    bool              _sceneHovered = false;
    bool              _scenePreviewFocused = false;

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

        static void editorStepQueue(Editor& editor, const U32 steps) {
            editor._stepQueue = steps;
        }

        static bool autoSaveCamera(const Editor& editor) {
            return editor._autoSaveCamera;
        }

        static void autoSaveCamera(Editor& editor, const bool state) {
            editor._autoSaveCamera = state;
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

        static void teleportToNode(const Editor& editor, SceneGraphNode* targetNode) {
            editor.teleportToNode(targetNode);
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

        static LightPool& getActiveLightPool(Editor& editor) {
            return editor.getActiveLightPool();
        }

        static void enableGizmo(const Editor& editor, bool state) {
            return editor._gizmo->enable(state);
        }
      
        static bool enableGizmo(const Editor& editor) {
            return editor._gizmo->enabled();
        }

        static bool hasUnsavedSceneChanges(const Editor& editor) noexcept {
            return editor.unsavedSceneChanges();
        }

        static void registerUnsavedSceneChanges(Editor& editor) noexcept {
            editor.unsavedSceneChanges(true);
        }

        static bool saveSceneChanges(Editor& editor) {
            return editor.saveSceneChanges();
        }

        static void inspectMemory(Editor& editor, std::pair<bufferPtr, size_t> data) noexcept {
            editor._memoryEditorData = data;
        }

        static bool modalTextureView(Editor& editor, const char* modalName, const Texture_ptr& tex, const vec2<F32>& dimensions, bool preserveAspect, bool useModal) {
            return editor.modalTextureView(modalName, tex, dimensions, preserveAspect, useModal);
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

        static bool addComponent(const Editor& editor, const Selections& selections, ComponentType newComponentType) {
            return editor.addComponent(selections, newComponentType);
        }

        static bool removeComponent(const Editor& editor, SceneGraphNode* selection, ComponentType newComponentType) {
            return editor.removeComponent(selection, newComponentType);
        }

        static bool removeComponent(const Editor& editor, const Selections& selections, ComponentType newComponentType) {
            return editor.removeComponent(selections, newComponentType);
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

#include "Editor.inl"