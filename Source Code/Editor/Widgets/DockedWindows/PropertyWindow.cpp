#include "stdafx.h"

#include "Headers/PropertyWindow.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/PlatformContext.h"

#include "Editor/Headers/Editor.h"
#include "Managers/Headers/SceneManager.h"
#include "Widgets/Headers/PanelManager.h"
#include "Rendering/Camera/Headers/Camera.h"

#undef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

namespace ImGui {
    bool InputDoubleN(const char* label, double* v, int components, const char* display_format, ImGuiInputTextFlags extra_flags)
    {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        bool value_changed = false;
        BeginGroup();
        PushID(label);
        PushMultiItemsWidths(components);
        for (int i = 0; i < components; i++)
        {
            PushID(i);
            value_changed |= InputDouble("##v", &v[i], 0.0, 0.0, display_format, extra_flags);
            SameLine(0, g.Style.ItemInnerSpacing.x);
            PopID();
            PopItemWidth();
        }
        PopID();

        TextUnformatted(label, FindRenderedTextEnd(label));
        EndGroup();

        return value_changed;
    }

    bool InputDouble2(const char* label, double v[2], const char* display_format, ImGuiInputTextFlags extra_flags)
    {
        return InputDoubleN(label, v, 2, display_format, extra_flags);
    }

    bool InputDouble3(const char* label, double v[3], const char* display_format, ImGuiInputTextFlags extra_flags)
    {
        return InputDoubleN(label, v, 3, display_format, extra_flags);
    }

    bool InputDouble4(const char* label, double v[4], const char* display_format, ImGuiInputTextFlags extra_flags)
    {
        return InputDoubleN(label, v, 4, display_format, extra_flags);
    }
};

namespace Divide {
    PropertyWindow::PropertyWindow(PanelManager& parent, PlatformContext& context)
        : DockedWindow(parent, "Properties"),
          PlatformContextComponent(context)
    {

    }

    PropertyWindow::~PropertyWindow()
    {

    }

    void PropertyWindow::draw() {
        drawTransformSettings();

        Camera* selectedCamera = Attorney::PanelManagerDockedWindows::getSelectedCamera(_parent);
        if (selectedCamera != nullptr) {
            if (ImGui::CollapsingHeader(selectedCamera->name().c_str())) {
                vec3<F32> eye = selectedCamera->getEye();
                if (ImGui::InputFloat3("Eye", eye._v)) {
                    selectedCamera->setEye(eye);
                }
                vec3<F32> euler = selectedCamera->getEuler();
                if (ImGui::InputFloat3("Euler", euler._v)) {
                    selectedCamera->setEuler(euler);
                }

                F32 aspect = selectedCamera->getAspectRatio();
                if (ImGui::InputFloat("Aspect", &aspect)) {
                    selectedCamera->setAspectRatio(aspect);
                }

                F32 horizontalFoV = selectedCamera->getHorizontalFoV();
                if (ImGui::InputFloat("FoV (horizontal)", &horizontalFoV)) {
                    selectedCamera->setHorizontalFoV(horizontalFoV);
                }

                vec2<F32> zPlanes = selectedCamera->getZPlanes();
                if (ImGui::InputFloat2("zPlanes", zPlanes._v)) {
                    if (selectedCamera->isOrthoProjected()) {
                        selectedCamera->setProjection(selectedCamera->orthoRect(), zPlanes);
                    } else {
                        selectedCamera->setProjection(selectedCamera->getAspectRatio(), selectedCamera->getVerticalFoV(), zPlanes);
                    }
                }

                if (selectedCamera->isOrthoProjected()) {
                    vec4<F32> orthoRect = selectedCamera->orthoRect();
                    if (ImGui::InputFloat4("Ortho", orthoRect._v)) {
                        selectedCamera->setProjection(orthoRect, selectedCamera->getZPlanes());
                    }
                }
                ImGui::Text("View Matrix");
                ImGui::Spacing();
                mat4<F32> viewMatrix = selectedCamera->getViewMatrix();
                EditorComponentField worldMatrixField = {
                    GFX::PushConstantType::MAT4,
                    EditorComponentFieldType::PUSH_TYPE,
                    true,
                    "View Matrix",
                    &viewMatrix
                };
                processBasicField(worldMatrixField);

                ImGui::Text("Projection Matrix");
                ImGui::Spacing();
                mat4<F32> projMatrix = selectedCamera->getProjectionMatrix();
                EditorComponentField projMatrixField = {
                    GFX::PushConstantType::MAT4,
                    EditorComponentFieldType::PUSH_TYPE,
                    true,
                    "Projection Matrix",
                    &projMatrix
                };
                processBasicField(projMatrixField);
            }
        } else {
            SceneManager& sceneManager = context().kernel().sceneManager();
            Scene& activeScene = sceneManager.getActiveScene();

            const vectorImpl<I64>& selections = activeScene.getCurrentSelection();
            for (I64 nodeGUID : selections) {
                SceneGraphNode* node = activeScene.sceneGraph().findNode(nodeGUID);
                if (node != nullptr) {
                    ImGui::Text(node->name().c_str());

                    vectorImpl<EditorComponent*>& editorComp = Attorney::SceneGraphNodeEditor::editorComponents(*node);
                    for (EditorComponent* comp : editorComp) {
                        if (ImGui::CollapsingHeader(comp->name().c_str()))
                        {
                            vectorImpl<EditorComponentField>& fields = Attorney::EditorComponentEditor::fields(*comp);
                            for (EditorComponentField& field : fields) {
                                ImGui::Text(field._name.c_str());
                                ImGui::Spacing();
                                if (processField(field)) {
                                    Attorney::EditorComponentEditor::onChanged(*comp, field);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

     bool PropertyWindow::processField(EditorComponentField& field) {
        bool ret = false;
        switch (field._type) {
            case EditorComponentFieldType::PUSH_TYPE: {
                ret = processBasicField(field);
            }break;
            case EditorComponentFieldType::BOUNDING_BOX: {
                BoundingBox* bb = static_cast<BoundingBox*>(field._data);
                F32* bbMin = Attorney::BoundingBoxEditor::min(*bb);
                F32* bbMax = Attorney::BoundingBoxEditor::max(*bb);
                ret = ImGui::InputFloat3("- Min ", bbMin, field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0) ||
                      ImGui::InputFloat3("- Max ", bbMax, field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
            }break;
            case EditorComponentFieldType::BOUNDING_SPHERE: {
                BoundingSphere* bs = static_cast<BoundingSphere*>(field._data);
                F32* center = Attorney::BoundingSphereEditor::center(*bs);
                F32& radius = Attorney::BoundingSphereEditor::radius(*bs);
                ret = ImGui::InputFloat3("- Center ", center, field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0) ||
                      ImGui::InputFloat("- Radius ", &radius, 0.0f, 0.0f, -1, field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
            }break;
            case EditorComponentFieldType::TRANSFORM: {
                ret = processTransform(static_cast<Transform*>(field._data), field._readOnly);
            }break;
        };

        return ret;
     }

     bool PropertyWindow::processTransform(Transform* transform, bool readOnly) {
         bool ret = false;
         if (transform) {
             vec3<F32> scale;
             vec3<F32> position;
             vec3<F32> orientationEuler;
             Quaternion<F32> orientation;

             transform->getScale(scale);
             transform->getPosition(position);
             transform->getOrientation(orientation);
             orientation.getEuler(orientationEuler);

             if (ImGui::InputFloat3(" - Position ", position._v, readOnly ? ImGuiInputTextFlags_ReadOnly : 0)) {
                 transform->setPosition(position);
                 ret = true;
             }
             if (ImGui::InputFloat3(" - Rotation ", orientationEuler._v, readOnly ? ImGuiInputTextFlags_ReadOnly : 0)) {
                 orientation.fromEuler(orientationEuler);
                 transform->setRotation(orientation);
                 ret = true;
             }

             if (ImGui::InputFloat3(" - Scale ", scale._v, readOnly ? ImGuiInputTextFlags_ReadOnly : 0)) {
                 transform->setScale(scale);
                 ret = true;
             }
         }

         return ret;
     }

     bool PropertyWindow::processBasicField(EditorComponentField& field) {
         bool ret = false;
         switch (field._basicType) {
             case GFX::PushConstantType::BOOL: {
                 ImGui::SameLine();
                 if (field._readOnly) {
                     bool val = *(bool*)(field._data);
                     ImGui::Checkbox("", &val);
                 } else {
                     ret = ImGui::Checkbox("", (bool*)(field._data));
                 }
             }break;
             case GFX::PushConstantType::INT: {
                 ImGui::SameLine();
                 ret = ImGui::InputInt("", (I32*)(field._data), 1, 100, field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
             }break;
             case GFX::PushConstantType::UINT: {
                 ImGui::SameLine();
                 ImGui::Text("Not currently supported");
             }break;
             case GFX::PushConstantType::DOUBLE: {
                 ImGui::SameLine();
                 ret = ImGui::InputDouble("", (D64*)(field._data), 0.0, 0.0, "%.6f", field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
             }break;
             case GFX::PushConstantType::FLOAT: {
                 ImGui::SameLine();
                 ret = ImGui::InputFloat("", (F32*)(field._data), 0.0f, 0.0f, -1, field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
             }break;
             case GFX::PushConstantType::IVEC2: {
                 ImGui::SameLine();
                 ret = ImGui::InputInt2("", (I32*)(field._data), field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
             }break;
             case GFX::PushConstantType::IVEC3: {
                 ImGui::SameLine();
                 ret = ImGui::InputInt3("", (I32*)(field._data), field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
             }break;
             case GFX::PushConstantType::IVEC4: {
                 ImGui::SameLine();
                 ret = ImGui::InputInt4("", (I32*)(field._data), field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
             }break;
             case GFX::PushConstantType::UVEC2: {
                 ImGui::SameLine();
                 ImGui::Text("Not currently supported");
             }break;
             case GFX::PushConstantType::UVEC3: {
                 ImGui::SameLine();
                 ImGui::Text("Not currently supported");
             }break;
             case GFX::PushConstantType::UVEC4: {
                 ImGui::SameLine();
                 ImGui::Text("Not currently supported");
             }break;
             case GFX::PushConstantType::VEC2: {
                 ImGui::SameLine();
                 ret = ImGui::InputFloat2("", (F32*)(field._data), field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
             }break;
             case GFX::PushConstantType::VEC3: {
                 ImGui::SameLine();
                 ret = ImGui::InputFloat3("", (F32*)(field._data), field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
             }break;
             case GFX::PushConstantType::VEC4: {
                 ImGui::SameLine();
                 ret = ImGui::InputFloat4("", (F32*)(field._data), field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
             }break;
             case GFX::PushConstantType::DVEC2: {
                 ImGui::SameLine();
                 ret = ImGui::InputDouble2("", (D64*)(field._data), "%.6f", field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
             }break;
             case GFX::PushConstantType::DVEC3: {
                 ImGui::SameLine();
                 ret = ImGui::InputDouble3("", (D64*)(field._data), "%.6f", field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
             }break;
             case GFX::PushConstantType::DVEC4: {
                 ImGui::SameLine();
                 ret = ImGui::InputDouble4("", (D64*)(field._data), "%.6f", field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
             }break;
             case GFX::PushConstantType::IMAT2: {
                 mat2<I32>* mat = (mat2<I32>*)(field._data);
                 ret = ImGui::InputInt2("", mat->_vec[0], field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0) ||
                       ImGui::InputInt2("", mat->_vec[1], field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
             }break;
             case GFX::PushConstantType::IMAT3: {
                 mat3<I32>* mat = (mat3<I32>*)(field._data);
                 ret = ImGui::InputInt3("", mat->_vec[0], field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0) ||
                       ImGui::InputInt3("", mat->_vec[1], field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0) ||
                       ImGui::InputInt3("", mat->_vec[2], field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
             }break;
             case GFX::PushConstantType::IMAT4: {
                 mat4<I32>* mat = (mat4<I32>*)(field._data);
                 ret = ImGui::InputInt4("", mat->_vec[0], field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0) ||
                       ImGui::InputInt4("", mat->_vec[1], field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0) ||
                       ImGui::InputInt4("", mat->_vec[2], field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0) ||
                       ImGui::InputInt4("", mat->_vec[3], field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
             }break;
             case GFX::PushConstantType::UMAT2: {
                 ImGui::SameLine();
                 ImGui::Text("Not currently supported");
             }break;
             case GFX::PushConstantType::UMAT3: {
                 ImGui::SameLine();
                 ImGui::Text("Not currently supported");
             }break;
             case GFX::PushConstantType::UMAT4: {
                 ImGui::SameLine();
                 ImGui::Text("Not currently supported");
             }break;
             case GFX::PushConstantType::MAT2: {
                 mat2<F32>* mat = (mat2<F32>*)(field._data);
                 ret = ImGui::InputFloat2("", mat->_vec[0], field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0) ||
                       ImGui::InputFloat2("", mat->_vec[1], field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
             }break;
             case GFX::PushConstantType::MAT3: {
                 mat3<F32>* mat = (mat3<F32>*)(field._data);
                 ret = ImGui::InputFloat3("", mat->_vec[0], field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0) ||
                       ImGui::InputFloat3("", mat->_vec[1], field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0) ||
                       ImGui::InputFloat3("", mat->_vec[2], field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
             }break;
             case GFX::PushConstantType::MAT4: {
                 mat4<F32>* mat = (mat4<F32>*)(field._data);
                 ret = ImGui::InputFloat4("", mat->_vec[0], field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0) ||
                       ImGui::InputFloat4("", mat->_vec[1], field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0) ||
                       ImGui::InputFloat4("", mat->_vec[2], field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0) ||
                       ImGui::InputFloat4("", mat->_vec[3], field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
             }break;
             case GFX::PushConstantType::DMAT2: {
                 mat2<D64>* mat = (mat2<D64>*)(field._data);
                 ret = ImGui::InputDouble2("", mat->_vec[0], "%.6f", field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0) ||
                       ImGui::InputDouble2("", mat->_vec[1], "%.6f", field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
             }break;
             case GFX::PushConstantType::DMAT3: {
                 mat3<D64>* mat = (mat3<D64>*)(field._data);
                 ret = ImGui::InputDouble3("", mat->_vec[0], "%.6f", field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0) ||
                       ImGui::InputDouble3("", mat->_vec[1], "%.6f", field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0) ||
                       ImGui::InputDouble3("", mat->_vec[2], "%.6f", field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
             }break;
             case GFX::PushConstantType::DMAT4: {
                 mat4<D64>* mat = (mat4<D64>*)(field._data);
                 ret = ImGui::InputDouble4("", mat->_vec[0], "%.6f", field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0) ||
                       ImGui::InputDouble4("", mat->_vec[1], "%.6f", field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0) ||
                       ImGui::InputDouble4("", mat->_vec[2], "%.6f", field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0) ||
                       ImGui::InputDouble4("", mat->_vec[3], "%.6f", field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
             }break;
             default: {
                 ImGui::Text(field._name.c_str());
             }break;
         }

         return ret;
     }

     void PropertyWindow::drawTransformSettings() {
         ImGui::Separator();
         bool enableGizmo = Attorney::PanelManagerDockedWindows::editorEnableGizmo(_parent);
         ImGui::Checkbox("Transform Gizmo", &enableGizmo);
         Attorney::PanelManagerDockedWindows::editorEnableGizmo(_parent, enableGizmo);

         if (enableGizmo) {
             TransformSettings settings = _parent.getTransformSettings();

             if (ImGui::IsKeyPressed(Input::KeyCode::KC_T)) {
                 settings.currentGizmoOperation = ImGuizmo::TRANSLATE;
             }

             if (ImGui::IsKeyPressed(Input::KeyCode::KC_R)) {
                 settings.currentGizmoOperation = ImGuizmo::ROTATE;
             }

             if (ImGui::IsKeyPressed(Input::KeyCode::KC_S)) {
                 settings.currentGizmoOperation = ImGuizmo::SCALE;
             }

             if (ImGui::RadioButton("Translate", settings.currentGizmoOperation == ImGuizmo::TRANSLATE)) {
                 settings.currentGizmoOperation = ImGuizmo::TRANSLATE;
             }

             ImGui::SameLine();
             if (ImGui::RadioButton("Rotate", settings.currentGizmoOperation == ImGuizmo::ROTATE)) {
                 settings.currentGizmoOperation = ImGuizmo::ROTATE;
             }

             ImGui::SameLine();
             if (ImGui::RadioButton("Scale", settings.currentGizmoOperation == ImGuizmo::SCALE)) {
                 settings.currentGizmoOperation = ImGuizmo::SCALE;
             }

             if (settings.currentGizmoOperation != ImGuizmo::SCALE) {
                 if (ImGui::RadioButton("Local", settings.currentGizmoMode == ImGuizmo::LOCAL)) {
                     settings.currentGizmoMode = ImGuizmo::LOCAL;
                 }
                 ImGui::SameLine();
                 if (ImGui::RadioButton("World", settings.currentGizmoMode == ImGuizmo::WORLD)) {
                     settings.currentGizmoMode = ImGuizmo::WORLD;
                 }
             }

             ImGui::Checkbox("", &settings.useSnap);
             ImGui::SameLine();

             switch (settings.currentGizmoOperation)
             {
                 case ImGuizmo::TRANSLATE:
                     ImGui::InputFloat3("Position Snap", &settings.snap[0]);
                     break;
                 case ImGuizmo::ROTATE:
                     ImGui::InputFloat("Angle Snap", &settings.snap[0]);
                     break;
                 case ImGuizmo::SCALE:
                     ImGui::InputFloat("Scale Snap", &settings.snap[0]);
                     break;
             }
             ImGui::Separator();

             _parent.setTransformSettings(settings);
         }
     }
}; //namespace Divide
