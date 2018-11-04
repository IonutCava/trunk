#include "stdafx.h"

#include "Headers/PropertyWindow.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/PlatformContext.h"

#include "Editor/Headers/Editor.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Geometry/Material/Headers/Material.h"

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
    PropertyWindow::PropertyWindow(Editor& parent, PlatformContext& context, const Descriptor& descriptor)
        : DockedWindow(parent, descriptor),
          PlatformContextComponent(context)
    {

    }

    PropertyWindow::~PropertyWindow()
    {

    }

    void PropertyWindow::drawInternal() {
        bool hasSelections = !selections().empty();

        Camera* selectedCamera = Attorney::EditorPropertyWindow::getSelectedCamera(_parent);
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
        } else if (hasSelections) {
            const F32 smallButtonWidth = 60.0f;
            F32 xOffset = ImGui::GetWindowSize().x * 0.5f - smallButtonWidth;
            const vector<I64>& crtSelections = selections();

            static bool closed = false;
            for (I64 nodeGUID : crtSelections) {
                SceneGraphNode* sgnNode = node(nodeGUID);
                if (sgnNode != nullptr) {
                    ImGui::Text(sgnNode->name().c_str());

                    vectorEASTL<EditorComponent*>& editorComp = Attorney::SceneGraphNodeEditor::editorComponents(*sgnNode);
                    for (EditorComponent* comp : editorComp) {
                        if (ImGui::CollapsingHeader(comp->name().c_str())) {
                            //const I32 RV = ImGui::AppendTreeNodeHeaderButtons(comp->name().c_str(), ImGui::GetCursorPosX(), 1, &closed, "Remove", NULL, 0);
                            ImGui::NewLine();
                            ImGui::SameLine(xOffset);
                            if (ImGui::Button("INSPECT", ImVec2(smallButtonWidth, 20))) {
                                Attorney::EditorGeneralWidget::inspectMemory(_context.editor(), std::make_pair(comp, sizeof(EditorComponent)));
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("REMOVE", ImVec2(smallButtonWidth, 20))) {
                                Attorney::EditorGeneralWidget::inspectMemory(_context.editor(), std::make_pair(nullptr, 0));
                            }
                            ImGui::Separator();

                            vector<EditorComponentField>& fields = Attorney::EditorComponentEditor::fields(*comp);
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

        if (hasSelections) {
            const F32 buttonWidth = 80.0f;
            F32 xOffset = ImGui::GetWindowSize().x - buttonWidth;
            ImGui::NewLine();
            ImGui::Separator();
            ImGui::NewLine();
            ImGui::SameLine(xOffset);
            if (ImGui::Button("ADD NEW", ImVec2(buttonWidth, 15))) {
                ImGui::OpenPopup("COMP_SELECTION_GROUP");
            }

            static ComponentType selectedType = ComponentType::COUNT;

            if (ImGui::BeginPopup("COMP_SELECTION_GROUP")) {
                for (ComponentType type : ComponentType::_values()) {
                    if (type._to_integral() == ComponentType::COUNT) {
                        continue;
                    }

                    if (ImGui::Selectable(type._to_string())) {
                        selectedType = type;
                    }
                }
                ImGui::EndPopup();
            }
            if (selectedType._to_integral() != ComponentType::COUNT) {
                ImGui::OpenPopup("Add new component");
            }

            if (ImGui::BeginPopupModal("Add new component", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text(Util::StringFormat("Add new %s component?", selectedType._to_string()).c_str());
                ImGui::Separator();

                if (ImGui::Button("OK", ImVec2(120, 0))) {
                    selectedType = ComponentType::COUNT;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                    selectedType = ComponentType::COUNT;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            
        }
    }
    
    vector<I64> PropertyWindow::selections() const {
        const SceneManager& sceneManager = context().kernel().sceneManager();
        const Scene& activeScene = sceneManager.getActiveScene();

        return activeScene.getCurrentSelection();
    }
    
    SceneGraphNode* PropertyWindow::node(I64 guid) const {
        const SceneManager& sceneManager = context().kernel().sceneManager();
        const Scene& activeScene = sceneManager.getActiveScene();

        return activeScene.sceneGraph().findNode(guid);
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
                ret = ImGui::InputFloat3("- Min ", bbMin, 2, field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0) ||
                      ImGui::InputFloat3("- Max ", bbMax, 2, field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
            }break;
            case EditorComponentFieldType::BOUNDING_SPHERE: {
                BoundingSphere* bs = static_cast<BoundingSphere*>(field._data);
                F32* center = Attorney::BoundingSphereEditor::center(*bs);
                F32& radius = Attorney::BoundingSphereEditor::radius(*bs);
                ret = ImGui::InputFloat3("- Center ", center, 2, field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0) ||
                      ImGui::InputFloat("- Radius ", &radius, 0.0f, 0.0f, -1, field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);
            }break;
            case EditorComponentFieldType::TRANSFORM: {
                ret = processTransform(static_cast<Transform*>(field._data), field._readOnly);
            }break;

            case EditorComponentFieldType::MATERIAL: {
                ret = processMaterial(static_cast<Material*>(field._data), field._readOnly);
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

             if (ImGui::InputFloat3(" - Position ", position._v, 2, readOnly ? ImGuiInputTextFlags_ReadOnly : 0)) {
                 transform->setPosition(position);
                 ret = true;
             }
             if (ImGui::InputFloat3(" - Rotation ", orientationEuler._v, 2, readOnly ? ImGuiInputTextFlags_ReadOnly : 0)) {
                 orientation.fromEuler(orientationEuler);
                 transform->setRotation(orientation);
                 ret = true;
             }

             if (ImGui::InputFloat3(" - Scale ", scale._v, 2, readOnly ? ImGuiInputTextFlags_ReadOnly : 0)) {
                 transform->setScale(scale);
                 ret = true;
             }
         }

         return ret;
     }

     bool PropertyWindow::processMaterial(Material* material, bool readOnly) {
         bool ret = false;
         if (material) {
             FColour diffuse = material->getColourData()._diffuse;
             if (ImGui::InputFloat4(" - Diffuse", diffuse._v, 2, readOnly ? ImGuiInputTextFlags_ReadOnly : 0)) {
                 material->setDiffuse(diffuse);
             }

             FColour emissive = material->getColourData()._emissive;
             if (ImGui::InputFloat4(" - Emissive", diffuse._v, 2, readOnly ? ImGuiInputTextFlags_ReadOnly : 0)) {
                 material->setEmissive(emissive);
             }


             FColour specular = material->getColourData()._specular;
             if (ImGui::InputFloat4(" - Specular", specular._v, 2, readOnly ? ImGuiInputTextFlags_ReadOnly : 0)) {
                 material->setSpecular(specular);
             }

             F32 shininess = material->getColourData()._shininess;
             if (ImGui::InputFloat(" - Shininess", &shininess, 0.0f, 0.0f, "%.3f", readOnly ? ImGuiInputTextFlags_ReadOnly : 0)) {
                 material->setShininess(shininess);
             }

             
             ImGui::SameLine();
             bool doubleSided = material->isDoubleSided();
             if (readOnly) {
                 ImGui::Checkbox("DobuleSided", &doubleSided);
             }  else {
                 material->setDoubleSided(ImGui::Checkbox("DobuleSided", &doubleSided));
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
     const char* PropertyWindow::name() const {
         const vector<I64> nodes = selections();
         if (nodes.empty()) {
             return DockedWindow::name();
         }
         if (nodes.size() == 1) {
             return node(nodes.front())->name().c_str();
         }

         return Util::StringFormat("%s, %s, ...", node(nodes[0])->name().c_str(), node(nodes[1])->name().c_str()).c_str();
     }
}; //namespace Divide
