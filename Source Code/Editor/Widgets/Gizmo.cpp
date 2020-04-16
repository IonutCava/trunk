#include "stdafx.h"

#include "Headers/Gizmo.h"
#include "Editor/Headers/Editor.h"
#include "Core/Headers/PlatformContext.h"
#include "Managers/Headers/SceneManager.h"

#include <imgui_internal.h>

namespace Divide {
    Gizmo::Gizmo(Editor& parent, ImGuiContext* targetContext)
        : _parent(parent),
          _imguiContext(targetContext),
          _visible(false),
          _enabled(false),
          _wasUsed(false),
          _shouldRegisterUndo(false)
    {
        _undoEntry._name = "Gizmo Manipulation";
    }

    Gizmo::~Gizmo()
    {
        _imguiContext= nullptr;
    }

    ImGuiContext& Gizmo::getContext() {
        assert(_imguiContext != nullptr);
        return *_imguiContext;
    }

    const ImGuiContext& Gizmo::getContext() const {
        assert(_imguiContext != nullptr);
        return *_imguiContext;
    }

    void Gizmo::enable(bool state) {
        _enabled = state;
    }

    bool Gizmo::enabled() const {
        return _enabled;
    }

    bool Gizmo::active() const {
        return enabled() && !_selectedNodes.empty();
    }

    void Gizmo::update(const U64 deltaTimeUS) {
        ACKNOWLEDGE_UNUSED(deltaTimeUS);

        if (_shouldRegisterUndo) {
            _parent.registerUndoEntry(_undoEntry);
            _shouldRegisterUndo = false;
        }
    }

    void Gizmo::render(const Camera& camera) {
        if (!active()) {
            return;
        }

        SceneGraphNode* sgn = _selectedNodes.front();
        if (sgn == nullptr) {
            return;
        }

        TransformComponent* const transform = sgn->get<TransformComponent>();
        if (transform == nullptr) {
            return;
        }

        ImGui::SetCurrentContext(_imguiContext);
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();

        ImGuiViewport* main_viewport = ImGui::GetMainViewport();

        DisplayWindow* mainWindow = static_cast<DisplayWindow*>(main_viewport->PlatformHandle);
        const vec2<U16> size = mainWindow->getDrawableSize();
        ImGuizmo::SetRect(0.0f, 0.0f, size.width, size.height);

        mat4<F32> matrix = {};
        transform->getLocalMatrix(matrix);
        ImGuizmo::Manipulate(camera.getViewMatrix(),
                             camera.getProjectionMatrix(),
                             _transformSettings.currentGizmoOperation,
                             _transformSettings.currentGizmoMode,
                             matrix,
                             NULL,
                             _transformSettings.useSnap ? &_transformSettings.snap[0] : NULL);

        if (ImGuizmo::IsUsing()) {
            if (!_wasUsed) {
                _undoEntry.oldVal = transform->getValues();
            }
            _wasUsed = true;
            //ToDo: This seems slow as hell, but it works. Should I bother? -Ionut
            TransformValues values;  vec3<F32> euler;
            ImGuizmo::DecomposeMatrixToComponents(matrix, values._translation, euler._v, values._scale);
            matrix.orthoNormalize();
            values._orientation.fromMatrix(mat3<F32>(matrix));
            transform->setTransform(values);

            _undoEntry.newVal = values;
            _undoEntry._dataSetter = [transform](const void* data) {
                if (transform != nullptr) {
                    transform->setTransform(*(TransformValues*)data);
                }
            };
        }

        ImGui::Render();
        Attorney::EditorGizmo::renderDrawList(_parent, ImGui::GetDrawData(), true, -1);
    }

    void Gizmo::updateSelections(const vectorEASTL<SceneGraphNode*>& nodes) {
         _selectedNodes.resize(0);
         for (SceneGraphNode* node : nodes) {
            if (node->get<TransformComponent>() != nullptr) {
                _selectedNodes.push_back(node);
            }
        }
    }

    void Gizmo::setTransformSettings(const TransformSettings& settings) {
        _transformSettings = settings;
    }

    const TransformSettings& Gizmo::getTransformSettings() const {
        return _transformSettings;
    }

    void Gizmo::onKey(bool pressed, const Input::KeyEvent& key) {
        if (!pressed && _wasUsed) {
            _shouldRegisterUndo = true;
            _wasUsed = false;
        }

        ImGuiIO& io = _imguiContext->IO;

        if (active() && !io.KeyCtrl) {
            TransformSettings settings = _parent.getTransformSettings();
            if (key._key == Input::KeyCode::KC_T) {
                settings.currentGizmoOperation = ImGuizmo::TRANSLATE;
            } else if (key._key == Input::KeyCode::KC_R) {
                settings.currentGizmoOperation = ImGuizmo::ROTATE;
            } else if (key._key == Input::KeyCode::KC_S) {
                settings.currentGizmoOperation = ImGuizmo::SCALE;
            }
            _parent.setTransformSettings(settings);
        }
    }

    void Gizmo::onMouseButton(bool presseed) {
        if (presseed) {
            _wasUsed = false;
        }
    }

    bool Gizmo::needsMouse() const {
        if (active()) {
            ImGuiContext* crtContext = ImGui::GetCurrentContext();
            ImGui::SetCurrentContext(_imguiContext);
            bool imguizmoState = ImGuizmo::IsOver();
            ImGui::SetCurrentContext(crtContext);
            return imguizmoState;
        }

        return false;
    }

}; //namespace Divide