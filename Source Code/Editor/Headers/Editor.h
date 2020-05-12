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
    class EditorOptionsWindow;
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
class EditorOptionsWindow;
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

void InitBasicImGUIState(ImGuiIO& io) noexcept;

class Editor : public PlatformContextComponent,
               public FrameListener,
               public Input::InputAggregatorInterface {

    friend class Attorney::EditorGizmo;
    friend class Attorney::EditorMenuBar;
    friend class Attorney::EditorOutputWindow;
    friend class Attorney::EditorGeneralWidget;
    friend class Attorney::EditorOptionsWindow;
    friend class Attorney::EditorWindowManager;
    friend class Attorney::EditorPropertyWindow;
    friend class Attorney::EditorSceneViewWindow;
    friend class Attorney::EditorSolutionExplorerWindow;

  public:
    static std::array<Input::MouseButton, 5> g_oisButtons;

    enum class WindowType : U8 {
        PostFX = 0,
        SolutionExplorer,
        Properties,
        ContentExplorer,
        Output,
        SceneView,
        COUNT
    };

      enum class ImGuiContextType : U8 {
          Gizmo = 0,
          Editor = 1,
          COUNT
    };
  public:
    explicit Editor(PlatformContext& context, ImGuiStyleEnum theme = ImGuiStyleEnum::ImGuiStyle_DarkCodz01);
    ~Editor();

    bool init(const vec2<U16>& renderResolution);
    void close();
    void idle();
    void update(const U64 deltaTimeUS);
    /// Render any editor specific element that needs to be part of the scene (e.g. Control Gizmo)
    void drawScreenOverlay(const Camera& camera, const Rect<I32>& targetViewport, GFX::CommandBuffer& bufferInOut);

    void toggle(const bool state);
    void onSizeChange(const SizeChangeParams& params);
    void selectionChangeCallback(PlayerIndex idx, const vectorEASTL<SceneGraphNode*>& node);

    bool Undo();
    inline size_t UndoStackSize() const noexcept;

    bool Redo();
    inline size_t RedoStackSize() const noexcept;

    const Rect<I32>& scenePreviewRect(bool globalCoords) const;
    bool wantsMouse() const;
    bool wantsKeyboard() const;
    bool wantsGamepad() const;

    template<typename T>
    inline void registerUndoEntry(const UndoEntry<T>& entry);

    inline bool simulationPauseRequested() const noexcept;
    inline void setTransformSettings(const TransformSettings& settings) noexcept;
    inline const TransformSettings& getTransformSettings() const noexcept;
    inline const Rect<I32>& getTargetViewport() const noexcept;
    inline bool running() const noexcept;
    inline bool inEditMode() const noexcept;
    inline bool scenePreviewFocused() const noexcept;
    inline bool scenePreviewHovered() const noexcept;

    void showStatusMessage(const stringImpl& message, F32 durationMS);

  protected: //frame listener
    bool frameStarted(const FrameEvent& evt) override;
    bool framePreRenderStarted(const FrameEvent& evt) override;
    bool framePreRenderEnded(const FrameEvent& evt) override;
    bool frameSceneRenderEnded(const FrameEvent& evt) override;
    bool frameRenderingQueued(const FrameEvent& evt) override;
    bool framePostRenderStarted(const FrameEvent& evt) override;
    bool framePostRenderEnded(const FrameEvent& evt) override;
    bool frameEnded(const FrameEvent& evt) override;

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
    inline bool isInit() const noexcept;
    bool render(const U64 deltaTime);
    void teleportToNode(const SceneGraphNode& sgn) const;
    void queueRemoveNode(I64 nodeGUID);

    void scenePreviewFocused(bool state);
    ImGuiViewport* findViewportByPlatformHandle(ImGuiContext* context, DisplayWindow* window);

    U32 saveItemCount() const  noexcept;
    PROPERTY_R_IW(bool, unsavedSceneChanges, false);

  protected: // attorney
    void renderDrawList(ImDrawData* pDrawData, const Rect<I32>& targetViewport, I64 windowGUID, GFX::CommandBuffer& bufferInOut);

    bool saveSceneChanges(DELEGATE<void, const char*> msgCallback = {}, DELEGATE<void, bool> finishCallback = {});
    void updateCameraSnapshot();
    // Returns true if the window was closed
    bool modalTextureView(const char* modalName, const Texture_ptr& tex, const vec2<F32>& dimensions, bool preserveAspect, bool useModal);
    // Returns true if the modal window was closed
    bool modalModelSpawn(const char* modalName, const Mesh_ptr& mesh);
    // Return true if the model was spawned as a scene node
    bool spawnGeometry(const Mesh_ptr& mesh, const vec3<F32>& scale, const stringImpl& name);

    LightPool& getActiveLightPool();

    inline void toggleMemoryEditor(bool state) noexcept;
    inline void setSelectedCamera(Camera* camera) noexcept;
    inline Camera* getSelectedCamera() const noexcept;

    bool addComponent(SceneGraphNode* selection, ComponentType newComponentType) const;
    bool addComponent(const Selections& selections, ComponentType newComponentType) const;
    bool removeComponent(SceneGraphNode* selection, ComponentType newComponentType) const;
    bool removeComponent(const Selections& selections, ComponentType newComponentType) const;
    SceneNode_ptr createNode(SceneNodeType type, const ResourceDescriptor& descriptor);

  private:
    Time::ProfileTimer& _editorUpdateTimer;
    Time::ProfileTimer& _editorRenderTimer;

    std::unique_ptr<MenuBar> _menuBar = nullptr;
    std::unique_ptr<StatusBar> _statusBar = nullptr;
    std::unique_ptr<EditorOptionsWindow> _optionsWindow = nullptr;
    std::unique_ptr<Gizmo> _gizmo = nullptr;

    Camera*           _selectedCamera = nullptr;
    DisplayWindow*    _mainWindow = nullptr;
    Texture_ptr       _fontTexture = nullptr;
    ShaderProgram_ptr _imguiProgram = nullptr;
    std::unique_ptr<UndoManager>  _undoManager = nullptr;

    std::pair<bufferPtr, size_t> _memoryEditorData = { nullptr, 0 };

    std::array<ImGuiContext*, to_base(ImGuiContextType::COUNT)> _imguiContexts = {};
    std::array<DockedWindow*, to_base(WindowType::COUNT)> _dockedWindows = {};

    hashMap<I64, CameraSnapshot> _cameraSnapshots;

    Rect<I32>         _targetViewport = {0, 0, 1, 1};
    U32               _stepQueue = 1u;

    ImGuiStyleEnum _currentTheme = ImGuiStyle_Count;

    stringImpl        _externalTextEditorPath = "";
    bool              _autoSaveCamera = false;
    bool              _autoFocusEditor = true;
    bool              _showSampleWindow = false;
    bool              _showOptionsWindow = false;
    bool              _showMemoryEditor = false;
    bool              _running = false;
    bool              _isScenePaused = false;
    bool              _sceneHovered = false;
    bool              _scenePreviewFocused = false;

}; //Editor

namespace Attorney {
    class EditorGizmo {
    private:
        static void renderDrawList(Editor& editor, ImDrawData* pDrawData, const Rect<I32>& targetViewport, I64 windowGUID, GFX::CommandBuffer& bufferInOut) {
            editor.renderDrawList(pDrawData, targetViewport, windowGUID, bufferInOut);
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

        static void editorStepQueue(Editor& editor, const U32 steps) noexcept {
            editor._stepQueue = steps;
        }

        static bool autoSaveCamera(const Editor& editor) noexcept {
            return editor._autoSaveCamera;
        }

        static void autoSaveCamera(Editor& editor, const bool state) noexcept {
            editor._autoSaveCamera = state;
        } 
        
        static bool autoFocusEditor(const Editor& editor) noexcept {
            return editor._autoFocusEditor;
        }

        static void autoFocusEditor(Editor& editor, const bool state) noexcept {
            editor._autoFocusEditor = state;
        }

        friend class Divide::SceneViewWindow;
    };

    class EditorSolutionExplorerWindow {
    private :
        static void setSelectedCamera(Editor& editor, Camera* camera)  noexcept {
            editor.setSelectedCamera(camera);
        }

        static Camera* getSelectedCamera(const Editor& editor)  noexcept {
            return editor.getSelectedCamera();
        }

        static bool editorEnableGizmo(const Editor& editor) noexcept {
            return editor._gizmo->enabled();
        }

        static void editorEnableGizmo(const Editor& editor, bool state) noexcept {
            editor._gizmo->enable(state);
        }

        static void teleportToNode(const Editor& editor, const SceneGraphNode& targetNode) {
            editor.teleportToNode(targetNode);
        }

        static void queueRemoveNode(Editor& editor, I64 nodeGUID) {
            editor.queueRemoveNode(nodeGUID);
        }

        static SceneNode_ptr createNode(Editor& editor, SceneNodeType type, const ResourceDescriptor& descriptor) {
            return editor.createNode(type, descriptor);
        }

        friend class Divide::SolutionExplorerWindow;
    };

    class EditorPropertyWindow {
    private :

        static void setSelectedCamera(Editor& editor, Camera* camera)  noexcept {
            editor.setSelectedCamera(camera);
        }

        static Camera* getSelectedCamera(const Editor& editor)  noexcept {
            return editor.getSelectedCamera();
        }

        friend class Divide::PropertyWindow;
    };


    class EditorOptionsWindow {
    private :
        static ImGuiStyleEnum getTheme(const Editor& editor) noexcept {
            return editor._currentTheme;
        }

        static void setTheme(Editor& editor, const ImGuiStyleEnum newTheme) noexcept {
            editor._currentTheme = newTheme;
        }

        static const stringImpl& externalTextEditorPath(Editor& editor) noexcept {
            return editor._externalTextEditorPath;
        }

        static void externalTextEditorPath(Editor& editor, const stringImpl& path) noexcept {
            editor._externalTextEditorPath = path;
        }

        friend class Divide::EditorOptionsWindow;
    };

    class EditorMenuBar {
    private:
        static void toggleMemoryEditor(Editor& editor, bool state)  noexcept {
            editor.toggleMemoryEditor(state);
        }

        static bool memoryEditorEnabled(const Editor& editor) noexcept {
            return editor._showMemoryEditor;
        }

        static bool& sampleWindowEnabled(Editor& editor) noexcept {
            return editor._showSampleWindow;
        }

        static bool& optionWindowEnabled(Editor& editor) noexcept {
            return editor._showOptionsWindow;
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

        static U32 saveItemCount(const Editor& editor) noexcept {
            return editor.saveItemCount();
        }

        static bool hasUnsavedSceneChanges(const Editor& editor) noexcept {
            return editor.unsavedSceneChanges();
        }

        static void registerUnsavedSceneChanges(Editor& editor) noexcept {
            editor.unsavedSceneChanges(true);
        }

        static bool saveSceneChanges(Editor& editor, DELEGATE<void, const char*> msgCallback = {}, DELEGATE<void, bool> finishCallback = {}) {
            return editor.saveSceneChanges(msgCallback, finishCallback);
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

        static ImGuiContext& getImGuiContext(Editor& editor, Editor::ImGuiContextType type) noexcept {
            return *editor._imguiContexts[to_base(type)];
        }

        static ImGuiContext& imguizmoContext(Editor& editor, Editor::ImGuiContextType type) noexcept {
            return *editor._imguiContexts[to_base(type)];
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

        static void showStatusMessage(Editor& editor, const stringImpl& message, F32 durationMS) {
            editor.showStatusMessage(message, durationMS);
        }

        static const stringImpl& externalTextEditorPath(Editor& editor) noexcept {
            return editor._externalTextEditorPath;
        }

        friend class Divide::Gizmo;
        friend class Divide::MenuBar;
        friend class Divide::PropertyWindow;
        friend class Divide::EditorOptionsWindow;
        friend class Divide::ContentExplorerWindow;
        friend class Divide::SolutionExplorerWindow;
    };
};

void PushReadOnly() noexcept;
void PopReadOnly() noexcept;

}; //namespace Divide

#endif //_DIVIDE_EDITOR_H_

#include "Editor.inl"