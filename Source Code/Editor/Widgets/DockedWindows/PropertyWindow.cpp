#include "stdafx.h"

#include "Headers/PropertyWindow.h"
#include "Headers/UndoManager.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/PlatformContext.h"

#include "Editor/Headers/Editor.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Geometry/Material/Headers/Material.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

#include "ECS/Components/Headers/TransformComponent.h"
#include "ECS/Components/Headers/SpotLightComponent.h"
#include "ECS/Components/Headers/PointLightComponent.h"
#include "ECS/Components/Headers/DirectionalLightComponent.h"

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

    bool PropertyWindow::drawCamera(Camera* cam) {
        bool sceneChanged = false;
        if (cam == nullptr) {
            return false;
        }

        if (ImGui::CollapsingHeader(cam->resourceName().c_str())) {
            vec3<F32> eye = cam->getEye();
            if (ImGui::InputFloat3("Eye", eye._v)) {
                cam->setEye(eye);
                sceneChanged = true;
            }
            vec3<F32> euler = cam->getEuler();
            if (ImGui::InputFloat3("Euler", euler._v)) {
                cam->setEuler(euler);
                sceneChanged = true;
            }

            F32 aspect = cam->getAspectRatio();
            if (ImGui::InputFloat("Aspect", &aspect)) {
                cam->setAspectRatio(aspect);
                sceneChanged = true;
            }

            F32 horizontalFoV = cam->getHorizontalFoV();
            if (ImGui::InputFloat("FoV (horizontal)", &horizontalFoV)) {
                cam->setHorizontalFoV(horizontalFoV);
                sceneChanged = true;
            }

            vec2<F32> zPlanes = cam->getZPlanes();
            if (ImGui::InputFloat2("zPlanes", zPlanes._v)) {
                if (cam->isOrthoProjected()) {
                    cam->setProjection(cam->orthoRect(), zPlanes);
                }
                else {
                    cam->setProjection(cam->getAspectRatio(), cam->getVerticalFoV(), zPlanes);
                }
                sceneChanged = true;
            }

            if (cam->isOrthoProjected()) {
                vec4<F32> orthoRect = cam->orthoRect();
                if (ImGui::InputFloat4("Ortho", orthoRect._v)) {
                    cam->setProjection(orthoRect, cam->getZPlanes());
                    sceneChanged = true;
                }
            }

            ImGui::Text("View Matrix");
            ImGui::Spacing();
            mat4<F32> viewMatrix = cam->getViewMatrix();
            EditorComponentField worldMatrixField;
            worldMatrixField._name = "View Matrix";
            worldMatrixField._basicType = GFX::PushConstantType::MAT4;
            worldMatrixField._type = EditorComponentFieldType::PUSH_TYPE;
            worldMatrixField._readOnly = true;
            worldMatrixField._data = &viewMatrix;

            if (processBasicField(worldMatrixField)) {
                sceneChanged = true;
            }

            ImGui::Text("Projection Matrix");
            ImGui::Spacing();
            mat4<F32> projMatrix = cam->getProjectionMatrix();
            EditorComponentField projMatrixField;
            projMatrixField._basicType = GFX::PushConstantType::MAT4;
            projMatrixField._type = EditorComponentFieldType::PUSH_TYPE;
            projMatrixField._readOnly = true;
            projMatrixField._name = "Projection Matrix";
            projMatrixField._data = &projMatrix;

            if (processBasicField(projMatrixField)) {
                sceneChanged = true;
            }
        }

        return sceneChanged;
    }

    namespace {
        bool isRequiredComponentType(SceneGraphNode* selection, const ComponentType componentType) {
            if (selection != nullptr) {
                return BitCompare(selection->getNode().requiredComponentMask(), to_U32(componentType));
            }

            return false;
        }
    };

    void PropertyWindow::drawInternal() {
        bool sceneChanged = false;

        const Selections crtSelections = selections();
        const bool hasSelections = crtSelections._selectionCount > 0u;

        Camera* selectedCamera = Attorney::EditorPropertyWindow::getSelectedCamera(_parent);
        if (selectedCamera != nullptr) {
            sceneChanged = drawCamera(selectedCamera);
        } else if (hasSelections) {
            const F32 smallButtonWidth = 60.0f;
            F32 xOffset = ImGui::GetWindowSize().x * 0.5f - smallButtonWidth;

            static bool closed = false;
            for (U8 i = 0u; i < crtSelections._selectionCount; ++i) {
                SceneGraphNode* sgnNode = node(crtSelections._selections[i]);

                if (sgnNode != nullptr) {
                    bool enabled = sgnNode->hasFlag(SceneGraphNode::Flags::ACTIVE);
                    if (ImGui::Checkbox(sgnNode->name().c_str(), &enabled)) {
                        if (enabled) {
                            sgnNode->setFlag(SceneGraphNode::Flags::ACTIVE);
                        } else {
                            sgnNode->clearFlag(SceneGraphNode::Flags::ACTIVE);
                        }
                        sceneChanged = true;
                    }
                    ImGui::Separator();

                    vectorSTDFast<EditorComponent*>& editorComp = Attorney::SceneGraphNodeEditor::editorComponents(*sgnNode);
                    for (EditorComponent* comp : editorComp) {
                        if (ImGui::CollapsingHeader(comp->name().c_str(), ImGuiTreeNodeFlags_OpenOnArrow)) {
                            //const I32 RV = ImGui::AppendTreeNodeHeaderButtons(comp->name().c_str(), ImGui::GetCursorPosX(), 1, &closed, "Remove", NULL, 0);
                            ImGui::NewLine();
                            ImGui::SameLine(xOffset);
                            if (ImGui::Button("INSPECT", ImVec2(smallButtonWidth, 20))) {
                                Attorney::EditorGeneralWidget::inspectMemory(_context.editor(), std::make_pair(comp, sizeof(EditorComponent)));
                            }

                            if (comp->parentComponentType()._value != ComponentType::COUNT && !isRequiredComponentType(sgnNode, comp->parentComponentType())) {
                                ImGui::SameLine();
                                if (ImGui::Button("REMOVE", ImVec2(smallButtonWidth, 20))) {
                                    Attorney::EditorGeneralWidget::inspectMemory(_context.editor(), std::make_pair(nullptr, 0));

                                    if (Attorney::EditorGeneralWidget::removeComponent(_context.editor(), sgnNode, comp->parentComponentType())) {
                                        sceneChanged = true;
                                        continue;
                                    }
                                }
                            }

                            ImGui::Separator();

                            vectorSTD<EditorComponentField>& fields = Attorney::EditorComponentEditor::fields(*comp);
                            for (EditorComponentField& field : fields) {
                                ImGui::Text(field._name.c_str());
                                //ImGui::NewLine();
                                if (processField(field) && !field._readOnly) {
                                    Attorney::EditorComponentEditor::onChanged(*comp, field);
                                    sceneChanged = true;
                                }
                                ImGui::Spacing();
                            }

                            // Show light/shadow specific options (if any)
                            const U32 componentMask = sgnNode->componentMask();
                            Light* light = nullptr;
                            if (BitCompare(componentMask, ComponentType::SPOT_LIGHT)) {
                                light = sgnNode->get<SpotLightComponent>();
                            } else if (BitCompare(componentMask, ComponentType::POINT_LIGHT)) {
                                light = sgnNode->get<PointLightComponent>();
                            } else if (BitCompare(componentMask, ComponentType::DIRECTIONAL_LIGHT)) {
                                light = sgnNode->get<DirectionalLightComponent>();
                            }
                            if (light != nullptr) {
                                bool isShadowCaster = light->castsShadows();
                                if (ImGui::Checkbox("Is Shadow Caster", &isShadowCaster)) {
                                    light->castsShadows(isShadowCaster);
                                    sceneChanged = true;
                                }
                                if (isShadowCaster) {
                                    if (ImGui::CollapsingHeader("Light Shadow Settings", ImGuiTreeNodeFlags_OpenOnArrow)) {
                                        ImGui::Text("Shadow Offset: %d", to_U32(light->getShadowOffset()));

                                        switch (light->getLightType()) {
                                            case LightType::POINT: {
                                                PointLightComponent* pointLight = static_cast<PointLightComponent*>(light);
                                                ACKNOWLEDGE_UNUSED(pointLight);
                                            } break;

                                            case LightType::SPOT: {
                                                SpotLightComponent* spotLight = static_cast<SpotLightComponent*>(light);
                                                ACKNOWLEDGE_UNUSED(spotLight);
                                            } break;

                                            case LightType::DIRECTIONAL: {
                                                DirectionalLightComponent* dirLight = static_cast<DirectionalLightComponent*>(light);
                                                for (U8 split = 0; split < dirLight->csmSplitCount(); ++split) {
                                                    Camera* shadowCamera = dirLight->shadowCameras()[split];
                                                    if (drawCamera(shadowCamera)) {
                                                        sceneChanged = true;
                                                    }
                                                }
                                            } break;

                                            default: {
                                                DIVIDE_UNEXPECTED_CALL();
                                            } break;
                                        }
                                    }
                                }

                                if (ImGui::CollapsingHeader("Scene Shadow Settings", ImGuiTreeNodeFlags_OpenOnArrow)) {
                                    SceneManager* sceneManager = context().kernel().sceneManager();
                                    SceneState& activeSceneState = sceneManager->getActiveScene().state();
                                    F32 bleedBias = activeSceneState.lightBleedBias();
                                    if (ImGui::SliderFloat("Light bleed bias", &bleedBias, 0.0f, 1.0f)) {
                                        activeSceneState.lightBleedBias(bleedBias);
                                        sceneChanged = true;
                                    }

                                    F32 shadowVariance = activeSceneState.minShadowVariance();
                                    if (ImGui::SliderFloat("Min shadow variance", &shadowVariance, 0.0f, 1.0f)) {
                                        activeSceneState.minShadowVariance(shadowVariance);
                                        sceneChanged = true;
                                    }

                                    constexpr U16 min = 1u, max = 1000u;
                                    U16 shadowFadeDistance = activeSceneState.shadowFadeDistance();
                                    if (ImGui::SliderScalar("Shadow fade distance", ImGuiDataType_U16, &shadowFadeDistance, &min, &max)) {
                                        activeSceneState.shadowFadeDistance(shadowFadeDistance);
                                        sceneChanged = true;
                                    } 
                                    
                                    U16 shadowMaxDistance = activeSceneState.shadowDistance();
                                    if (ImGui::SliderScalar("Shadow max distance", ImGuiDataType_U16, &shadowMaxDistance, &min, &max)) {
                                        activeSceneState.shadowDistance(shadowMaxDistance);
                                        sceneChanged = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            //ToDo: Speed this up. Also, how do we handle adding stuff like RenderingComponents and creating materials and the like?
            const auto validComponentToAdd = [this, &crtSelections](const ComponentType type) -> bool {
                if (type._value == ComponentType::COUNT) {
                    return false;
                }

                if (type._value == ComponentType::SCRIPT) {
                    return true;
                }

                bool missing = false;
                for (U8 i = 0u; i < crtSelections._selectionCount; ++i) {
                    SceneGraphNode* sgn = node(crtSelections._selections[i]);
                    if (sgn != nullptr && !BitCompare(sgn->componentMask(), to_U32(type))) {
                        missing = true;
                        break;
                    }
                }

                return missing;
            };

            constexpr F32 buttonWidth = 80.0f;

            xOffset = ImGui::GetWindowSize().x - buttonWidth - 20.0f;
            ImGui::NewLine();
            ImGui::Separator();
            ImGui::NewLine();
            ImGui::SameLine(xOffset);
            if (ImGui::Button("ADD NEW", ImVec2(buttonWidth, 15))) {
                ImGui::OpenPopup("COMP_SELECTION_GROUP");
            }

            static ComponentType selectedType = ComponentType::COUNT;

            if (ImGui::BeginPopup("COMP_SELECTION_GROUP")) {
                for (const ComponentType type : ComponentType::_values()) {
                    if (type._to_integral() == ComponentType::COUNT || !validComponentToAdd(type)) {
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
                    if (Attorney::EditorGeneralWidget::addComponent(_context.editor(), crtSelections, selectedType)) {
                        sceneChanged = true;
                    }
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
        } else {
            ImGui::Separator();
            ImGui::Text("Please select a scene node to inspect its properties");
            ImGui::Separator();
        }

        if (sceneChanged) {
            Attorney::EditorGeneralWidget::registerUnsavedSceneChanges(_context.editor());
        }
    }
    
    const Selections& PropertyWindow::selections() const {
        static Selections selections;

        const Scene& activeScene = context().kernel().sceneManager()->getActiveScene();

        selections = activeScene.getCurrentSelection();
        return selections;
    }
    
    SceneGraphNode* PropertyWindow::node(I64 guid) const {
        const Scene& activeScene = context().kernel().sceneManager()->getActiveScene();

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

        TransformValues transformValues = transform->getValues();

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

        template<typename T, size_t num_comp>
        bool inputOrSlider(Editor& parent, bool slider, const char* label, const char* name, const float stepIn, ImGuiDataType data_type, EditorComponentField& field, ImGuiInputTextFlags flags, const char* format = "%d", float power = 1.0f) {
            static UndoEntry<T> undo = {};

            if (slider && field._readOnly) {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }

            T val = field.get<T>();
            const T min = T(field._range.min), max = T(field._range.max);
            const T cStep = T(stepIn * 100);

            const void* step = IS_ZERO(stepIn) ? nullptr : (void*)&stepIn;
            const void* step_fast = step == nullptr ? nullptr : (void*)&cStep;

            bool ret = false;
            if_constexpr (num_comp == 1) {
                if (slider) {
                    ret = ImGui::SliderScalar(label, data_type, (void*)&val, (void*)&min, (void*)&max, format, power);
                } else {
                    ret = ImGui::InputScalar(label, data_type, (void*)&val, step, step_fast, format, flags);
                }
            } else {
                if (slider) {
                    ret = ImGui::SliderScalarN(label, data_type, (void*)&val, num_comp, (void*)&min, (void*)&max, format, power);
                } else {
                    ret = ImGui::InputScalarN(label, data_type, (void*)&val, num_comp, step, step_fast, format, flags);
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

        template<typename T, size_t num_rows>
        bool inputMatrix(Editor & parent, const char* label, const char* name, const float stepIn, ImGuiDataType data_type, EditorComponentField& field, ImGuiInputTextFlags flags, const char* format = "%d") {
            static UndoEntry<T> undo = {};

            const T cStep = T(stepIn * 100);

            const void* step = IS_ZERO(stepIn) ? nullptr : (void*)&stepIn;
            const void* step_fast = step == nullptr ? nullptr : (void*)&cStep;

            T mat = field.get<T>();
            bool ret = ImGui::InputScalarN(label, data_type, (void*)mat._vec[0]._v, num_rows, step, step_fast, format, flags) ||
                       ImGui::InputScalarN(label, data_type, (void*)mat._vec[1]._v, num_rows, step, step_fast, format, flags);
            if_constexpr(num_rows > 2) {
                ret = ImGui::InputScalarN(label, data_type, (void*)mat._vec[2]._v, num_rows, step, step_fast, format, flags) || ret;
                if_constexpr(num_rows > 3) {
                    ret = ImGui::InputScalarN(label, data_type, (void*)mat._vec[3]._v, num_rows, step, step_fast, format, flags) || ret;
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

             static RenderStagePass currentStagePass = {};
             {
                 const char* crtStage = TypeUtil::RenderStageToString(currentStagePass._stage);
                 if (ImGui::BeginCombo("Stage", crtStage, ImGuiComboFlags_PopupAlignLeft)) {
                     for (U8 n = 0; n < to_U8(RenderStage::COUNT); ++n) {
                        const RenderStage stage = static_cast<RenderStage>(n);
                        const bool isSelected = currentStagePass._stage == stage;

                        if (ImGui::Selectable(TypeUtil::RenderStageToString(stage), isSelected)) {
                            currentStagePass._stage = stage;
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                     }
                     ImGui::EndCombo();
                 }

                 const char* crtPass = TypeUtil::RenderPassTypeToString(currentStagePass._passType);
                 if (ImGui::BeginCombo("Pass", crtPass, ImGuiComboFlags_PopupAlignLeft)) {
                     for (U8 n = 0; n < to_U8(RenderPassType::COUNT); ++n) {
                         const RenderPassType pass = static_cast<RenderPassType>(n);
                         const bool isSelected = currentStagePass._passType == pass;

                         if (ImGui::Selectable(TypeUtil::RenderPassTypeToString(pass), isSelected)) {
                             currentStagePass._passType = pass;
                         }
                         if (isSelected) {
                             ImGui::SetItemDefaultFocus();
                         }
                     }
                     ImGui::EndCombo();
                 }

                 constexpr U8 min = 0u, max = Material::g_maxVariantsPerPass;
                 ImGui::SliderScalar("Variant", ImGuiDataType_U8, &currentStagePass._variant, &min, &max);
                 ImGui::InputScalar("Pass Index", ImGuiDataType_U32, &currentStagePass._passIndex);
                 ImGui::InputScalar("Index A", ImGuiDataType_U16, &currentStagePass._indexA);
                 ImGui::InputScalar("Index B", ImGuiDataType_U16, &currentStagePass._indexA);
             }

             size_t stateHash = 0;
             stringImpl shaderName = "None";
             ShaderProgram* program = nullptr;
             if (currentStagePass._stage != RenderStage::COUNT && currentStagePass._passType != RenderPassType::COUNT) {
                 const I64 shaderGUID = material->getProgramGUID(currentStagePass);
                 program = ShaderProgram::findShaderProgram(shaderGUID);
                 if (program != nullptr) {
                     shaderName = program->resourceName().c_str();
                 }
                 stateHash = material->getRenderStateBlock(currentStagePass);
             }

             if (ImGui::CollapsingHeader(("Program: " + shaderName).c_str())) {
                 if (program != nullptr) {
                     const ShaderProgramDescriptor& descriptor = program->descriptor();
                     for (const ShaderModuleDescriptor& module : descriptor._modules) {
                         const char* stages[] = { "PS", "VS", "GS" "HS", "DS","CS" };
                         if (ImGui::CollapsingHeader(Util::StringFormat("%s: File [ %s ] Variant [ %s ]",
                                                                        stages[to_base(module._moduleType)],
                                                                        module._sourceFile.c_str(),
                                                                        module._variant.empty() ? "-" : module._variant.c_str()).c_str())) 
                         {
                             ImGui::Text("Defines: ");
                             ImGui::Separator();
                             for (const auto& define : module._defines) {
                                ImGui::Text(define.first.c_str());
                             }
                             if (ImGui::Button("Open Source File")) {
                                 if (!openFile((program->assetLocation() + Paths::Shaders::GLSL::g_parentShaderLoc.c_str()).c_str(), module._sourceFile.c_str())) {
                                     Attorney::EditorGeneralWidget::showStatusMessage(_context.editor(), "ERROR: Couldn't open specified source file!", Time::SecondsToMilliseconds<F32>(3));
                                 }
                             }
                         }
                     }
                     ImGui::Separator();
                     if (ImGui::Button("Rebuild from source") && !readOnly) {
                         Attorney::EditorGeneralWidget::showStatusMessage(_context.editor(), "Rebuilding shader from source ...", Time::SecondsToMilliseconds<F32>(3));
                         if (!program->recompile(true)) {
                             Attorney::EditorGeneralWidget::showStatusMessage(_context.editor(), "ERROR: Failed to rebuild shader from source!", Time::SecondsToMilliseconds<F32>(3));
                         } else {
                             Attorney::EditorGeneralWidget::showStatusMessage(_context.editor(), "Rebuilt shader from source!", Time::SecondsToMilliseconds<F32>(3));
                             ret = true;
                         }
                     }
                     ImGui::Separator();
                 }
             }

             if (ImGui::CollapsingHeader(Util::StringFormat("Render State: %zu", stateHash).c_str()) && stateHash > 0)
             {
                RenderStateBlock block = RenderStateBlock::get(stateHash);
                bool changed = false;
                {
                    const P32 colourWrite = block.colourWrite();
                    bool R = colourWrite.b[0] == 1;
                    bool G = colourWrite.b[1] == 1;
                    bool B = colourWrite.b[2] == 1;
                    bool A = colourWrite.b[3] == 1;
                    if (ImGui::Checkbox("R", &R)) {
                        block.setColourWrites(R, G, B, A);
                        changed = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::Checkbox("R", &G)) {
                        block.setColourWrites(R, G, B, A);
                        changed = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::Checkbox("R", &B)) {
                        block.setColourWrites(R, G, B, A);
                        changed = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::Checkbox("R", &A)) {
                        block.setColourWrites(R, G, B, A);
                        changed = true;
                    }
                }

                F32 bias[] = { block.zBias(), block.zUnits() };
                if (ImGui::InputFloat2("ZBias", bias)) {
                    block.setZBias(bias[0], bias[1]);
                    changed = true;
                }

                ImGui::Text("Tessellation control points: %d", block.tessControlPoints());
                {
                    CullMode cMode = block.cullMode();
                    const char* cullModes[] = { "None", "CW/BACK", "CCW/FRONT", "ALL", "ERROR!" };
                    const char* crtMode = cullModes[to_base(cMode)];
                    if (ImGui::BeginCombo("Cull Mode", crtMode, ImGuiComboFlags_PopupAlignLeft)) {
                        for (U8 n = 0; n < to_U8(CullMode::COUNT); ++n) {
                            const CullMode mode = static_cast<CullMode>(n);
                            const bool isSelected = cMode == mode;

                            if (ImGui::Selectable(cullModes[to_base(mode)], isSelected)) {
                                cMode = mode;
                                block.setCullMode(mode);
                                changed = true;
                            }
                            if (isSelected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                }

                {
                    FillMode fMode = block.fillMode();
                    const char* fillModes[] = { "Point", "Wireframe", "Solid", "ERROR!" };
                    const char* crtMode = fillModes[to_base(fMode)];
                    if (ImGui::BeginCombo("Cull Mode", crtMode, ImGuiComboFlags_PopupAlignLeft)) {
                        for (U8 n = 0; n < to_U8(FillMode::COUNT); ++n) {
                            const FillMode mode = static_cast<FillMode>(n);
                            const bool isSelected = fMode == mode;

                            if (ImGui::Selectable(fillModes[to_base(mode)], isSelected)) {
                                fMode = mode;
                                block.setFillMode(mode);
                                changed = true;
                            }
                            if (isSelected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                }

                U32 stencilReadMask = block.stencilMask();
                U32 stencilWriteMask = block.stencilWriteMask();
                if (ImGui::InputScalar("Stencil mask", ImGuiDataType_U32, &stencilReadMask, NULL, NULL, "%08X", ImGuiInputTextFlags_CharsHexadecimal)) {
                    block.setStencilReadWriteMask(stencilReadMask, stencilWriteMask);
                    changed = true;
                }

                if (ImGui::InputScalar("Stencil write mask", ImGuiDataType_U32, &stencilWriteMask, NULL, NULL, "%08X", ImGuiInputTextFlags_CharsHexadecimal)) {
                    block.setStencilReadWriteMask(stencilReadMask, stencilWriteMask);
                    changed = true;
                }

                bool stencilDirty = false;
                U32 stencilRef = block.stencilRef();
                if (ImGui::InputScalar("Stencil reference mask", ImGuiDataType_U32, &stencilRef, NULL, NULL, "%08X", ImGuiInputTextFlags_CharsHexadecimal)) {
                    stencilDirty = true;
                }

                const char* compFunctionNames[] = {
                "NEVER", "LESS", "EQUAL", "LEQUAL", "GREATER", "NEQUAL", "GEQUAL", "ALWAYS", "ERROR"
                };

                const char* stencilOpNames[] = {
                "KEEP", "ZERO", "REPLACE", "INCREMENT", "DECREMENT", "INVERT", "INCREMENT_WRAP", "DECREMENT_WRAP", "ERROR"
                };

                {
                    ComparisonFunction zFunc = block.zFunc();
                    const char* crtMode = compFunctionNames[to_base(zFunc)];
                    if (ImGui::BeginCombo("Depth function", crtMode, ImGuiComboFlags_PopupAlignLeft)) {
                        for (U8 n = 0; n < to_U8(ComparisonFunction::COUNT); ++n) {
                            const ComparisonFunction func = static_cast<ComparisonFunction>(n);
                            const bool isSelected = zFunc == func;

                            if (ImGui::Selectable(compFunctionNames[to_base(func)], isSelected)) {
                                zFunc = func;
                                block.setZFunc(func);
                                changed = true;
                            }
                            if (isSelected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                }
                StencilOperation sFailOp = block.stencilFailOp();
                {
                    const char* crtMode = stencilOpNames[to_base(sFailOp)];
                    if (ImGui::BeginCombo("Stencil fail op", crtMode, ImGuiComboFlags_PopupAlignLeft)) {
                        for (U8 n = 0; n < to_U8(StencilOperation::COUNT); ++n) {
                            const StencilOperation op = static_cast<StencilOperation>(n);
                            const bool isSelected = sFailOp == op;

                            if (ImGui::Selectable(stencilOpNames[to_base(op)], isSelected)) {
                                sFailOp = op;
                                stencilDirty = true;
                            }
                            if (isSelected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                }
                StencilOperation sZFailOp = block.stencilZFailOp();
                {
                    const char* crtMode = stencilOpNames[to_base(sZFailOp)];
                    if (ImGui::BeginCombo("Stencil depth fail op", crtMode, ImGuiComboFlags_PopupAlignLeft)) {
                        for (U8 n = 0; n < to_U8(StencilOperation::COUNT); ++n) {
                            const StencilOperation op = static_cast<StencilOperation>(n);
                            const bool isSelected = sZFailOp == op;

                            if (ImGui::Selectable(stencilOpNames[to_base(op)], isSelected)) {
                                sZFailOp = op;
                                stencilDirty = true;
                            }
                            if (isSelected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                }
                StencilOperation sPassOp = block.stencilPassOp();
                {
                    const char* crtMode = stencilOpNames[to_base(sPassOp)];
                    if (ImGui::BeginCombo("Stencil pass op", crtMode, ImGuiComboFlags_PopupAlignLeft)) {
                        for (U8 n = 0; n < to_U8(StencilOperation::COUNT); ++n) {
                            const StencilOperation op = static_cast<StencilOperation>(n);
                            const bool isSelected = sPassOp == op;

                            if (ImGui::Selectable(stencilOpNames[to_base(op)], isSelected)) {
                                sPassOp = op;
                                stencilDirty = true;
                            }
                            if (isSelected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                }
                ComparisonFunction sFunc = block.stencilFunc();
                {
                    const char* crtMode = compFunctionNames[to_base(sFunc)];
                    if (ImGui::BeginCombo("Stencil function", crtMode, ImGuiComboFlags_PopupAlignLeft)) {
                        for (U8 n = 0; n < to_U8(ComparisonFunction::COUNT); ++n) {
                            const ComparisonFunction mode = static_cast<ComparisonFunction>(n);
                            const bool isSelected = sFunc == mode;

                            if (ImGui::Selectable(compFunctionNames[to_base(mode)], isSelected)) {
                                sFunc = mode;
                                stencilDirty = true;
                            }
                            if (isSelected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                }

                bool frontFaceCCW = block.frontFaceCCW();
                if (ImGui::Checkbox("CCW front face", &frontFaceCCW)) {
                    block.setFrontFaceCCW(frontFaceCCW);
                    changed = true;
                }

                bool scissorEnabled = block.scissorTestEnabled();
                if (ImGui::Checkbox("Scissor test", &scissorEnabled)) {
                    block.setScissorTest(scissorEnabled);
                    changed = true;
                }

                bool depthTestEnabled = block.depthTestEnabled();
                if (ImGui::Checkbox("Depth test", &depthTestEnabled)) {
                    block.depthTestEnabled(depthTestEnabled);
                    changed = true;
                }

                bool stencilEnabled = block.stencilEnable();
                if (ImGui::Checkbox("Stencil test", &stencilEnabled)) {
                    stencilDirty = true;
                }

                if (stencilDirty) {
                    block.setStencil(stencilEnabled, stencilRef, sFailOp, sZFailOp, sPassOp, sFunc);
                    changed = true;
                }

                if (changed && !readOnly) {
                    material->setRenderStateBlock(block.getHash(), currentStagePass._stage, currentStagePass._passType, currentStagePass._variant);
                    ret = true;
                }
             }

             if (ImGui::CollapsingHeader(Util::StringFormat("Shading Mode [ %s ]", getShadingModeName(material->getShadingMode())).c_str())) {
                 const auto diffuseSetter = [material](const void* data) {
                     material->getColourData().baseColour(*(FColour4*)data);
                 };

                 FColour4 diffuse = material->getColourData().baseColour();
                 if (colourInput4(_parent, " - BaseColour", diffuse, readOnly, diffuseSetter)) {
                     diffuseSetter(diffuse._v);
                     ret = true;
                 }
                 
                 const auto emissiveSetter = [material](const void* data) {
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
                     const auto specularSetter = [material](const void* data) {
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
                 }
             }

             if (readOnly) {
                 ImGui::PopStyleVar();
                 ImGui::PopItemFlag();
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

         const F32 step = field._step;

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
                    case GFX::PushConstantSize::QWORD: ret = inputOrSlider<I64, 1>(_parent, isSlider, "", name, step, ImGuiDataType_S64, field, flags); break;
                    case GFX::PushConstantSize::DWORD: ret = inputOrSlider<I32, 1>(_parent, isSlider, "", name, step, ImGuiDataType_S32, field, flags); break;
                    case GFX::PushConstantSize::WORD:  ret = inputOrSlider<I16, 1>(_parent, isSlider, "", name, step, ImGuiDataType_S16, field, flags); break;
                    case GFX::PushConstantSize::BYTE:  ret = inputOrSlider<I8,  1>(_parent, isSlider, "", name, step, ImGuiDataType_S8,  field, flags); break;
                    default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::UINT: {
                 switch (field._basicTypeSize) {
                     case GFX::PushConstantSize::QWORD: ret = inputOrSlider<U64, 1>(_parent, isSlider, "", name, step, ImGuiDataType_U64, field, flags); break;
                     case GFX::PushConstantSize::DWORD: ret = inputOrSlider<U32, 1>(_parent, isSlider, "", name, step, ImGuiDataType_U32, field, flags); break;
                     case GFX::PushConstantSize::WORD:  ret = inputOrSlider<U16, 1>(_parent, isSlider, "", name, step, ImGuiDataType_U16, field, flags); break;
                     case GFX::PushConstantSize::BYTE:  ret = inputOrSlider<U8,  1>(_parent, isSlider, "", name, step, ImGuiDataType_U8,  field, flags); break;
                    default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::DOUBLE: {
                 ret = inputOrSlider<D64, 1>(_parent, isSlider, "", name, step, ImGuiDataType_Double, field, flags, "%.6f");
             }break;
             case GFX::PushConstantType::FLOAT: {
                 ret = inputOrSlider<F32, 1>(_parent, isSlider, "", name, step, ImGuiDataType_Float, field, flags, "%.3f");
             }break;
             case GFX::PushConstantType::IVEC2: {
                 switch (field._basicTypeSize) {
                     case GFX::PushConstantSize::QWORD: ret = inputOrSlider<vec2<I64>, 2>(_parent, isSlider, "", name, step, ImGuiDataType_S64, field, flags); break;
                     case GFX::PushConstantSize::DWORD: ret = inputOrSlider<vec2<I32>, 2>(_parent, isSlider, "", name, step, ImGuiDataType_S32, field, flags); break;
                     case GFX::PushConstantSize::WORD:  ret = inputOrSlider<vec2<I16>, 2>(_parent, isSlider, "", name, step, ImGuiDataType_S16, field, flags); break;
                     case GFX::PushConstantSize::BYTE:  ret = inputOrSlider<vec2<I8>,  2>(_parent, isSlider, "", name, step, ImGuiDataType_S8,  field, flags); break;
                     default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::IVEC3: {
                 switch (field._basicTypeSize) {
                     case GFX::PushConstantSize::QWORD: ret = inputOrSlider<vec3<I64>, 3>(_parent, isSlider, "", name, step, ImGuiDataType_S64, field, flags); break;
                     case GFX::PushConstantSize::DWORD: ret = inputOrSlider<vec3<I32>, 3>(_parent, isSlider, "", name, step, ImGuiDataType_S32, field, flags); break;
                     case GFX::PushConstantSize::WORD:  ret = inputOrSlider<vec3<I16>, 3>(_parent, isSlider, "", name, step, ImGuiDataType_S16, field, flags); break;
                     case GFX::PushConstantSize::BYTE:  ret = inputOrSlider<vec3<I8>,  3>(_parent, isSlider, "", name, step, ImGuiDataType_S8,  field, flags); break;
                     default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::IVEC4: {
                 switch (field._basicTypeSize) {
                     case GFX::PushConstantSize::QWORD: ret = inputOrSlider<vec4<I64>, 4>(_parent, isSlider, "", name, step, ImGuiDataType_S64, field, flags); break;
                     case GFX::PushConstantSize::DWORD: ret = inputOrSlider<vec4<I32>, 4>(_parent, isSlider, "", name, step, ImGuiDataType_S32, field, flags); break;
                     case GFX::PushConstantSize::WORD:  ret = inputOrSlider<vec4<I16>, 4>(_parent, isSlider, "", name, step, ImGuiDataType_S16, field, flags); break;
                     case GFX::PushConstantSize::BYTE:  ret = inputOrSlider<vec4<I8>,  4>(_parent, isSlider, "", name, step, ImGuiDataType_S8,  field, flags); break;
                     default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::UVEC2: {
                 switch (field._basicTypeSize) {
                     case GFX::PushConstantSize::QWORD: ret = inputOrSlider<vec2<U64>, 2>(_parent, isSlider, "", name, step, ImGuiDataType_U64, field, flags); break;
                     case GFX::PushConstantSize::DWORD: ret = inputOrSlider<vec2<U32>, 2>(_parent, isSlider, "", name, step, ImGuiDataType_U32, field, flags); break;
                     case GFX::PushConstantSize::WORD:  ret = inputOrSlider<vec2<U16>, 2>(_parent, isSlider, "", name, step, ImGuiDataType_U16, field, flags); break;
                     case GFX::PushConstantSize::BYTE:  ret = inputOrSlider<vec2<U8>,  2>(_parent, isSlider, "", name, step, ImGuiDataType_U8,  field, flags); break;
                     default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::UVEC3: {
                 switch (field._basicTypeSize) {
                     case GFX::PushConstantSize::QWORD: ret = inputOrSlider<vec3<U64>, 3>(_parent, isSlider, "", name, step, ImGuiDataType_U64, field, flags); break;
                     case GFX::PushConstantSize::DWORD: ret = inputOrSlider<vec3<U32>, 3>(_parent, isSlider, "", name, step, ImGuiDataType_U32, field, flags); break;
                     case GFX::PushConstantSize::WORD:  ret = inputOrSlider<vec3<U16>, 3>(_parent, isSlider, "", name, step, ImGuiDataType_U16, field, flags); break;
                     case GFX::PushConstantSize::BYTE:  ret = inputOrSlider<vec3<U8>,  3>(_parent, isSlider, "", name, step, ImGuiDataType_U8,  field, flags); break;
                     default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::UVEC4: {
                 switch (field._basicTypeSize) {
                     case GFX::PushConstantSize::QWORD: ret = inputOrSlider<vec4<U64>, 4>(_parent, isSlider, "", name, step, ImGuiDataType_U64, field, flags); break;
                     case GFX::PushConstantSize::DWORD: ret = inputOrSlider<vec4<U32>, 4>(_parent, isSlider, "", name, step, ImGuiDataType_U32, field, flags); break;
                     case GFX::PushConstantSize::WORD:  ret = inputOrSlider<vec4<U16>, 4>(_parent, isSlider, "", name, step, ImGuiDataType_U16, field, flags); break;
                     case GFX::PushConstantSize::BYTE:  ret = inputOrSlider<vec4<U8>,  4>(_parent, isSlider, "", name, step, ImGuiDataType_U8,  field, flags); break;
                     default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::VEC2: {
                 ret = inputOrSlider<vec2<F32>, 2>(_parent, isSlider, "", name, step, ImGuiDataType_Float, field, flags, "%.3f");
             }break;
             case GFX::PushConstantType::VEC3: {
                 ret = inputOrSlider<vec3<F32>, 3>(_parent, isSlider, "", name, step, ImGuiDataType_Float, field, flags, "%.3f");
             }break;
             case GFX::PushConstantType::VEC4: {
                 ret = inputOrSlider<vec4<F32>, 4>(_parent, isSlider, "", name, step, ImGuiDataType_Float, field, flags, "%.3f");
             }break;
             case GFX::PushConstantType::DVEC2: {
                 ret = inputOrSlider<vec2<D64>, 2>(_parent, isSlider, "", name, step, ImGuiDataType_Double, field, flags, "%.6f");
             }break;
             case GFX::PushConstantType::DVEC3: {
                 ret = inputOrSlider<vec3<D64>, 3>(_parent, isSlider, "", name, step, ImGuiDataType_Double, field, flags, "%.6f");
             }break;
             case GFX::PushConstantType::DVEC4: {
                 ret = inputOrSlider<vec4<D64>, 4>(_parent, isSlider, "", name, step, ImGuiDataType_Double, field, flags, "%.6f");
             }break;
             case GFX::PushConstantType::IMAT2: {
                 switch (field._basicTypeSize) {
                     case GFX::PushConstantSize::QWORD: ret = inputMatrix<mat2<I64>, 2>(_parent, "", name, step, ImGuiDataType_S64, field, flags); break;
                     case GFX::PushConstantSize::DWORD: ret = inputMatrix<mat2<I32>, 2>(_parent, "", name, step, ImGuiDataType_S32, field, flags); break;
                     case GFX::PushConstantSize::WORD:  ret = inputMatrix<mat2<I16>, 2>(_parent, "", name, step, ImGuiDataType_S16, field, flags); break;
                     case GFX::PushConstantSize::BYTE:  ret = inputMatrix<mat2<I8>,  2>(_parent, "", name, step, ImGuiDataType_S8,  field, flags); break;
                     default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::IMAT3: {
                 switch (field._basicTypeSize) {
                     case GFX::PushConstantSize::QWORD: ret = inputMatrix<mat3<I64>, 3>(_parent, "", name, step, ImGuiDataType_S64, field, flags); break;
                     case GFX::PushConstantSize::DWORD: ret = inputMatrix<mat3<I32>, 3>(_parent, "", name, step, ImGuiDataType_S32, field, flags); break;
                     case GFX::PushConstantSize::WORD:  ret = inputMatrix<mat3<I16>, 3>(_parent, "", name, step, ImGuiDataType_S16, field, flags); break;
                     case GFX::PushConstantSize::BYTE:  ret = inputMatrix<mat3<I8>,  3>(_parent, "", name, step, ImGuiDataType_S8,  field, flags); break;
                     default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::IMAT4: {
                 switch (field._basicTypeSize) {
                 case GFX::PushConstantSize::QWORD: ret = inputMatrix<mat4<I64>, 4>(_parent, "", name, step, ImGuiDataType_S64, field, flags); break;
                 case GFX::PushConstantSize::DWORD: ret = inputMatrix<mat4<I32>, 4>(_parent, "", name, step, ImGuiDataType_S32, field, flags); break;
                 case GFX::PushConstantSize::WORD:  ret = inputMatrix<mat4<I16>, 4>(_parent, "", name, step, ImGuiDataType_S16, field, flags); break;
                 case GFX::PushConstantSize::BYTE:  ret = inputMatrix<mat4<I8>,  4>(_parent, "", name, step, ImGuiDataType_S8,  field, flags); break;
                     default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::UMAT2: {
                 switch (field._basicTypeSize) {
                     case GFX::PushConstantSize::QWORD: ret = inputMatrix<mat2<U64>, 2>(_parent, "", name, step, ImGuiDataType_U64, field, flags); break;
                     case GFX::PushConstantSize::DWORD: ret = inputMatrix<mat2<U32>, 2>(_parent, "", name, step, ImGuiDataType_U32, field, flags); break;
                     case GFX::PushConstantSize::WORD:  ret = inputMatrix<mat2<U16>, 2>(_parent, "", name, step, ImGuiDataType_U16, field, flags); break;
                     case GFX::PushConstantSize::BYTE:  ret = inputMatrix<mat2<U8>,  2>(_parent, "", name, step, ImGuiDataType_U8,  field, flags); break;
                     default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::UMAT3: {
                 switch (field._basicTypeSize) {
                     case GFX::PushConstantSize::QWORD: ret = inputMatrix<mat3<U64>, 3>(_parent, "", name, step, ImGuiDataType_U64, field, flags); break;
                     case GFX::PushConstantSize::DWORD: ret = inputMatrix<mat3<U32>, 3>(_parent, "", name, step, ImGuiDataType_U32, field, flags); break;
                     case GFX::PushConstantSize::WORD:  ret = inputMatrix<mat3<U16>, 3>(_parent, "", name, step, ImGuiDataType_U16, field, flags); break;
                     case GFX::PushConstantSize::BYTE:  ret = inputMatrix<mat3<U8>,  3>(_parent, "", name, step, ImGuiDataType_U8,  field, flags); break;
                     default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::UMAT4: {
                 switch (field._basicTypeSize) {
                     case GFX::PushConstantSize::QWORD: ret = inputMatrix<mat4<U64>, 4>(_parent, "", name, step, ImGuiDataType_U64, field, flags); break;
                     case GFX::PushConstantSize::DWORD: ret = inputMatrix<mat4<U32>, 4>(_parent, "", name, step, ImGuiDataType_U32, field, flags); break;
                     case GFX::PushConstantSize::WORD:  ret = inputMatrix<mat4<U16>, 4>(_parent, "", name, step, ImGuiDataType_U16, field, flags); break;
                     case GFX::PushConstantSize::BYTE:  ret = inputMatrix<mat4<U8>,  4>(_parent, "", name, step, ImGuiDataType_U8,  field, flags); break;
                     default: DIVIDE_UNEXPECTED_CALL(); break;
                 }
             }break;
             case GFX::PushConstantType::MAT2: {
                 ret = inputMatrix<mat2<F32>, 2>(_parent, "", name, step, ImGuiDataType_Float, field, flags, "%.3f");
             }break;
             case GFX::PushConstantType::MAT3: {
                 ret = inputMatrix<mat3<F32>, 3>(_parent, "", name, step, ImGuiDataType_Float, field, flags, "%.3f");
             }break;
             case GFX::PushConstantType::MAT4: {
                 ret = inputMatrix<mat4<F32>, 4>(_parent, "", name, step, ImGuiDataType_Float, field, flags, "%.3f");
             }break;
             case GFX::PushConstantType::DMAT2: {
                 ret = inputMatrix<mat2<D64>, 2>(_parent, "", name, step, ImGuiDataType_Double, field, flags, "%.6f");
             }break;
             case GFX::PushConstantType::DMAT3: {
                 ret = inputMatrix<mat3<D64>, 3>(_parent, "", name, step, ImGuiDataType_Double, field, flags, "%.6f");
             }break;
             case GFX::PushConstantType::DMAT4: {
                 ret = inputMatrix<mat4<D64>, 4>(_parent, "", name, step, ImGuiDataType_Double, field, flags, "%.6f");
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
         const Selections& nodes = selections();
         if (nodes._selectionCount == 0) {
             return DockedWindow::name();
         }
         if (nodes._selectionCount == 1) {
             return node(nodes._selections[0])->name().c_str();
         }

         return Util::StringFormat("%s, %s, ...", node(nodes._selections[0])->name().c_str(), node(nodes._selections[1])->name().c_str()).c_str();
     }
}; //namespace Divide
