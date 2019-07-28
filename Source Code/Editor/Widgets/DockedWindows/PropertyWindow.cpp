#include "stdafx.h"

#include "Headers/PropertyWindow.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/PlatformContext.h"

#include "Editor/Headers/Editor.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Geometry/Material/Headers/Material.h"

#include "ECS/Components/Headers/TransformComponent.h"

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
        PushMultiItemsWidths(components, CalcItemWidth());
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
            if (ImGui::CollapsingHeader(selectedCamera->resourceName().c_str())) {
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
                    bool enabled = sgnNode->isActive();
                    if (ImGui::Checkbox(sgnNode->name().c_str(), &enabled)) {
                        sgnNode->setActive(enabled);
                    }
                    ImGui::Separator();

                    vectorFast<EditorComponent*>& editorComp = Attorney::SceneGraphNodeEditor::editorComponents(*sgnNode);
                    for (EditorComponent* comp : editorComp) {
                        if (ImGui::CollapsingHeader(comp->name().c_str(), ImGuiTreeNodeFlags_OpenOnArrow)) {
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
                                if (processField(field) && !field._readOnly) {
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
            F32 xOffset = ImGui::GetWindowSize().x - buttonWidth - 20.0f;
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
                ImGui::Text("Add new %s component?", selectedType._to_string());
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
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank;
        flags |= field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0;

        bool ret = false;
        switch (field._type) {
            case EditorComponentFieldType::PUSH_TYPE: {
                ret = processBasicField(field);
            }break;
            case EditorComponentFieldType::BOUNDING_BOX: {
                BoundingBox bb = {};
                field.get<BoundingBox>(bb);

                F32* bbMin = Attorney::BoundingBoxEditor::min(bb);
                F32* bbMax = Attorney::BoundingBoxEditor::max(bb);
                ret = ImGui::InputFloat3("- Min ", bbMin, "%.3f", flags) ||
                      ImGui::InputFloat3("- Max ", bbMax, "%.3f", flags);
                if (ret) {
                    field.set<BoundingBox>(bb);
                }
            }break;
            case EditorComponentFieldType::BOUNDING_SPHERE: {
                BoundingSphere bs = {};
                field.get<BoundingSphere>(bs);
                F32* center = Attorney::BoundingSphereEditor::center(bs);
                F32& radius = Attorney::BoundingSphereEditor::radius(bs);
                ret = ImGui::InputFloat3("- Center ", center, "%.3f", flags) ||
                      ImGui::InputFloat("- Radius ", &radius, 0.0f, 0.0f, -1, flags);
                if (ret) {
                    field.set<BoundingSphere>(bs);
                }
            }break;
            case EditorComponentFieldType::TRANSFORM: {
                assert(!field._dataSetter && "Need direct access to memory");
                ret = processTransform(field.getPtr<TransformComponent>(), field._readOnly);
            }break;

            case EditorComponentFieldType::MATERIAL: {
                assert(!field._dataSetter && "Need direct access to memory");
                ret = processMaterial(field.getPtr<Material>(), field._readOnly);
            }break;
        };

        return ret;
     }

    bool PropertyWindow::processTransform(TransformComponent* transform, bool readOnly) {
        bool ret = false;

        if (transform == nullptr) {
            return ret;
        }

        ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank;
        flags |= readOnly ? ImGuiInputTextFlags_ReadOnly : 0;

        TransformValues transformValues;
        transform->getValues(transformValues);

        vec3<F32> rot; transformValues._orientation.getEuler(rot); rot = Angle::to_DEGREES(rot);
        vec3<F32>& pos = transformValues._translation;
        vec3<F32>& scale = transformValues._scale;

        if (ImGui::InputFloat3(" - Position ", pos, "%.3f", flags)) {
            if (!readOnly) {
                ret = true;
            }
        }
        if (ImGui::InputFloat3(" - Rotation ", rot, "%.3f", flags)) {
            if (!readOnly) {
                ret = true;
            }
        }

        if (ImGui::InputFloat3(" - Scale ", scale, "%.3f", flags)) {
            if (!readOnly) {
                ret = true;
            }
        }

        if (ret) {
            // Scale is tricky as it may invalidate everything if it's set wrong!
            for (U8 i = 0; i < 3; ++i) {
                if (IS_ZERO(scale[i])) {
                    scale[i] = EPSILON_F32;
                }
            }

            transformValues._orientation.fromEuler(rot);
            transform->setTransform(transformValues);
        }

        return ret;
     }

     bool PropertyWindow::processMaterial(Material* material, bool readOnly) {
         ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank;
         flags |= readOnly ? ImGuiInputTextFlags_ReadOnly : 0;

         bool ret = false;
         if (material) {
             FColour diffuse = material->getColourData()._diffuse;
             if (ImGui::InputFloat4(" - Diffuse", diffuse._v, "%.3f", flags)) {
                 material->setDiffuse(diffuse);
             }

             FColour emissive = material->getColourData()._emissive;
             if (ImGui::InputFloat4(" - Emissive", emissive._v, "%.3f", flags)) {
                 material->setEmissive(emissive);
             }


             FColour specular = material->getColourData()._specular;
             if (ImGui::InputFloat4(" - Specular", specular._v, "%.3f", flags)) {
                 material->setSpecular(specular);
             }

             F32 shininess = material->getColourData()._shininess;
             if (ImGui::InputFloat(" - Shininess", &shininess, 0.0f, 0.0f, "%.3f", flags)) {
                 material->setShininess(shininess);
             }

             
             ImGui::SameLine();
             bool doubleSided = material->isDoubleSided();
             if (readOnly) {
                 ImGui::Checkbox("DoubleSided", &doubleSided);
             }  else {
                 material->setDoubleSided(ImGui::Checkbox("DoubleSided", &doubleSided));
             }

             bool receivesShadows = material->receivesShadows();
             if (readOnly) {
                 ImGui::Checkbox("ReceivesShadows", &receivesShadows);
             } else {
                 material->setReceivesShadows(ImGui::Checkbox("ReceivesShadows", &receivesShadows));
             }
         }

         return ret;
     }

     bool PropertyWindow::processBasicField(EditorComponentField& field) {
         ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsDecimal;
         flags |= field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0;

         const char* name = field._name.c_str();
         ImGui::PushID(name);

         bool ret = false;
         switch (field._basicType) {
             case GFX::PushConstantType::BOOL: {
                 ImGui::SameLine();
                 bool val = field.get<bool>();
                 ret = ImGui::Checkbox("", &val);
                 if (ret && !field._readOnly) {
                     field.set<bool>(val);
                 }
             }break;
             case GFX::PushConstantType::INT: {
                 I32 val = field.get<I32>();
                 ImGui::SameLine();

                 ret = ImGui::InputInt("", &val, 1, 100, flags);
                 if (ret && !field._readOnly) {
                     field.set<I32>(val);
                 }
             }break;
             case GFX::PushConstantType::UINT: {
                 ImGui::SameLine();
                 ImGui::Text("Not currently supported");
             }break;
             case GFX::PushConstantType::DOUBLE: {
                 D64 val = field.get<D64>();
                 ImGui::SameLine();
                 ret = ImGui::InputDouble("", &val, 0.0, 0.0, "%.6f", flags);
                 if (ret && !field._readOnly) {
                     field.set<D64>(val);
                 }
             }break;
             case GFX::PushConstantType::FLOAT: {
                 F32 val = field.get<F32>();
                 ImGui::SameLine();
                 ret = ImGui::InputFloat("", &val, 0.0f, 0.0f, -1, flags);
                 if (ret && !field._readOnly) {
                     field.set<F32>(val);
                 }
             }break;
             case GFX::PushConstantType::IVEC2: {
                 vec2<I32> val = field.get<vec2<I32>>();
                 ImGui::SameLine();
                 ret = ImGui::InputInt2("", val._v, flags);
                 if (ret && !field._readOnly) {
                     field.set<vec2<I32>>(val);
                 }
             }break;
             case GFX::PushConstantType::IVEC3: {
                 vec3<I32> val = field.get<vec3<I32>>();
                 ImGui::SameLine();
                 ret = ImGui::InputInt3("", val._v, flags);
                 if (ret && !field._readOnly) {
                     field.set<vec3<I32>>(val);
                 }
             }break;
             case GFX::PushConstantType::IVEC4: {
                 vec4<I32> val = field.get<vec4<I32>>();
                 ImGui::SameLine();
                 ret = ImGui::InputInt4("", val._v, flags);
                 if (ret && !field._readOnly) {
                     field.set<vec4<I32>>(val);
                 }
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
                 vec2<F32> val = field.get<vec2<F32>>();
                 ImGui::SameLine();
                 ret = ImGui::InputFloat2("", val._v, "%.3f", flags);
                 if (ret && !field._readOnly) {
                     field.set<vec2<F32>>(val);
                 }
             }break;
             case GFX::PushConstantType::VEC3: {
                 vec3<F32> val = field.get<vec3<F32>>();
                 ImGui::SameLine();
                 ret = ImGui::InputFloat3("", val._v, "%.3f", flags);
                 if (ret && !field._readOnly) {
                     field.set<vec3<F32>>(val);
                 }
             }break;
             case GFX::PushConstantType::VEC4: {
                 vec4<F32> val = field.get<vec4<F32>>();
                 ImGui::SameLine();
                 ret = ImGui::InputFloat4("", val._v, "%.3f", flags);
                 if (ret && !field._readOnly) {
                     field.set<vec4<F32>>(val);
                 }
             }break;
             case GFX::PushConstantType::DVEC2: {
                 vec2<D64> val = field.get<vec2<D64>>();
                 ImGui::SameLine();
                 ret = ImGui::InputDouble2("", val._v, "%.6f", flags);
                 if (ret && !field._readOnly) {
                     field.set<vec2<D64>>(val);
                 }
             }break;
             case GFX::PushConstantType::DVEC3: {
                 vec3<D64> val = field.get<vec3<D64>>();
                 ImGui::SameLine();
                 ret = ImGui::InputDouble3("", val._v, "%.6f", flags);
                 if (ret && !field._readOnly) {
                     field.set<vec3<D64>>(val);
                 }
             }break;
             case GFX::PushConstantType::DVEC4: {
                 vec4<D64> val = field.get<vec4<D64>>();
                 ImGui::SameLine();
                 ret = ImGui::InputDouble4("", val._v, "%.6f", flags);
                 if (ret && !field._readOnly) {
                     field.set<vec4<D64>>(val);
                 }
             }break;
             case GFX::PushConstantType::IMAT2: {
                 mat2<I32> mat = field.get<mat2<I32>>();
                 ret = ImGui::InputInt2("", mat._vec[0], flags) ||
                       ImGui::InputInt2("", mat._vec[1], flags);
                 if (ret && !field._readOnly) {
                     field.set<mat2<I32>>(mat);
                 }
             }break;
             case GFX::PushConstantType::IMAT3: {
                 mat3<I32> mat = field.get<mat3<I32>>();
                 ret = ImGui::InputInt3("", mat._vec[0], flags) ||
                       ImGui::InputInt3("", mat._vec[1], flags) ||
                       ImGui::InputInt3("", mat._vec[2], flags);
                 if (ret && !field._readOnly) {
                     field.set<mat3<I32>>(mat);
                 }
             }break;
             case GFX::PushConstantType::IMAT4: {
                 mat4<I32> mat = field.get<mat4<I32>>();
                 ret = ImGui::InputInt4("", mat._vec[0], flags) ||
                       ImGui::InputInt4("", mat._vec[1], flags) ||
                       ImGui::InputInt4("", mat._vec[2], flags) ||
                       ImGui::InputInt4("", mat._vec[3], flags);
                 if (ret && !field._readOnly) {
                     field.set<mat4<I32>>(mat);
                 }
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
                 mat2<F32> mat = field.get<mat2<F32>>();
                 ret = ImGui::InputFloat2("", mat._vec[0], "%.3f", flags) ||
                       ImGui::InputFloat2("", mat._vec[1], "%.3f", flags);
                 if (ret && !field._readOnly) {
                     field.set<mat2<F32>>(mat);
                 }
             }break;
             case GFX::PushConstantType::MAT3: {
                 mat3<F32> mat = field.get<mat3<F32>>();
                 ret = ImGui::InputFloat3("", mat._vec[0], "%.3f", flags) ||
                       ImGui::InputFloat3("", mat._vec[1], "%.3f", flags) ||
                       ImGui::InputFloat3("", mat._vec[2], "%.3f", flags);
                 if (ret && !field._readOnly) {
                     field.set<mat3<F32>>(mat);
                 }
             }break;
             case GFX::PushConstantType::MAT4: {
                 mat4<F32> mat = field.get<mat4<F32>>();
                 ret = ImGui::InputFloat4("", mat._vec[0], "%.3f", flags) ||
                       ImGui::InputFloat4("", mat._vec[1], "%.3f", flags) ||
                       ImGui::InputFloat4("", mat._vec[2], "%.3f", flags) ||
                       ImGui::InputFloat4("", mat._vec[3], "%.3f", flags);
                 if (ret && !field._readOnly) {
                     field.set<mat4<F32>>(mat);
                 }
             }break;
             case GFX::PushConstantType::DMAT2: {
                 mat2<D64> mat = field.get<mat2<D64>>();
                 ret = ImGui::InputDouble2("", mat._vec[0], "%.6f", flags) ||
                       ImGui::InputDouble2("", mat._vec[1], "%.6f", flags);
                 if (ret && !field._readOnly) {
                     field.set<mat2<D64>>(mat);
                 }
             }break;
             case GFX::PushConstantType::DMAT3: {
                 mat3<D64> mat = field.get<mat3<D64>>();
                 ret = ImGui::InputDouble3("", mat._vec[0], "%.6f", flags) ||
                       ImGui::InputDouble3("", mat._vec[1], "%.6f", flags) ||
                       ImGui::InputDouble3("", mat._vec[2], "%.6f", flags);
                 if (ret && !field._readOnly) {
                     field.set<mat3<D64>>(mat);
                 }
             }break;
             case GFX::PushConstantType::DMAT4: {
                 mat4<D64> mat = field.get<mat4<D64>>();
                 ret = ImGui::InputDouble4("", mat._vec[0], "%.6f", flags) ||
                       ImGui::InputDouble4("", mat._vec[1], "%.6f", flags) ||
                       ImGui::InputDouble4("", mat._vec[2], "%.6f", flags) ||
                       ImGui::InputDouble4("", mat._vec[3], "%.6f", flags);
                 if (ret && !field._readOnly) {
                     field.set<mat4<D64>>(mat);
                 }
             }break;
             default: {
                 ImGui::Text(name);
             }break;
         }
         ImGui::PopID();

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
