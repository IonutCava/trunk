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
#ifndef _EDITOR_GIZMO_H_
#define _EDITOR_GIZMO_H_

#include "Editor/Headers/UndoManager.h"
#include "ECS/Components/Headers/TransformComponent.h"
#include "Platform/Input/Headers/InputAggregatorInterface.h"

#include <imgui/addons/imguigizmo/ImGuizmo.h>

struct ImGuiContext;

namespace Divide {
    class Editor;
    class Camera;
    class DisplayWindow;
    class SceneGraphNode;

    struct SizeChangeParams;

    namespace Attorney {
        class GizmoEditor;
    };

    struct TransformSettings {
        ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::ROTATE;
        ImGuizmo::MODE currentGizmoMode = ImGuizmo::WORLD;
        bool useSnap = false;
        F32 snap[3] = { 1.f, 1.f, 1.f };
    };

    class Gizmo {
        friend class Attorney::GizmoEditor;

    public:
        explicit Gizmo(Editor& parent, ImGuiContext* targetContext);
        ~Gizmo();

        ImGuiContext& getContext() noexcept;
        const ImGuiContext& getContext() const noexcept;

        bool needsMouse() const;
        bool hovered() const;
        void enable(bool state) noexcept;
        bool enabled() const noexcept;
        bool active() const noexcept;

        void onMouseButton(bool presseed);
        bool onKey(bool pressed, const Input::KeyEvent& key);

    protected:
        void update(const U64 deltaTimeUS);
        void render(const Camera* camera, const Rect<I32>& targetViewport, GFX::CommandBuffer& bufferInOut);
        void updateSelections(const vectorEASTL<SceneGraphNode*>& node);
        void setTransformSettings(const TransformSettings& settings) noexcept;
        const TransformSettings& getTransformSettings() const noexcept;

    private:
        Editor& _parent;
        bool _enabled = false;
        bool _visible = false;
        bool _wasUsed = false;
        bool _shouldRegisterUndo = false;
        vectorEASTL<SceneGraphNode*> _selectedNodes;
        ImGuiContext* _imguiContext = nullptr;
        TransformSettings _transformSettings;
        TransformValues _workValues;
    };

    namespace Attorney {
        class GizmoEditor {
        private:
            static void render(Gizmo* gizmo, const Camera* camera, const Rect<I32>& targetViewport, GFX::CommandBuffer& bufferInOut) {
                gizmo->render(camera, targetViewport, bufferInOut);
            }

            static void updateSelection(Gizmo* gizmo, const vectorEASTL<SceneGraphNode*>& nodes) {
                gizmo->updateSelections(nodes);
            }

            static void update(Gizmo* gizmo, const U64 deltaTimeUS) {
                gizmo->update(deltaTimeUS);
            }

            static void setTransformSettings(Gizmo* gizmo, const TransformSettings& settings) noexcept {
                gizmo->setTransformSettings(settings);
            }

            static const TransformSettings& getTransformSettings(const Gizmo* gizmo) noexcept {
                return gizmo->getTransformSettings();
            }
            friend class Divide::Editor;
        };
    };
}; //namespace Divide

#endif //_EDITOR_GIZMO_H_