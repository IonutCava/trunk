#include "stdafx.h"

#include "Headers/PropertyWindow.h"
#include "Headers/UndoManager.h"

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
                EditorComponentField worldMatrixField;
                worldMatrixField._name = "View Matrix";
                worldMatrixField._basicType = GFX::PushConstantType::MAT4;
                worldMatrixField._type = EditorComponentFieldType::PUSH_TYPE;
                worldMatrixField._readOnly = true;
                worldMatrixField._data = &viewMatrix;
                
                processBasicField(worldMatrixField);

                ImGui::Text("Projection Matrix");
                ImGui::Spacing();
                mat4<F32> projMatrix = selectedCamera->getProjectionMatrix();
                EditorComponentField projMatrixField;
                projMatrixField._basicType = GFX::PushConstantType::MAT4;
                projMatrixField._type = EditorComponentFieldType::PUSH_TYPE;
                projMatrixField._readOnly = true;
                projMatrixField._name = "Projection Matrix";
                projMatrixField._data = &projMatrix;
                
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
                    bool enabled = sgnNode->hasFlag(SceneGraphNode::Flags::ACTIVE);
                    if (ImGui::Checkbox(sgnNode->name().c_str(), &enabled)) {
                        if (enabled) {
                            sgnNode->setFlag(SceneGraphNode::Flags::ACTIVE);
                        } else {
                            sgnNode->clearFlag(SceneGraphNode::Flags::ACTIVE);
                        }
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
                                //ImGui::NewLine();
                                if (processField(field) && !field._readOnly) {
                                    Attorney::EditorComponentEditor::onChanged(*comp, field);
                                }
                                ImGui::Spacing();
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
            case EditorComponentFieldType::BUTTON: {
                if (field._readOnly) {
                    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                }

                if (ImGui::Button(field._name.c_str(), ImVec2(field._range.x, field._range.y))) {
                    ret = true;
                }

                if (field._readOnly) {
                    ImGui::PopItemFlag();
                    ImGui::PopStyleVar();
                }
            }break;
            case EditorComponentFieldType::SLIDER_TYPE:
            case EditorComponentFieldType::PUSH_TYPE: {
                ret = processBasicField(field);
            }break;
            case EditorComponentFieldType::DROPDOWN_TYPE: {
                const U32 crtIndex = to_U32(field._range.x);
                const U32 entryCount = to_U32(field._range.y);

                if (field._dataSetter && field._dataGetter && entryCount > 0) {
                    ret = ImGui::BeginCombo(field._name.c_str(), "");
                    if (ret) {
                        for (U32 n = 0; n < entryCount; ++n) {

                        }
                        ImGui::EndCombo();
                    }
                }
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
        if (!field._tooltip.empty() && ImGui::IsItemHovered()) {
            ImGui::SetTooltip(field._tooltip.c_str());
        }
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

     namespace {
         bool colourInput4(Editor& parent, const char* name, FColour4& col, bool readOnly, std::function<void(const void*)> dataSetter = {}) {
             static UndoEntry<FColour4> undo = {};

             FColour4 oldVal = col;
             bool ret = ImGui::ColorEdit4(readOnly ? "" : name, col._v, ImGuiColorEditFlags__OptionsDefault);

             if (!readOnly) {
                 if (ImGui::IsItemActivated()) {
                     undo.oldVal = oldVal;
                 }

                 if (ImGui::IsItemDeactivatedAfterEdit()) {
                     undo._type = GFX::PushConstantType::FCOLOUR4;
                     undo._name = name;
                     undo._data = col._v;
                     undo._dataSetter = dataSetter;
                     undo.newVal = col;
                     parent.registerUndoEntry(undo);
                 }
             }

             return ret;
         }

         bool colourInput3(Editor& parent, const char* name, FColour3& col, bool readOnly, std::function<void(const void*)> dataSetter = {}) {
             static UndoEntry<FColour3> undo = {};

             FColour3 oldVal = col;
             bool ret =  ImGui::ColorEdit3( readOnly ? "" : name, col._v, ImGuiColorEditFlags__OptionsDefault);

             if (!readOnly) {
                 if (ImGui::IsItemActivated()) {
                     undo.oldVal = oldVal;
                 }

                 if (ImGui::IsItemDeactivatedAfterEdit()) {
                     undo._type = GFX::PushConstantType::FCOLOUR3;
                     undo._name = name;
                     undo._data = col._v;
                     undo._dataSetter = dataSetter;
                     undo.newVal = col;
                     parent.registerUndoEntry(undo);
                 }
             }

             return ret;
         }

         bool colourInput4(Editor& parent, const char* name, EditorComponentField& field) {
             FColour4 val = field.get<FColour4>();
             if (colourInput4(parent, name, val, field._readOnly, field._dataSetter) && val != field.get<FColour4>()) {
                 field.set(val);
                 return true;
             }

             return false;
         }

         bool colourInput3(Editor& parent, const char* name, EditorComponentField& field) {
             FColour3 val = field.get<FColour3>();
             if (colourInput3(parent, name, val, field._readOnly, field._dataSetter) && val != field.get<FColour3>()) {
                 field.set(val);
                 return true;
             }

             return false;
         }

        template<typename T, size_t num_comp, size_t step = 1, size_t step_fast = 100>
        bool inputOrSlider(Editor& parent, bool slider, const char* label, const char* name, ImGuiDataType data_type, EditorComponentField& field, ImGuiInputTextFlags flags, const char* format = "%d", float power = 1.0f) {
            static UndoEntry<T> undo = {};

            if (slider && field._readOnly) {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }

            T val = field.get<T>();
            T min = T(field._range.min), max = T(field._range.max);
            T cStep = T(step);
            T cStepFast = T(step_fast);

            bool ret = false;
            if (num_comp == 1) {
                if (slider) {
                    ret = ImGui::SliderScalar(label, data_type, (void*)&val, (void*)&min, (void*)&max, format, power);
                } else {
                    ret = ImGui::InputScalar(label, data_type, (void*)&val, (void*)&cStep, (void*)&cStepFast, format, flags);
                }
            } else {
                if (slider) {
                    ret = ImGui::SliderScalarN(label, data_type, (void*)&val, num_comp, (void*)&min, (void*)&max, format, power);
                } else {
                    ret = ImGui::InputScalarN(label, data_type, (void*)&val, num_comp, (void*)&cStep, (void*)&cStepFast, format, flags);
                }
            }

            if (!field._readOnly) {
                if (ImGui::IsItemActivated()) {
                    undo.oldVal = field.get<T>();
                }

                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    undo._type = field._basicType;
                    undo._name = name;
                    undo._data = field._data;
                    undo._dataSetter = field._dataSetter;
                    undo.newVal = val;
                    parent.registerUndoEntry(undo);
                }

                if (ret && val != field.get<T>()) {
                    field.set(val);
                }
            } else if (slider) {
                ImGui::PopStyleVar();
                ImGui::PopItemFlag();
            }

            return ret;
         };

        template<typename T, size_t num_rows, size_t step = 1, size_t step_fast = 100>
        bool inputMatrix(Editor & parent, const char* label, const char* name, ImGuiDataType data_type, EditorComponentField& field, ImGuiInputTextFlags flags, const char* format = "%d") {
            static UndoEntry<T> undo = {};

            T cStep = T(step);
            T cStepFast = T(step_fast);

            T mat = field.get<T>();
            bool ret = ImGui::InputScalarN(label, data_type, (void*)mat._vec[0]._v, num_rows, NULL, NULL, format, flags) ||
                       ImGui::InputScalarN(label, data_type, (void*)mat._vec[1]._v, num_rows, NULL, NULL, format, flags);
            if (num_rows > 2) {
                ret = ImGui::InputScalarN(label, data_type, (void*)mat._vec[2]._v, num_rows, NULL, NULL, format, flags) || ret;
                if (num_rows > 3) {
                    ret = ImGui::InputScalarN(label, data_type, (void*)mat._vec[3]._v, num_rows, NULL, NULL, format, flags) || ret;
                }
            }
            if (!field._readOnly) {
                if (ImGui::IsItemActivated()) {
                    undo.oldVal = field.get<T>();
                }

                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    undo._type = field._basicType;
                    undo._name = name;
                    undo._data = field._data;
                    undo._dataSetter = field._dataSetter;
                    undo.newVal = mat;
                    parent.registerUndoEntry(undo);
                }
                if (ret && !field._readOnly) {
                    if (ret && mat != field.get<T>()) {
                        field.set<>(mat);
                    }
                }
            }
            return ret;
        }
     };

     bool PropertyWindow::processMaterial(Material* material, bool readOnly) {
         bool ret = false;

         if (material) {

             if (readOnly) {
                 ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                 ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
             }

             ImGui::Text(Util::StringFormat("Shading Mode [ %s ]", getShadingModeName(material->getShadingMode())).c_str());

             auto diffuseSetter = [material](const void* data) {
                 material->getColourData().baseColour(*(FColour4*)data);
             };
             FColour4 diffuse = material->getColourData().baseColour();
             if (colourInput4(_parent, " - BaseColour", diffuse, readOnly, diffuseSetter)) {
                 diffuseSetter(diffuse._v);
                 ret = true;
             }
                 
             auto emissiveSetter = [material](const void* data) {
                 material->getColourData().emissive(*(FColour3*)data);
             };
             FColour3 emissive = material->getColourData().emissive();
             if (colourInput3(_parent, " - Emissive", emissive, readOnly, emissiveSetter)) {
                 emissiveSetter(emissive._v);
                 ret = true;
             }
             
             if (material->isPBRMaterial()) {
                 F32 reflectivity = material->getColourData().reflectivity();
                 if (ImGui::SliderFloat(" - Specular", &reflectivity, 0.0f, 1.0f)) {
                     material->getColourData().reflectivity(reflectivity);
                     ret = true;
                 }
                 F32 metallic = material->getColourData().metallic();
                 if (ImGui::SliderFloat(" - Shininess", &metallic, 0.0f, 1.0f)) {
                     material->getColourData().metallic(metallic);
                     ret = true;
                 }
                 F32 roughness = material->getColourData().roughness();
                 if (ImGui::SliderFloat(" - Roughness", &roughness, 0.0f, 1.0f)) {
                     material->getColourData().roughness(roughness);
                     ret = true;
                 }
             } else {
                 auto specularSetter = [material](const void* data) {
                     material->getColourData().specular(*(FColour3*)data);
                 };
                 FColour3 specular = material->getColourData().specular();
                 if (colourInput3(_parent, " - Specular", specular, readOnly, specularSetter)) {
                     specularSetter(specular._v);
                     ret = true;
                 }

                 F32 shininess = material->getColourData().shininess();
                 if (ImGui::SliderFloat(" - Shininess", &shininess, 0.0f, 1.0f)) {
                     material->getColourData().shininess(shininess);
                     ret = true;
                 }
             }

             bool doubleSided = material->isDoubleSided();
             bool receivesShadows = material->receivesShadows();

             ret = ImGui::Checkbox("DoubleSided", &doubleSided) || ret;
             ret = ImGui::Checkbox("ReceivesShadows", &receivesShadows) || ret;
             if (!readOnly) {
                 material->setDoubleSided(doubleSided);
                 material->setReceivesShadows(receivesShadows);
             } else {
                 ImGui::PopStyleVar();
                 ImGui::PopItemFlag();
             }

             if (readOnly) {
                 ret = false;
             }
         }

         return ret;
     }


     bool PropertyWindow::processBasicField(EditorComponentField& field) {
         const bool isSlider = field._type == EditorComponentFieldType::SLIDER_TYPE &&
                               field._basicType != GFX::PushConstantType::BOOL &&
                               !field.isMatrix();

         const ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue |
                                           ImGuiInputTextFlags_CharsNoBlank |
                                           ImGuiInputTextFlags_CharsDecimal |
                                           (field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);

         const char* name = field._name.c_str();
         ImGui::PushID(name);

         bool ret = false;
         switch (field._basicType) {
             case GFX::PushConstantType::BOOL: {
                 bool val = field.get<bool>();
                 ret = ImGui::Checkbox("", &val);
                 if (ret && !field._readOnly) {
                     field.set<bool>(val);
                 }
             }break;
             case GFX::PushConstantType::INT: {
                switch (field._basicTypeSize) {
                    case GFX::PushConstantSize::QWORD: ret = inputOrSlider<I64, 1>(_parent, isSlider, "", name, ImGuiDataType_S64, field, flags); break;
                    case GFX::PushConstantSize::DWORD: ret = inputOrSlider<I32, 1>(_parent, isSlider, "", name, ImGuiDataType_S32, field, flags); break;
                    case GFX::PushConstantSize::WORD:  ret = inputOrSlider<I16, 1>(_parent, isSlider, "", name, ImGuiDataType_S16, field, flags); break;
                    case GFX::PushConstantSize::BYTE:  ret = inputOrSlider<I8,  1>(_parent, isSlider, "", name, ImGuiDataType_S8,  field, flags); break;
                    default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::UINT: {
                 switch (field._basicTypeSize) {
                     case GFX::PushConstantSize::QWORD: ret = inputOrSlider<U64, 1>(_parent, isSlider, "", name, ImGuiDataType_U64, field, flags); break;
                     case GFX::PushConstantSize::DWORD: ret = inputOrSlider<U32, 1>(_parent, isSlider, "", name, ImGuiDataType_U32, field, flags); break;
                     case GFX::PushConstantSize::WORD:  ret = inputOrSlider<U16, 1>(_parent, isSlider, "", name, ImGuiDataType_U16, field, flags); break;
                     case GFX::PushConstantSize::BYTE:  ret = inputOrSlider<U8,  1>(_parent, isSlider, "", name, ImGuiDataType_U8,  field, flags); break;
                    default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::DOUBLE: {
                 ret = inputOrSlider<D64, 1>(_parent, isSlider, "", name, ImGuiDataType_Double, field, flags, "%.6f");
             }break;
             case GFX::PushConstantType::FLOAT: {
                 ret = inputOrSlider<F32, 1>(_parent, isSlider, "", name, ImGuiDataType_Float, field, flags, "%.3f");
             }break;
             case GFX::PushConstantType::IVEC2: {
                 switch (field._basicTypeSize) {
                     case GFX::PushConstantSize::QWORD: ret = inputOrSlider<vec2<I64>, 2>(_parent, isSlider, "", name, ImGuiDataType_S64, field, flags); break;
                     case GFX::PushConstantSize::DWORD: ret = inputOrSlider<vec2<I32>, 2>(_parent, isSlider, "", name, ImGuiDataType_S32, field, flags); break;
                     case GFX::PushConstantSize::WORD:  ret = inputOrSlider<vec2<I16>, 2>(_parent, isSlider, "", name, ImGuiDataType_S16, field, flags); break;
                     case GFX::PushConstantSize::BYTE:  ret = inputOrSlider<vec2<I8>,  2>(_parent, isSlider, "", name, ImGuiDataType_S8,  field, flags); break;
                     default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::IVEC3: {
                 switch (field._basicTypeSize) {
                     case GFX::PushConstantSize::QWORD: ret = inputOrSlider<vec3<I64>, 3>(_parent, isSlider, "", name, ImGuiDataType_S64, field, flags); break;
                     case GFX::PushConstantSize::DWORD: ret = inputOrSlider<vec3<I32>, 3>(_parent, isSlider, "", name, ImGuiDataType_S32, field, flags); break;
                     case GFX::PushConstantSize::WORD:  ret = inputOrSlider<vec3<I16>, 3>(_parent, isSlider, "", name, ImGuiDataType_S16, field, flags); break;
                     case GFX::PushConstantSize::BYTE:  ret = inputOrSlider<vec3<I8>,  3>(_parent, isSlider, "", name, ImGuiDataType_S8,  field, flags); break;
                     default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::IVEC4: {
                 switch (field._basicTypeSize) {
                     case GFX::PushConstantSize::QWORD: ret = inputOrSlider<vec4<I64>, 4>(_parent, isSlider, "", name, ImGuiDataType_S64, field, flags); break;
                     case GFX::PushConstantSize::DWORD: ret = inputOrSlider<vec4<I32>, 4>(_parent, isSlider, "", name, ImGuiDataType_S32, field, flags); break;
                     case GFX::PushConstantSize::WORD:  ret = inputOrSlider<vec4<I16>, 4>(_parent, isSlider, "", name, ImGuiDataType_S16, field, flags); break;
                     case GFX::PushConstantSize::BYTE:  ret = inputOrSlider<vec4<I8>,  4>(_parent, isSlider, "", name, ImGuiDataType_S8,  field, flags); break;
                     default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::UVEC2: {
                 switch (field._basicTypeSize) {
                     case GFX::PushConstantSize::QWORD: ret = inputOrSlider<vec2<U64>, 2>(_parent, isSlider, "", name, ImGuiDataType_U64, field, flags); break;
                     case GFX::PushConstantSize::DWORD: ret = inputOrSlider<vec2<U32>, 2>(_parent, isSlider, "", name, ImGuiDataType_U32, field, flags); break;
                     case GFX::PushConstantSize::WORD:  ret = inputOrSlider<vec2<U16>, 2>(_parent, isSlider, "", name, ImGuiDataType_U16, field, flags); break;
                     case GFX::PushConstantSize::BYTE:  ret = inputOrSlider<vec2<U8>,  2>(_parent, isSlider, "", name, ImGuiDataType_U8,  field, flags); break;
                     default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::UVEC3: {
                 switch (field._basicTypeSize) {
                     case GFX::PushConstantSize::QWORD: ret = inputOrSlider<vec3<U64>, 3>(_parent, isSlider, "", name, ImGuiDataType_U64, field, flags); break;
                     case GFX::PushConstantSize::DWORD: ret = inputOrSlider<vec3<U32>, 3>(_parent, isSlider, "", name, ImGuiDataType_U32, field, flags); break;
                     case GFX::PushConstantSize::WORD:  ret = inputOrSlider<vec3<U16>, 3>(_parent, isSlider, "", name, ImGuiDataType_U16, field, flags); break;
                     case GFX::PushConstantSize::BYTE:  ret = inputOrSlider<vec3<U8>,  3>(_parent, isSlider, "", name, ImGuiDataType_U8,  field, flags); break;
                     default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::UVEC4: {
                 switch (field._basicTypeSize) {
                     case GFX::PushConstantSize::QWORD: ret = inputOrSlider<vec4<U64>, 4>(_parent, isSlider, "", name, ImGuiDataType_U64, field, flags); break;
                     case GFX::PushConstantSize::DWORD: ret = inputOrSlider<vec4<U32>, 4>(_parent, isSlider, "", name, ImGuiDataType_U32, field, flags); break;
                     case GFX::PushConstantSize::WORD:  ret = inputOrSlider<vec4<U16>, 4>(_parent, isSlider, "", name, ImGuiDataType_U16, field, flags); break;
                     case GFX::PushConstantSize::BYTE:  ret = inputOrSlider<vec4<U8>,  4>(_parent, isSlider, "", name, ImGuiDataType_U8,  field, flags); break;
                     default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::VEC2: {
                 ret = inputOrSlider<vec2<F32>, 2>(_parent, isSlider, "", name, ImGuiDataType_Float, field, flags, "%.3f");
             }break;
             case GFX::PushConstantType::VEC3: {
                 ret = inputOrSlider<vec3<F32>, 3>(_parent, isSlider, "", name, ImGuiDataType_Float, field, flags, "%.3f");
             }break;
             case GFX::PushConstantType::VEC4: {
                 ret = inputOrSlider<vec4<F32>, 4>(_parent, isSlider, "", name, ImGuiDataType_Float, field, flags, "%.3f");
             }break;
             case GFX::PushConstantType::DVEC2: {
                 ret = inputOrSlider<vec2<D64>, 2>(_parent, isSlider, "", name, ImGuiDataType_Double, field, flags, "%.6f");
             }break;
             case GFX::PushConstantType::DVEC3: {
                 ret = inputOrSlider<vec3<D64>, 3>(_parent, isSlider, "", name, ImGuiDataType_Double, field, flags, "%.6f");
             }break;
             case GFX::PushConstantType::DVEC4: {
                 ret = inputOrSlider<vec4<D64>, 4>(_parent, isSlider, "", name, ImGuiDataType_Double, field, flags, "%.6f");
             }break;
             case GFX::PushConstantType::IMAT2: {
                 switch (field._basicTypeSize) {
                     case GFX::PushConstantSize::QWORD: ret = inputMatrix<mat2<I64>, 2>(_parent, "", name, ImGuiDataType_S64, field, flags); break;
                     case GFX::PushConstantSize::DWORD: ret = inputMatrix<mat2<I32>, 2>(_parent, "", name, ImGuiDataType_S32, field, flags); break;
                     case GFX::PushConstantSize::WORD:  ret = inputMatrix<mat2<I16>, 2>(_parent, "", name, ImGuiDataType_S16, field, flags); break;
                     case GFX::PushConstantSize::BYTE:  ret = inputMatrix<mat2<I8>,  2>(_parent, "", name, ImGuiDataType_S8,  field, flags); break;
                     default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::IMAT3: {
                 switch (field._basicTypeSize) {
                     case GFX::PushConstantSize::QWORD: ret = inputMatrix<mat3<I64>, 3>(_parent, "", name, ImGuiDataType_S64, field, flags); break;
                     case GFX::PushConstantSize::DWORD: ret = inputMatrix<mat3<I32>, 3>(_parent, "", name, ImGuiDataType_S32, field, flags); break;
                     case GFX::PushConstantSize::WORD:  ret = inputMatrix<mat3<I16>, 3>(_parent, "", name, ImGuiDataType_S16, field, flags); break;
                     case GFX::PushConstantSize::BYTE:  ret = inputMatrix<mat3<I8>,  3>(_parent, "", name, ImGuiDataType_S8,  field, flags); break;
                     default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::IMAT4: {
                 switch (field._basicTypeSize) {
                 case GFX::PushConstantSize::QWORD: ret = inputMatrix<mat4<I64>, 4>(_parent, "", name, ImGuiDataType_S64, field, flags); break;
                 case GFX::PushConstantSize::DWORD: ret = inputMatrix<mat4<I32>, 4>(_parent, "", name, ImGuiDataType_S32, field, flags); break;
                 case GFX::PushConstantSize::WORD:  ret = inputMatrix<mat4<I16>, 4>(_parent, "", name, ImGuiDataType_S16, field, flags); break;
                 case GFX::PushConstantSize::BYTE:  ret = inputMatrix<mat4<I8>,  4>(_parent, "", name, ImGuiDataType_S8,  field, flags); break;
                     default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::UMAT2: {
                 switch (field._basicTypeSize) {
                     case GFX::PushConstantSize::QWORD: ret = inputMatrix<mat2<U64>, 2>(_parent, "", name, ImGuiDataType_U64, field, flags); break;
                     case GFX::PushConstantSize::DWORD: ret = inputMatrix<mat2<U32>, 2>(_parent, "", name, ImGuiDataType_U32, field, flags); break;
                     case GFX::PushConstantSize::WORD:  ret = inputMatrix<mat2<U16>, 2>(_parent, "", name, ImGuiDataType_U16, field, flags); break;
                     case GFX::PushConstantSize::BYTE:  ret = inputMatrix<mat2<U8>,  2>(_parent, "", name, ImGuiDataType_U8,  field, flags); break;
                     default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::UMAT3: {
                 switch (field._basicTypeSize) {
                     case GFX::PushConstantSize::QWORD: ret = inputMatrix<mat3<U64>, 3>(_parent, "", name, ImGuiDataType_U64, field, flags); break;
                     case GFX::PushConstantSize::DWORD: ret = inputMatrix<mat3<U32>, 3>(_parent, "", name, ImGuiDataType_U32, field, flags); break;
                     case GFX::PushConstantSize::WORD:  ret = inputMatrix<mat3<U16>, 3>(_parent, "", name, ImGuiDataType_U16, field, flags); break;
                     case GFX::PushConstantSize::BYTE:  ret = inputMatrix<mat3<U8>,  3>(_parent, "", name, ImGuiDataType_U8,  field, flags); break;
                     default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::UMAT4: {
                 switch (field._basicTypeSize) {
                     case GFX::PushConstantSize::QWORD: ret = inputMatrix<mat4<U64>, 4>(_parent, "", name, ImGuiDataType_U64, field, flags); break;
                     case GFX::PushConstantSize::DWORD: ret = inputMatrix<mat4<U32>, 4>(_parent, "", name, ImGuiDataType_U32, field, flags); break;
                     case GFX::PushConstantSize::WORD:  ret = inputMatrix<mat4<U16>, 4>(_parent, "", name, ImGuiDataType_U16, field, flags); break;
                     case GFX::PushConstantSize::BYTE:  ret = inputMatrix<mat4<U8>,  4>(_parent, "", name, ImGuiDataType_U8,  field, flags); break;
                     default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::MAT2: {
                 ret = inputMatrix<mat2<F32>, 2>(_parent, "", name, ImGuiDataType_Float, field, flags, "%.3f");
             }break;
             case GFX::PushConstantType::MAT3: {
                 ret = inputMatrix<mat3<F32>, 3>(_parent, "", name, ImGuiDataType_Float, field, flags, "%.3f");
             }break;
             case GFX::PushConstantType::MAT4: {
                 ret = inputMatrix<mat4<F32>, 4>(_parent, "", name, ImGuiDataType_Float, field, flags, "%.3f");
             }break;
             case GFX::PushConstantType::DMAT2: {
                 ret = inputMatrix<mat2<D64>, 2>(_parent, "", name, ImGuiDataType_Double, field, flags, "%.6f");
             }break;
             case GFX::PushConstantType::DMAT3: {
                 ret = inputMatrix<mat3<D64>, 3>(_parent, "", name, ImGuiDataType_Double, field, flags, "%.6f");
             }break;
             case GFX::PushConstantType::DMAT4: {
                 ret = inputMatrix<mat4<D64>, 4>(_parent, "", name, ImGuiDataType_Double, field, flags, "%.6f");
             }break;
             case GFX::PushConstantType::FCOLOUR3: {
                 ret = colourInput3(_parent, "", field);
             }break;
             case GFX::PushConstantType::FCOLOUR4: {
                 ret = colourInput4(_parent, "", field);
             }break;
             default: {
                 ImGui::Text(name);
             }break;
         }

         ImGui::Spacing();
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
