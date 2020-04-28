#include "stdafx.h"

#include "Headers/Gizmo.h"
#include "Editor/Headers/Editor.h"
#include "Core/Headers/PlatformContext.h"
#include "Managers/Headers/SceneManager.h"

#include <imgui_internal.h>

namespace Divide {
    namespace {
        constexpr U8 g_maxSelectedNodes = 12;
        constexpr F32 g_maxScaleFactor = 100.0f;
        using TransformCache = std::array<TransformValues, g_maxSelectedNodes>;
        using NodeCache = std::array<SceneGraphNode*, g_maxSelectedNodes>;

        TransformCache g_transformCache;
        NodeCache g_selectedNodesCache;
        UndoEntry<std::pair<TransformCache, NodeCache>> g_undoEntry;
    }

    Gizmo::Gizmo(Editor& parent, ImGuiContext* targetContext)
        : _parent(parent),
          _imguiContext(targetContext),
          _visible(false),
          _enabled(false),
          _wasUsed(false),
          _shouldRegisterUndo(false)
    {
        g_undoEntry._name = "Gizmo Manipulation";
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
            _parent.registerUndoEntry(g_undoEntry);
            _shouldRegisterUndo = false;
        }
    }

    void Gizmo::render(const Camera& camera, const Rect<I32>& targetViewport, GFX::CommandBuffer& bufferInOut) {
        if (!active() || _selectedNodes.empty()) {
            return;
        }

        const SceneGraphNode* sgn = _selectedNodes.front();
        if (sgn == nullptr) {
            return;
        }

        TransformComponent* const transform = sgn->get<TransformComponent>();
        if (transform == nullptr) {
            return;
        }

        const bool multiNodeEdit = _selectedNodes.size() > 1;

        vec3<F32> startPos = {};
        mat4<F32> matrix = {};
        BoundingBox nodesBB = {};

        {
            U8 selectionCounter = 0;
            for (auto& node : _selectedNodes) {
                TransformComponent* tComp = node->get<TransformComponent>();
                if (tComp != nullptr) {
                    nodesBB.add(tComp->getLocalPosition());
                    if (++selectionCounter == g_maxSelectedNodes) {
                        break;
                    }
                }
            }
        }

        startPos = nodesBB.getCenter();
        matrix.identity();
        matrix.setTranslation(startPos);

        ImGui::SetCurrentContext(_imguiContext);
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();

        DisplayWindow* mainWindow = static_cast<DisplayWindow*>(ImGui::GetMainViewport()->PlatformHandle);
        const vec2<U16> size = mainWindow->getDrawableSize();
        ImGuizmo::SetRect(0.f, 0.f, to_F32(size.width), to_F32(size.height));

        ImGuizmo::Manipulate(camera.getViewMatrix(),
                             camera.getProjectionMatrix(),
                             _transformSettings.currentGizmoOperation,
                             _transformSettings.currentGizmoMode,
                             matrix,
                             NULL,
                             _transformSettings.useSnap ? &_transformSettings.snap[0] : NULL);

        if (ImGuizmo::IsUsing()) {
            if (!_wasUsed) {
                U8 selectionCounter = 0;
                for (auto& node : _selectedNodes) {
                    TransformComponent* tComp = node->get<TransformComponent>();
                    if (tComp != nullptr) {
                        g_transformCache[selectionCounter] = tComp->getValues();
                        g_selectedNodesCache[selectionCounter] = node;
                        if (++selectionCounter == g_maxSelectedNodes) {
                            break;
                        }
                    }
                }
                
                g_undoEntry.oldVal = { g_transformCache, g_selectedNodesCache };
            }

            //ToDo: This seems slow as hell, but it works. Should I bother? -Ionut
            vec3<F32> euler;
            ImGuizmo::DecomposeMatrixToComponents(matrix, _workValues._translation._v, euler._v, _workValues._scale._v);
            matrix.orthoNormalize();
            _workValues._orientation.fromMatrix(mat3<F32>(matrix));
            CLAMP(_workValues._scale.x, 1 / g_maxScaleFactor, 1 * g_maxScaleFactor);
            CLAMP(_workValues._scale.y, 1 / g_maxScaleFactor, 1 * g_maxScaleFactor);
            CLAMP(_workValues._scale.z, 1 / g_maxScaleFactor, 1 * g_maxScaleFactor);

            _workValues._translation = _workValues._translation - startPos;
            for (auto& node : _selectedNodes) {
                TransformComponent* tComp = node->get<TransformComponent>();
                U8 selectionCounter = 0;
                if (tComp != nullptr) {
                    tComp->scale(_workValues._scale);
                    tComp->rotate(_workValues._orientation);
                    tComp->translate(_workValues._translation);
                    g_transformCache[selectionCounter] = tComp->getValues();
                    if (++selectionCounter == g_maxSelectedNodes) {
                        break;
                    }
                }
            }
            _wasUsed = true;

            g_undoEntry.newVal = { g_transformCache, g_selectedNodesCache };

            g_undoEntry._dataSetter = [](const void* data) {
                const std::pair<TransformCache, NodeCache>& oldData = *(std::pair<TransformCache, NodeCache>*)data;

                for (U8 i = 0; i < g_maxSelectedNodes; ++i) {
                    SceneGraphNode* node = oldData.second[i];
                    if (node != nullptr) {
                        TransformComponent* tComp = node->get<TransformComponent>();
                        if (tComp != nullptr) {
                            tComp->setTransform(oldData.first[i]);
                        }
                    }
                }
            };
        }

        ImGui::Render();
        Attorney::EditorGizmo::renderDrawList(_parent, ImGui::GetDrawData(), targetViewport, -1, bufferInOut);
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

    bool Gizmo::onKey(bool pressed, const Input::KeyEvent& key) {
        if (pressed) {
            _wasUsed = false;
        } else if (_wasUsed) {
            _shouldRegisterUndo = true;
            _wasUsed = false;
        }

        bool ret = false;
        ImGuiIO& io = _imguiContext->IO;
        if (active() && io.KeyCtrl) {
            TransformSettings settings = _parent.getTransformSettings();
            if (key._key == Input::KeyCode::KC_T) {
                settings.currentGizmoOperation = ImGuizmo::TRANSLATE;
                ret = true;
            } else if (key._key == Input::KeyCode::KC_R) {
                settings.currentGizmoOperation = ImGuizmo::ROTATE;
                ret = true;
            } else if (key._key == Input::KeyCode::KC_S) {
                settings.currentGizmoOperation = ImGuizmo::SCALE;
                ret = true;
            }
            if (ret && !pressed) {
                _parent.setTransformSettings(settings);
            }
        }

        return ret;
    }

    void Gizmo::onMouseButton(bool presseed) {
        if (presseed) {
            _wasUsed = false;
        } else if (_wasUsed) {
            _shouldRegisterUndo = true;
            _wasUsed = false;
        }
    }

    bool Gizmo::needsMouse() const {
        if (active()) {
            ImGuiContext* crtContext = ImGui::GetCurrentContext();
            ImGui::SetCurrentContext(_imguiContext);
            const bool imguizmoState = ImGuizmo::IsOver();
            ImGui::SetCurrentContext(crtContext);
            return imguizmoState;
        }

        return false;
    }

}; //namespace Divide