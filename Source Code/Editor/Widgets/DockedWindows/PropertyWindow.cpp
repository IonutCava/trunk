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
#include "ECS/Components/Headers/EnvironmentProbeComponent.h"

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
    namespace {
        // Separate activate is used for stuff that do continous value changes, e.g. colour selectors, but you only want to register the old val once
        template<typename T, bool SeparateActivate, typename Pred>
        void RegisterUndo(Editor& editor, GFX::PushConstantType Type, const T& oldVal, const T& newVal, const char* name, Pred&& dataSetter) {
            static hashMap<U64, UndoEntry<T>> _undoEntries;
            UndoEntry<T>& undo = _undoEntries[_ID(name)];
            if (!SeparateActivate || ImGui::IsItemActivated()) {
                undo._oldVal = oldVal;
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                undo._type = Type;
                undo._name = name;
                undo._newVal = newVal;
                undo._dataSetter = dataSetter;
                editor.registerUndoEntry(undo);
            }
        }

        hashMap<U64, std::tuple<Frustum, FColour3, bool>> g_debugFrustums;

        inline const char* getFormat(ImGuiDataType dataType, const char* input) {
            if (input == nullptr || strlen(input) == 0) {
                const auto unsignedType = [dataType]() {
                    return dataType == ImGuiDataType_U8 || dataType == ImGuiDataType_U16 || dataType == ImGuiDataType_U32 || dataType == ImGuiDataType_U64;
                };

                return dataType == ImGuiDataType_Float ? "%.3f"
                                                   : dataType == ImGuiDataType_Double ? "%.6f"
                                                   : unsignedType() ? "%u" : "%d";
            }
            
            return input;
        }

        template<typename Pred>
        bool colourInput4(Editor& parent, const char* name, FColour4& col, bool readOnly, Pred&& dataSetter) {
            FColour4 oldVal = col;
            const bool ret = ImGui::ColorEdit4(name, col._v, ImGuiColorEditFlags__OptionsDefault);
            if (!readOnly) {
                RegisterUndo<FColour4, true>(parent, GFX::PushConstantType::FCOLOUR4, oldVal, col, name, [dataSetter](const FColour4& oldColour) {
                    dataSetter(oldColour._v);
                });
            }

            return ret;
        }

        template<typename Pred>
        bool colourInput3(Editor& parent, const char* name, FColour3& col, bool readOnly, Pred&& dataSetter) {
            FColour3 oldVal = col;
            const bool ret =  ImGui::ColorEdit3(name, col._v, ImGuiColorEditFlags__OptionsDefault);
            if (!readOnly) {
                RegisterUndo<FColour3, true>(parent, GFX::PushConstantType::FCOLOUR3, oldVal, col, name, [dataSetter](const FColour4& oldColour) {
                    dataSetter(oldColour._v);
                });
            }

            return ret;
        }

        bool colourInput4(Editor& parent, const char* name, EditorComponentField& field) {
            FColour4 val = field.get<FColour4>();
            const auto setter = [&field](const FColour4& col) {
                field.set(col);
            };

            if (colourInput4(parent, name, val, field._readOnly, setter) && val != field.get<FColour4>()) {
                field.set(val);
                return true;
            }

            return false;
        }

        bool colourInput3(Editor& parent, const char* name, EditorComponentField& field) {
            FColour3 val = field.get<FColour3>();
            const auto setter = [&field](const FColour3& col) {
                field.set(col);
            };

            if (colourInput3(parent, name, val, field._readOnly, setter) && val != field.get<FColour3>()) {
                field.set(val);
                return true;
            }

            return false;
        }

        template<typename T, size_t num_comp>
        bool inputOrSlider(Editor& parent, const bool isSlider, const char* label, const char* name, const float stepIn, ImGuiDataType data_type, EditorComponentField& field, ImGuiInputTextFlags flags, const char* format, float power = 1.0f) {
            if (isSlider) {
                return inputOrSlider<T, num_comp, true>(parent, label, name, stepIn, data_type, field, flags, format, power);
            }
            return inputOrSlider<T, num_comp, false>(parent, label, name, stepIn, data_type, field, flags, format, power);
        }

        template<typename T, size_t num_comp, bool IsSlider>
        bool inputOrSlider(Editor& parent, const char* label, const char* name, const float stepIn, ImGuiDataType data_type, EditorComponentField& field, ImGuiInputTextFlags flags, const char* format, float power = 1.0f) {
            if (field._readOnly) {
                PushReadOnly();
            }

            T val = field.get<T>();
            const T cStep = static_cast<T>(stepIn * 100);

            const void* step = IS_ZERO(stepIn) ? nullptr : (void*)&stepIn;
            const void* step_fast = step == nullptr ? nullptr : (void*)&cStep;

            bool ret = false;
            if_constexpr (num_comp == 1) {
                const T min = static_cast<T>(field._range.min);
                T max = static_cast<T>(field._range.max);
                if_constexpr (IsSlider) {
                    ACKNOWLEDGE_UNUSED(step_fast);
                    assert(min <= max);
                    ret = ImGui::SliderScalar(label, data_type, (void*)&val, (void*)&min, (void*)&max, getFormat(data_type, format), power);
                } else {
                    ret = ImGui::InputScalar(label, data_type, (void*)&val, step, step_fast, getFormat(data_type, format), flags);
                    if (max > min) {
                        CLAMP(val, min, max);
                    }
                }
            } else {
                T min = T{ field._range.min };
                T max = T{ field._range.max };

                if_constexpr(IsSlider) {
                    ACKNOWLEDGE_UNUSED(step_fast);
                    assert(min <= max);
                    ret = ImGui::SliderScalarN(label, data_type, (void*)&val, num_comp, (void*)&min, (void*)&max, getFormat(data_type, format), power);
                } else {
                    ret = ImGui::InputScalarN(label, data_type, (void*)&val, num_comp, step, step_fast, getFormat(data_type, format), flags);
                    if (max > min) {
                        for (I32 i = 0; i < to_I32(num_comp); ++i) {
                            val[i] = CLAMPED(val[i], min[i], max[i]);
                        }
                    }
                }
            }
            if (IsSlider || ret) {
                auto tempData = field._data;
                auto tempSetter = field._dataSetter;
                RegisterUndo<T, IsSlider>(parent, field._basicType, field.get<T>(), val, name, [tempData, tempSetter](const T& oldVal) {
                    if (tempSetter != nullptr) {
                        tempSetter(&oldVal);
                    } else {
                        *static_cast<T*>(tempData) = oldVal;
                    }
                });
            }
            if (!field._readOnly && ret && val != field.get<T>()) {
                field.set(val);
            }
                
            if (field._readOnly) {
                PopReadOnly();
            }

            return ret;
        };

        template<typename T, size_t num_rows>
        bool inputMatrix(Editor & parent, const char* label, const char* name, const float stepIn, ImGuiDataType data_type, EditorComponentField& field, ImGuiInputTextFlags flags, const char* format) {
            const T cStep = T(stepIn * 100);

            const void* step = IS_ZERO(stepIn) ? nullptr : (void*)&stepIn;
            const void* step_fast = step == nullptr ? nullptr : (void*)&cStep;

            T mat = field.get<T>();
            bool ret = ImGui::InputScalarN(label, data_type, (void*)mat._vec[0]._v, num_rows, step, step_fast, getFormat(data_type, format), flags) ||
                       ImGui::InputScalarN(label, data_type, (void*)mat._vec[1]._v, num_rows, step, step_fast, getFormat(data_type, format), flags);
            if_constexpr(num_rows > 2) {
                ret = ImGui::InputScalarN(label, data_type, (void*)mat._vec[2]._v, num_rows, step, step_fast, getFormat(data_type, format), flags) || ret;
                if_constexpr(num_rows > 3) {
                    ret = ImGui::InputScalarN(label, data_type, (void*)mat._vec[3]._v, num_rows, step, step_fast, getFormat(data_type, format), flags) || ret;
                }
            }

            if (ret && !field._readOnly && mat != field.get<T>()) {
                auto tempData = field._data;
                auto tempSetter = field._dataSetter;
                RegisterUndo<T, false>(parent, field._basicType, field.get<T>(), mat, name, [tempData, tempSetter](const T& oldVal) {
                    if (tempSetter != nullptr) {
                        tempSetter(&oldVal);
                    } else {
                        *static_cast<T*>(tempData) = oldVal;
                    }
                });
                field.set<>(mat);
            }
            return ret;
        }

        bool isRequiredComponentType(SceneGraphNode* selection, const ComponentType componentType) {
            if (selection != nullptr) {
                return BitCompare(selection->getNode().requiredComponentMask(), to_U32(componentType));
            }

            return false;
        }

        template<typename Pred>
        void ApplyAllButton(I32 &id, Material * material, bool readOnly, Pred&& predicate) {
            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 40);
            ImGui::PushID(4321234 + id++);
            if (readOnly) {
                PushReadOnly();
            }
            if (ImGui::SmallButton("A")) {
                predicate();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Apply to all instances");
            }
            if (readOnly) {
                PopReadOnly();
            }
            ImGui::PopID();
        }

        bool PreviewTextureButton(I32 &id, Texture* tex, bool readOnly) {
            bool ret = false;
            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 15);
            ImGui::PushID(4321234 + id++);
            if (readOnly) {
                PushReadOnly();
            }
            if (ImGui::SmallButton("T")) {
                ret = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(Util::StringFormat("Preview texture : %s", tex->assetName().c_str()).c_str());
            }
            if (readOnly) {
                PopReadOnly();
            }
            ImGui::PopID();
            return ret;
        }
    };

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

        const char* camName = cam->resourceName().c_str();
        const U64 camID = _ID(camName);
        ImGui::PushID(to_I32(camID) * 54321);

        if (ImGui::CollapsingHeader(camName)) {
            {
                vec3<F32> eye = cam->getEye();
                EditorComponentField camField = {};
                camField._name = "Eye";
                camField._basicType = GFX::PushConstantType::VEC3;
                camField._type = EditorComponentFieldType::PUSH_TYPE;
                camField._readOnly = false;
                camField._data = eye._v;
                camField._dataSetter = [cam](const void* eye) {
                    cam->setEye(*static_cast<const vec3<F32>*>(eye));
                };
                sceneChanged = processField(camField) || sceneChanged;
            }
            {
                vec3<F32> euler = cam->getEuler();
                EditorComponentField camField = {};
                camField._name = "Euler";
                camField._basicType = GFX::PushConstantType::VEC3;
                camField._type = EditorComponentFieldType::PUSH_TYPE;
                camField._readOnly = false;
                camField._data = euler._v;
                camField._dataSetter = [cam](const void* euler) {
                    cam->setEuler(*static_cast<const vec3<F32>*>(euler));
                };
                sceneChanged = processField(camField) || sceneChanged;
            }
            {
                F32 aspect = cam->getAspectRatio();
                EditorComponentField camField = {};
                camField._name = "Aspect";
                camField._basicType = GFX::PushConstantType::FLOAT;
                camField._type = EditorComponentFieldType::PUSH_TYPE;
                camField._readOnly = false;
                camField._data = &aspect;
                camField._dataSetter = [cam](const void* aspect) {
                    cam->setAspectRatio(*static_cast<const F32*>(aspect));
                };
                sceneChanged = processField(camField) || sceneChanged;
            }
            {
                F32 horizontalFoV = cam->getHorizontalFoV();
                EditorComponentField camField = {};
                camField._name = "FoV (horizontal)";
                camField._basicType = GFX::PushConstantType::FLOAT;
                camField._type = EditorComponentFieldType::PUSH_TYPE;
                camField._readOnly = false;
                camField._data = &horizontalFoV;
                camField._dataSetter = [cam](const void* fov) {
                    cam->setHorizontalFoV(*static_cast<const F32*>(fov));
                };
                sceneChanged = processField(camField) || sceneChanged;
            }
            {
                vec2<F32> zPlanes = cam->getZPlanes();
                EditorComponentField camField = {};
                camField._name = "zPlanes";
                camField._basicType = GFX::PushConstantType::VEC2;
                camField._type = EditorComponentFieldType::PUSH_TYPE;
                camField._readOnly = false;
                camField._data = zPlanes._v;
                camField._dataSetter = [cam](const void* planes) {
                    vec2<F32> zPlanes = *static_cast<const vec2<F32>*>(planes);
                    if (cam->isOrthoProjected()) {
                        cam->setProjection(cam->orthoRect(), zPlanes);
                    } else {
                        cam->setProjection(cam->getAspectRatio(), cam->getVerticalFoV(), zPlanes);
                    }
                };
                sceneChanged = processField(camField) || sceneChanged;
            }
            if (cam->isOrthoProjected()) {
                vec4<F32> orthoRect = cam->orthoRect();
                EditorComponentField camField = {};
                camField._name = "Ortho";
                camField._basicType = GFX::PushConstantType::VEC4;
                camField._type = EditorComponentFieldType::PUSH_TYPE;
                camField._readOnly = false;
                camField._data = orthoRect._v;
                camField._dataSetter = [cam](const void* rect) {
                    cam->setProjection(*static_cast<const vec4<F32>*>(rect), cam->getZPlanes());
                };
                sceneChanged = processField(camField) || sceneChanged;
            }
            {
                ImGui::Text("View Matrix");
                ImGui::Spacing();
                mat4<F32> viewMatrix = cam->getViewMatrix();
                EditorComponentField worldMatrixField = {};
                worldMatrixField._name = "View Matrix";
                worldMatrixField._basicType = GFX::PushConstantType::MAT4;
                worldMatrixField._type = EditorComponentFieldType::PUSH_TYPE;
                worldMatrixField._readOnly = true;
                worldMatrixField._data = &viewMatrix;
                processBasicField(worldMatrixField);
            }
            {
                ImGui::Text("Projection Matrix");
                ImGui::Spacing();
                mat4<F32> projMatrix = cam->getProjectionMatrix();
                EditorComponentField projMatrixField;
                projMatrixField._basicType = GFX::PushConstantType::MAT4;
                projMatrixField._type = EditorComponentFieldType::PUSH_TYPE;
                projMatrixField._readOnly = true;
                projMatrixField._name = "Projection Matrix";
                projMatrixField._data = &projMatrix;
                processBasicField(projMatrixField);
            }
            {
                ImGui::Separator();
                bool drawFustrum = g_debugFrustums.find(camID) != eastl::cend(g_debugFrustums);
                ImGui::PushID(to_I32(camID) * 123456);
                if (ImGui::Checkbox("Draw debug frustum", &drawFustrum)) {
                    if (drawFustrum) {
                        g_debugFrustums[camID] = { cam->getFrustum(), DefaultColours::GREEN, true };
                    } else {
                        g_debugFrustums.erase(camID);
                    }
                }
                if (drawFustrum) {
                    auto& [frust, colour, realtime] = g_debugFrustums[camID];
                    ImGui::Checkbox("Update realtime", &realtime);
                    const bool update = realtime || ImGui::Button("Update frustum");
                    if (update) {
                        frust = cam->getFrustum();
                    }
                    ImGui::PushID(cam->resourceName().c_str());
                    ImGui::ColorEdit3("Frust Colour", colour._v, ImGuiColorEditFlags__OptionsDefault);
                    ImGui::PopID();
                }
                ImGui::PopID();
            }
        }
        ImGui::PopID();

        return sceneChanged;
    }

    void PropertyWindow::backgroundUpdateInternal() {
        for (const auto& it : g_debugFrustums) {
            const auto& [frust, colour, realtime] = it.second;
            _context.gfx().debugDrawFrustum(frust, colour);
        }
    }

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
                ImGui::PushID(sgnNode->name().c_str());

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

                    vectorEASTLFast<EditorComponent*>& editorComp = Attorney::SceneGraphNodeEditor::editorComponents(sgnNode);
                    for (EditorComponent* comp : editorComp) {
                        if (comp->fields().empty()) {
                            PushReadOnly();
                            ImGui::CollapsingHeader(comp->name().c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet);
                            PopReadOnly();
                            continue;
                        }

                        if (ImGui::CollapsingHeader(comp->name().c_str(), ImGuiTreeNodeFlags_OpenOnArrow)) {
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

                            vectorEASTL<EditorComponentField>& fields = Attorney::EditorComponentEditor::fields(*comp);
                            for (EditorComponentField& field : fields) {
                                if (processField(field) && !field._readOnly) {
                                    Attorney::EditorComponentEditor::onChanged(*comp, field);
                                    sceneChanged = true;
                                }
                                ImGui::Spacing();
                            }
                            const U32 componentMask = sgnNode->componentMask();
                            // Environment probes
                            if (BitCompare(componentMask, ComponentType::ENVIRONMENT_PROBE)) {
                                EnvironmentProbeComponent* probe = sgnNode->get<EnvironmentProbeComponent>();
                                if (probe != nullptr) {
                                    const auto& cameras = probe->probeCameras();

                                    for (U8 face = 0; face < 6; ++face) {
                                        Camera* probeCameras = cameras[face];
                                        if (drawCamera(probeCameras)) {
                                            sceneChanged = true;
                                        }
                                    }
                                }
                            }
                            // Show light/shadow specific options (if any)
                            Light* light = nullptr;
                            if (BitCompare(componentMask, ComponentType::SPOT_LIGHT)) {
                                light = sgnNode->get<SpotLightComponent>();
                            } else if (BitCompare(componentMask, ComponentType::POINT_LIGHT)) {
                                light = sgnNode->get<PointLightComponent>();
                            } else if (BitCompare(componentMask, ComponentType::DIRECTIONAL_LIGHT)) {
                                light = sgnNode->get<DirectionalLightComponent>();
                            }
                            if (light != nullptr) {
                                if (light->castsShadows()) {
                                    if (ImGui::CollapsingHeader("Light Shadow Settings", ImGuiTreeNodeFlags_OpenOnArrow)) {
                                        ImGui::Text("Shadow Offset: %d", to_U32(light->getShadowOffset()));

                                        switch (light->getLightType()) {
                                            case LightType::POINT: {
                                                PointLightComponent* pointLight = static_cast<PointLightComponent*>(light);
                                                ACKNOWLEDGE_UNUSED(pointLight);

                                                for (U8 face = 0; face < 6; ++face) {
                                                    Camera* shadowCamera = ShadowMap::shadowCameras(ShadowType::CUBEMAP)[face];
                                                    if (drawCamera(shadowCamera)) {
                                                        sceneChanged = true;
                                                    }
                                                }
                                                
                                            } break;

                                            case LightType::SPOT: {
                                                SpotLightComponent* spotLight = static_cast<SpotLightComponent*>(light);
                                                ACKNOWLEDGE_UNUSED(spotLight);

                                                Camera* shadowCamera = ShadowMap::shadowCameras(ShadowType::SINGLE).front();
                                                if (drawCamera(shadowCamera)) {
                                                    sceneChanged = true;
                                                }
                                            } break;

                                            case LightType::DIRECTIONAL: {
                                                DirectionalLightComponent* dirLight = static_cast<DirectionalLightComponent*>(light);
                                                for (U8 split = 0; split < dirLight->csmSplitCount(); ++split) {
                                                    Camera* shadowCamera = ShadowMap::shadowCameras(ShadowType::LAYERED)[split];
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
                                    SceneState* activeSceneState = sceneManager->getActiveScene().state();
                                    {
                                        F32 bleedBias = activeSceneState->lightBleedBias();
                                        EditorComponentField tempField = {};
                                        tempField._name = "Light bleed bias";
                                        tempField._basicType = GFX::PushConstantType::FLOAT;
                                        tempField._type = EditorComponentFieldType::PUSH_TYPE;
                                        tempField._readOnly = false;
                                        tempField._data = &bleedBias;
                                        tempField._format = "%.6f";
                                        tempField._range = { 0.0f, 1.0f };
                                        tempField._dataSetter = [&activeSceneState](const void* bias) {
                                            activeSceneState->lightBleedBias(*static_cast<const F32*>(bias));
                                        };
                                        sceneChanged = processField(tempField) || sceneChanged;
                                    }
                                    {
                                        F32 shadowVariance = activeSceneState->minShadowVariance();
                                        EditorComponentField tempField = {};
                                        tempField._name = "Min shadow variance";
                                        tempField._basicType = GFX::PushConstantType::FLOAT;
                                        tempField._type = EditorComponentFieldType::PUSH_TYPE;
                                        tempField._readOnly = false;
                                        tempField._data = &shadowVariance;
                                        tempField._range = { 0.00001f, 0.99999f };
                                        tempField._format = "%.6f";
                                        tempField._dataSetter = [&activeSceneState](const void* variance) {
                                            activeSceneState->minShadowVariance(*static_cast<const F32*>(variance));
                                        };
                                        sceneChanged = processField(tempField) || sceneChanged;
                                    }
                                    constexpr U16 min = 1u, max = 1000u;
                                    {
                                        U16 shadowFadeDistance = activeSceneState->shadowFadeDistance();
                                        EditorComponentField tempField = {};
                                        tempField._name = "Shadow fade distance";
                                        tempField._basicType = GFX::PushConstantType::UINT;
                                        tempField._basicTypeSize = GFX::PushConstantSize::WORD;
                                        tempField._type = EditorComponentFieldType::PUSH_TYPE;
                                        tempField._readOnly = false;
                                        tempField._data = &shadowFadeDistance;
                                        tempField._range = { min, max };
                                        tempField._dataSetter = [&activeSceneState](const void* bias) {
                                            activeSceneState->shadowFadeDistance(*static_cast<const U16*>(bias));
                                        };
                                        sceneChanged = processField(tempField) || sceneChanged;
                                    }
                                    {
                                        U16 shadowMaxDistance = activeSceneState->shadowDistance();
                                        EditorComponentField tempField = {};
                                        tempField._name = "Shadow max distance";
                                        tempField._basicType = GFX::PushConstantType::UINT;
                                        tempField._basicTypeSize = GFX::PushConstantSize::WORD;
                                        tempField._type = EditorComponentFieldType::PUSH_TYPE;
                                        tempField._readOnly = false;
                                        tempField._data = &shadowMaxDistance;
                                        tempField._range = { min, max };
                                        tempField._dataSetter = [&activeSceneState](const void* bias) {
                                            activeSceneState->shadowDistance(*static_cast<const U16*>(bias));
                                        };
                                        sceneChanged = processField(tempField) || sceneChanged;
                                    }
                                }
                            }
                        }
                    }
                }
                ImGui::PopID();
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
                    const SceneGraphNode* sgn = node(crtSelections._selections[i]);
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
            ImGui::Text("Please select a scene node \n to inspect its properties");
            ImGui::Separator();
        }

        if (_previewTexture != nullptr) {
            if (Attorney::EditorGeneralWidget::modalTextureView(_context.editor(), Util::StringFormat("Image Preview: %s", _previewTexture->resourceName().c_str()).c_str(), _previewTexture, vec2<F32>(512, 512), true, false)) {
                _previewTexture = nullptr;
            }
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

        return activeScene.sceneGraph()->findNode(guid);
    }

    bool PropertyWindow::processField(EditorComponentField& field) {
        const ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank | (field._readOnly ? ImGuiInputTextFlags_ReadOnly : 0);

        ImGui::Text(field._name.c_str());

        if (field._readOnly) {
            PushReadOnly();
        }

        bool ret = false;
        switch (field._type) {
            case EditorComponentFieldType::SEPARATOR: {
                ImGui::Separator();
            }break;
            case EditorComponentFieldType::BUTTON: {
                if (field._range.y - field._range.x > 1.0f) {
                    ret = ImGui::Button(field._name.c_str(), ImVec2(field._range.x, field._range.y));
                } else {
                    ret = ImGui::Button(field._name.c_str());
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
                {
                    EditorComponentField bbField = {};
                    bbField._name = "- Min ";
                    bbField._basicType = GFX::PushConstantType::VEC3;
                    bbField._type = EditorComponentFieldType::PUSH_TYPE;
                    bbField._readOnly = field._readOnly;
                    bbField._data = bbMin;
                    bbField._dataSetter = [&field](const void* min) {
                        BoundingBox bb = {};
                        field.get<BoundingBox>(bb);
                        bb.setMin(*static_cast<const vec3<F32>*>(min));
                        field.set<BoundingBox>(bb);
                    };
                    ret = processField(bbField) || ret;
                }
                {
                    EditorComponentField bbField = {};
                    bbField._name = "- Max ";
                    bbField._basicType = GFX::PushConstantType::VEC3;
                    bbField._type = EditorComponentFieldType::PUSH_TYPE;
                    bbField._readOnly = field._readOnly;
                    bbField._data = bbMax;
                    bbField._dataSetter = [&field](const void* max) {
                        BoundingBox bb = {};
                        field.get<BoundingBox>(bb);
                        bb.setMax(*static_cast<const vec3<F32>*>(max));
                        field.set<BoundingBox>(bb);
                    };
                    ret = processField(bbField) || ret;
                }
            }break;
            case EditorComponentFieldType::BOUNDING_SPHERE: {
                BoundingSphere bs = {};
                field.get<BoundingSphere>(bs);
                F32* center = Attorney::BoundingSphereEditor::center(bs);
                F32& radius = Attorney::BoundingSphereEditor::radius(bs);
                {
                    EditorComponentField bbField = {};
                    bbField._name = "- Center ";
                    bbField._basicType = GFX::PushConstantType::VEC3;
                    bbField._type = EditorComponentFieldType::PUSH_TYPE;
                    bbField._readOnly = field._readOnly;
                    bbField._data = center;
                    bbField._dataSetter = [&field](const void* center) {
                        BoundingSphere bs = {};
                        field.get<BoundingSphere>(bs);
                        bs.setCenter(*static_cast<const vec3<F32>*>(center));
                        field.set<BoundingSphere>(bs);
                    };
                    ret = processField(bbField) || ret;
                }
                {
                    EditorComponentField bbField = {};
                    bbField._name = "- Radius ";
                    bbField._basicType = GFX::PushConstantType::FLOAT;
                    bbField._type = EditorComponentFieldType::PUSH_TYPE;
                    bbField._readOnly = field._readOnly;
                    bbField._data = &radius;
                    bbField._dataSetter = [&field](const void* radius) {
                        BoundingSphere bs = {};
                        field.get<BoundingSphere>(bs);
                        bs.setRadius(*static_cast<const F32*>(radius));
                        field.set<BoundingSphere>(bs);
                    };
                    ret = processField(bbField) || ret;
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
        if (field._readOnly) {
            PopReadOnly();
        }
        if (!field._tooltip.empty() && ImGui::IsItemHovered()) {
            ImGui::SetTooltip(field._tooltip.c_str());
        }
        return ret;
    }

    bool PropertyWindow::processTransform(TransformComponent* transform, bool readOnly) {
        if (transform == nullptr) {
            return false;
        }

        bool ret = false;
        const ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank | (readOnly ? ImGuiInputTextFlags_ReadOnly : 0);

        const TransformValues transformValues = transform->getValues();

        vec3<F32> pos = transformValues._translation;
        if (ImGui::InputFloat3(" - Position ", pos, "%.3f", flags)) {
            if (!readOnly) {
                ret = true;
                RegisterUndo<vec3<F32>, false>(_parent, GFX::PushConstantType::VEC3, transformValues._translation, pos, "Transform position", [transform](const vec3<F32>& val) {
                    transform->setPosition(val);
                });
                transform->setPosition(pos);
            }
        }

        vec3<F32> rot; transformValues._orientation.getEuler(rot); rot = Angle::to_DEGREES(rot);
        const vec3<F32> oldRot = rot;
        if (ImGui::InputFloat3(" - Rotation ", rot, "%.3f", flags)) {
            if (!readOnly) {
                ret = true;
                RegisterUndo<vec3<F32>, false>(_parent, GFX::PushConstantType::VEC3, oldRot, rot, "Transform rotation", [transform](const vec3<F32>& val) {
                    transform->setRotationEuler(val);
                });
                transform->setRotationEuler(rot);
            }
        }

        vec3<F32> scale = transformValues._scale;
        if (ImGui::InputFloat3(" - Scale ", scale, "%.3f", flags)) {
            if (!readOnly) {
                ret = true;
                // Scale is tricky as it may invalidate everything if it's set wrong!
                for (U8 i = 0; i < 3; ++i) {
                    scale[i] = std::max(EPSILON_F32, scale[i]);
                }
                RegisterUndo<vec3<F32>, false>(_parent, GFX::PushConstantType::VEC3, transformValues._scale, scale, "Transform scale", [transform](const vec3<F32>& val) {
                    transform->setScale(val);
                });
                transform->setScale(scale);
            }
        }

        return ret;
    }

    bool PropertyWindow::processMaterial(Material* material, bool readOnly) {
        if (material == nullptr) {
            return false;
        }

        if (readOnly) {
            PushReadOnly();
        }

        bool ret = false;
        static RenderStagePass currentStagePass = {};
        {
            I32 crtStage = to_I32(currentStagePass._stage);
            const char* crtStageName = TypeUtil::RenderStageToString(currentStagePass._stage);
            if (ImGui::SliderInt("Stage", &crtStage, 0, to_base(RenderStage::COUNT), crtStageName)) {
                currentStagePass._stage = static_cast<RenderStage>(crtStage);
            }

            I32 crtPass = to_I32(currentStagePass._passType);
            const char* crtPassName = TypeUtil::RenderPassTypeToString(currentStagePass._passType);
            if (ImGui::SliderInt("Pass", &crtPass, 0, to_base(RenderPassType::COUNT), crtPassName)) {
                currentStagePass._passType = static_cast<RenderPassType>(crtPass);
            }

            constexpr U8 min = 0u, max = Material::g_maxVariantsPerPass;
            ImGui::SliderScalar("Variant", ImGuiDataType_U8, &currentStagePass._variant, &min, &max);
            ImGui::InputScalar("Index", ImGuiDataType_U16, &currentStagePass._index);
            ImGui::InputScalar("Pass", ImGuiDataType_U16, &currentStagePass._pass);
        }

        size_t stateHash = 0;
        stringImpl shaderName = "None";
        ShaderProgram* program = nullptr;
        if (currentStagePass._stage != RenderStage::COUNT && currentStagePass._passType != RenderPassType::COUNT) {
            const I64 shaderGUID = material->computeAndGetProgramGUID(currentStagePass);
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
                                                                module._sourceFile.data(),
                                                                module._variant.empty() ? "-" : module._variant.c_str()).c_str())) 
                    {
                        ImGui::Text("Defines: ");
                        ImGui::Separator();
                        for (const auto& define : module._defines) {
                        ImGui::Text(define.first.c_str());
                        }
                        if (ImGui::Button("Open Source File")) {
                            const stringImpl& textEditor = Attorney::EditorGeneralWidget::externalTextEditorPath(_context.editor());
                            if (textEditor.empty()) {
                                Attorney::EditorGeneralWidget::showStatusMessage(_context.editor(), "ERROR: No text editor specified!", Time::SecondsToMilliseconds<F32>(3));
                            } else {
                                if (!openFile(textEditor.c_str(), (program->assetLocation() + Paths::Shaders::GLSL::g_parentShaderLoc.c_str()).c_str(), module._sourceFile.data())) {
                                    Attorney::EditorGeneralWidget::showStatusMessage(_context.editor(), "ERROR: Couldn't open specified source file!", Time::SecondsToMilliseconds<F32>(3));
                                }
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
                P32 colourWrite = block.colourWrite();
                const char* names[] = { "R", "G", "B", "A" };

                for (U8 i = 0; i < 4; ++i) {
                    if (i > 0) {
                        ImGui::SameLine();
                    }

                    bool val = colourWrite.b[i] == 1;
                    if (ImGui::Checkbox(names[i], &val)) {
                        RegisterUndo<bool, false>(_parent, GFX::PushConstantType::BOOL, !val, val, "Colour Mask", [stateHash, material, i](const bool& oldVal) {
                            RenderStateBlock block = RenderStateBlock::get(stateHash);
                            const P32 colourWrite = block.colourWrite();
                            block.setColourWrites(
                                i == 0 ? oldVal : colourWrite.b[0],
                                i == 1 ? oldVal : colourWrite.b[1],
                                i == 2 ? oldVal : colourWrite.b[2],
                                i == 3 ? oldVal : colourWrite.b[3]
                            );
                            material->setRenderStateBlock(block.getHash(), currentStagePass._stage, currentStagePass._passType, currentStagePass._variant);
                        });
                        colourWrite.b[i] = val ? 1 : 0;
                        block.setColourWrites(colourWrite.b[0] == 1, colourWrite.b[1] == 1, colourWrite.b[2] == 1, colourWrite.b[3] == 1);
                        changed = true;
                    }
                }
            }

            F32 zBias = block.zBias();
            F32 zUnits = block.zUnits();
            {
                EditorComponentField tempField = {};
                tempField._name = "ZBias";
                tempField._basicType = GFX::PushConstantType::FLOAT;
                tempField._type = EditorComponentFieldType::PUSH_TYPE;
                tempField._readOnly = readOnly;
                tempField._data = &zBias;
                tempField._range = { 0.0f, 1000.0f };
                RenderStagePass tempPass = currentStagePass;
                tempField._dataSetter = [material, stateHash, tempPass, zUnits](const void* data) {
                    RenderStateBlock block = RenderStateBlock::get(stateHash);
                    block.setZBias(*(static_cast<const F32*>(data)), zUnits);
                    material->setRenderStateBlock(block.getHash(), tempPass._stage, tempPass._passType, tempPass._variant);
                };
                changed = processField(tempField) || changed;
            }
            {
                EditorComponentField tempField = {};
                tempField._name = "ZUnits";
                tempField._basicType = GFX::PushConstantType::FLOAT;
                tempField._type = EditorComponentFieldType::PUSH_TYPE;
                tempField._readOnly = readOnly;
                tempField._data = &zUnits;
                tempField._range = { 0.0f, 65536.0f };
                RenderStagePass tempPass = currentStagePass;
                tempField._dataSetter = [material, stateHash, tempPass, zBias](const void* data) {
                    RenderStateBlock block = RenderStateBlock::get(stateHash);
                    block.setZBias(zBias, *(static_cast<const F32*>(data)));
                    material->setRenderStateBlock(block.getHash(), tempPass._stage, tempPass._passType, tempPass._variant);
                };
                changed = processField(tempField) || changed;
            }

            ImGui::Text("Tessellation control points: %d", block.tessControlPoints());
            
            {
                CullMode cMode = block.cullMode();

                static UndoEntry<I32> cullUndo = {};
                const char* crtMode = TypeUtil::CullModeToString(cMode);
                if (ImGui::BeginCombo("Cull Mode", crtMode, ImGuiComboFlags_PopupAlignLeft)) {
                    for (U8 n = 0; n < to_U8(CullMode::COUNT); ++n) {
                        const CullMode mode = static_cast<CullMode>(n);
                        const bool isSelected = cMode == mode;

                        if (ImGui::Selectable(TypeUtil::CullModeToString(mode), isSelected)) {
                            cullUndo._type = GFX::PushConstantType::INT;
                            cullUndo._name = "Cull Mode";
                            cullUndo._oldVal = to_I32(cMode);
                            cullUndo._newVal = to_I32(mode);
                            RenderStagePass tempPass = currentStagePass;
                            cullUndo._dataSetter = [material, stateHash, tempPass](const I32& data) {
                                RenderStateBlock block = RenderStateBlock::get(stateHash);
                                block.setCullMode(static_cast<CullMode>(data));
                                material->setRenderStateBlock(block.getHash(), tempPass._stage, tempPass._passType, tempPass._variant);
                            };
                            _context.editor().registerUndoEntry(cullUndo);

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
                static UndoEntry<I32> fillUndo = {};
                FillMode fMode = block.fillMode();
                const char* crtMode = TypeUtil::FillModeToString(fMode);
                if (ImGui::BeginCombo("Fill Mode", crtMode, ImGuiComboFlags_PopupAlignLeft)) {
                    for (U8 n = 0; n < to_U8(FillMode::COUNT); ++n) {
                        const FillMode mode = static_cast<FillMode>(n);
                        const bool isSelected = fMode == mode;

                        if (ImGui::Selectable(TypeUtil::FillModeToString(mode), isSelected)) {
                            fillUndo._type = GFX::PushConstantType::INT;
                            fillUndo._name = "Fill Mode";
                            fillUndo._oldVal = to_I32(fMode);
                            fillUndo._newVal = to_I32(mode);
                            RenderStagePass tempPass = currentStagePass;
                            fillUndo._dataSetter = [material, stateHash, tempPass](const I32& data) {
                                RenderStateBlock block = RenderStateBlock::get(stateHash);
                                block.setFillMode(static_cast<FillMode>(data));
                                material->setRenderStateBlock(block.getHash(), tempPass._stage, tempPass._passType, tempPass._variant);
                            };
                            _context.editor().registerUndoEntry(fillUndo);

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
                RenderStagePass tempPass = currentStagePass;
                RegisterUndo<U32, false>(_parent, GFX::PushConstantType::UINT, block.stencilMask(), stencilReadMask, "Stencil mask", [material, stateHash, stencilWriteMask, tempPass](const U32& oldVal) {
                    RenderStateBlock block = RenderStateBlock::get(stateHash);
                    block.setStencilReadWriteMask(oldVal, stencilWriteMask);
                    material->setRenderStateBlock(block.getHash(), tempPass._stage, tempPass._passType, tempPass._variant);
                });

                block.setStencilReadWriteMask(stencilReadMask, stencilWriteMask);
                changed = true;
            }

            if (ImGui::InputScalar("Stencil write mask", ImGuiDataType_U32, &stencilWriteMask, NULL, NULL, "%08X", ImGuiInputTextFlags_CharsHexadecimal)) {
                RenderStagePass tempPass = currentStagePass;
                RegisterUndo<U32, false>(_parent, GFX::PushConstantType::UINT, block.stencilWriteMask(), stencilWriteMask, "Stencil write mask", [material, stateHash, stencilReadMask, tempPass](const U32& oldVal) {
                    RenderStateBlock block = RenderStateBlock::get(stateHash);
                    block.setStencilReadWriteMask(stencilReadMask, oldVal);
                    material->setRenderStateBlock(block.getHash(), tempPass._stage, tempPass._passType, tempPass._variant);
                });

                block.setStencilReadWriteMask(stencilReadMask, stencilWriteMask);
                changed = true;
            }

            bool stencilDirty = false;
            U32 stencilRef = block.stencilRef();
            bool stencilEnabled = block.stencilEnable();
            StencilOperation sFailOp = block.stencilFailOp();
            StencilOperation sZFailOp = block.stencilZFailOp();
            StencilOperation sPassOp = block.stencilPassOp();
            ComparisonFunction sFunc = block.stencilFunc();

            if (ImGui::InputScalar("Stencil reference mask", ImGuiDataType_U32, &stencilRef, NULL, NULL, "%08X", ImGuiInputTextFlags_CharsHexadecimal)) {
                RenderStagePass tempPass = currentStagePass;
                RegisterUndo<U32, false>(_parent, GFX::PushConstantType::UINT, block.stencilRef(), stencilRef, "Stencil reference mask", [material, stateHash, tempPass, stencilEnabled, stencilReadMask, sFailOp, sZFailOp, sPassOp, sFunc](const U32& oldVal) {
                    RenderStateBlock block = RenderStateBlock::get(stateHash);
                    block.setStencil(stencilEnabled, oldVal, sFailOp, sZFailOp, sPassOp, sFunc);
                    material->setRenderStateBlock(block.getHash(), tempPass._stage, tempPass._passType, tempPass._variant);
                });
                stencilDirty = true;
            }

            {
                static UndoEntry<I32> stencilUndo = {};
                ComparisonFunction zFunc = block.zFunc();
                const char* crtMode = TypeUtil::ComparisonFunctionToString(zFunc);
                if (ImGui::BeginCombo("Depth function", crtMode, ImGuiComboFlags_PopupAlignLeft)) {
                    for (U8 n = 0; n < to_U8(ComparisonFunction::COUNT); ++n) {
                        const ComparisonFunction func = static_cast<ComparisonFunction>(n);
                        const bool isSelected = zFunc == func;

                        if (ImGui::Selectable(TypeUtil::ComparisonFunctionToString(func), isSelected)) {
                            stencilUndo._type = GFX::PushConstantType::INT;
                            stencilUndo._name = "Depth function";
                            stencilUndo._oldVal = to_I32(zFunc);
                            stencilUndo._newVal = to_I32(func);
                            RenderStagePass tempPass = currentStagePass;
                            stencilUndo._dataSetter = [material, stateHash, tempPass](const I32& data) {
                                RenderStateBlock block = RenderStateBlock::get(stateHash);
                                block.setZFunc(static_cast<ComparisonFunction>(data));
                                material->setRenderStateBlock(block.getHash(), tempPass._stage, tempPass._passType, tempPass._variant);
                            };
                            _context.editor().registerUndoEntry(stencilUndo);

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
            {
                static UndoEntry<I32> stencilUndo = {};
                const char* crtMode = TypeUtil::StencilOperationToString(sFailOp);
                if (ImGui::BeginCombo("Stencil fail op", crtMode, ImGuiComboFlags_PopupAlignLeft)) {
                    for (U8 n = 0; n < to_U8(StencilOperation::COUNT); ++n) {
                        const StencilOperation op = static_cast<StencilOperation>(n);
                        const bool isSelected = sFailOp == op;

                        if (ImGui::Selectable(TypeUtil::StencilOperationToString(op), isSelected)) {
                            stencilUndo._type = GFX::PushConstantType::INT;
                            stencilUndo._name = "Stencil fail op";
                            stencilUndo._oldVal = to_I32(sFailOp);
                            stencilUndo._newVal = to_I32(op);
                            RenderStagePass tempPass = currentStagePass;
                            stencilUndo._dataSetter = [material, stateHash, tempPass, stencilEnabled, stencilRef, sZFailOp, sPassOp, sFunc](const I32& data) {
                                RenderStateBlock block = RenderStateBlock::get(stateHash);
                                block.setStencil(stencilEnabled, stencilRef, static_cast<StencilOperation>(data), sZFailOp, sPassOp, sFunc);
                                material->setRenderStateBlock(block.getHash(), tempPass._stage, tempPass._passType, tempPass._variant);
                            };
                            _context.editor().registerUndoEntry(stencilUndo);

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
            {
                static UndoEntry<I32> stencilUndo = {};
                const char* crtMode = TypeUtil::StencilOperationToString(sZFailOp);
                if (ImGui::BeginCombo("Stencil depth fail op", crtMode, ImGuiComboFlags_PopupAlignLeft)) {
                    for (U8 n = 0; n < to_U8(StencilOperation::COUNT); ++n) {
                        const StencilOperation op = static_cast<StencilOperation>(n);
                        const bool isSelected = sZFailOp == op;

                        if (ImGui::Selectable(TypeUtil::StencilOperationToString(op), isSelected)) {
                            stencilUndo._type = GFX::PushConstantType::INT;
                            stencilUndo._name = "Stencil depth fail op";
                            stencilUndo._oldVal = to_I32(sZFailOp);
                            stencilUndo._newVal = to_I32(op);
                            RenderStagePass tempPass = currentStagePass;
                            stencilUndo._dataSetter = [material, stateHash, tempPass, stencilEnabled, stencilRef, sFailOp, sPassOp, sFunc](const I32& data) {
                                RenderStateBlock block = RenderStateBlock::get(stateHash);
                                block.setStencil(stencilEnabled, stencilRef, sFailOp, static_cast<StencilOperation>(data), sPassOp, sFunc);
                                material->setRenderStateBlock(block.getHash(), tempPass._stage, tempPass._passType, tempPass._variant);
                            };
                            _context.editor().registerUndoEntry(stencilUndo);

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
            {
                static UndoEntry<I32> stencilUndo = {};
                const char* crtMode = TypeUtil::StencilOperationToString(sPassOp);
                if (ImGui::BeginCombo("Stencil pass op", crtMode, ImGuiComboFlags_PopupAlignLeft)) {
                    for (U8 n = 0; n < to_U8(StencilOperation::COUNT); ++n) {
                        const StencilOperation op = static_cast<StencilOperation>(n);
                        const bool isSelected = sPassOp == op;

                        if (ImGui::Selectable(TypeUtil::StencilOperationToString(op), isSelected)) {

                            stencilUndo._type = GFX::PushConstantType::INT;
                            stencilUndo._name = "Stencil pass op";
                            stencilUndo._oldVal = to_I32(sPassOp);
                            stencilUndo._newVal = to_I32(op);
                            RenderStagePass tempPass = currentStagePass;
                            stencilUndo._dataSetter = [material, stateHash, tempPass, stencilEnabled, stencilRef, sFailOp, sZFailOp, sFunc](const I32& data) {
                                RenderStateBlock block = RenderStateBlock::get(stateHash);
                                block.setStencil(stencilEnabled, stencilRef, sFailOp, sZFailOp, static_cast<StencilOperation>(data), sFunc);
                                material->setRenderStateBlock(block.getHash(), tempPass._stage, tempPass._passType, tempPass._variant);
                            };
                            _context.editor().registerUndoEntry(stencilUndo);

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
            {
                static UndoEntry<I32> stencilUndo = {};
                const char* crtMode = TypeUtil::ComparisonFunctionToString(sFunc);
                if (ImGui::BeginCombo("Stencil function", crtMode, ImGuiComboFlags_PopupAlignLeft)) {
                    for (U8 n = 0; n < to_U8(ComparisonFunction::COUNT); ++n) {
                        const ComparisonFunction mode = static_cast<ComparisonFunction>(n);
                        const bool isSelected = sFunc == mode;

                        if (ImGui::Selectable(TypeUtil::ComparisonFunctionToString(mode), isSelected)) {

                            stencilUndo._type = GFX::PushConstantType::INT;
                            stencilUndo._name = "Stencil function";
                            stencilUndo._oldVal = to_I32(sFunc);
                            stencilUndo._newVal = to_I32(mode);
                            RenderStagePass tempPass = currentStagePass;
                            stencilUndo._dataSetter = [material, stateHash, tempPass, stencilEnabled, stencilRef, sFailOp, sZFailOp, sPassOp](const I32& data) {
                                RenderStateBlock block = RenderStateBlock::get(stateHash);
                                block.setStencil(stencilEnabled, stencilRef, sFailOp, sZFailOp, sPassOp, static_cast<ComparisonFunction>(data));
                                material->setRenderStateBlock(block.getHash(), tempPass._stage, tempPass._passType, tempPass._variant);
                            };
                            _context.editor().registerUndoEntry(stencilUndo);

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
                const RenderStagePass tempPass = currentStagePass;
                RegisterUndo<bool, false>(_parent, GFX::PushConstantType::BOOL, !frontFaceCCW, frontFaceCCW, "CCW front face", [material, stateHash, tempPass](const bool& oldVal) {
                    RenderStateBlock block = RenderStateBlock::get(stateHash);
                    block.setFrontFaceCCW(oldVal);
                    material->setRenderStateBlock(block.getHash(), tempPass._stage, tempPass._passType, tempPass._variant);
                });

                block.setFrontFaceCCW(frontFaceCCW);
                changed = true;
            }

            bool scissorEnabled = block.scissorTestEnabled();
            if (ImGui::Checkbox("Scissor test", &scissorEnabled)) {
                const RenderStagePass tempPass = currentStagePass;
                RegisterUndo<bool, false>(_parent, GFX::PushConstantType::BOOL, !scissorEnabled, scissorEnabled, "Scissor test", [material, stateHash, tempPass](const bool& oldVal) {
                    RenderStateBlock block = RenderStateBlock::get(stateHash);
                    block.setScissorTest(oldVal);
                    material->setRenderStateBlock(block.getHash(), tempPass._stage, tempPass._passType, tempPass._variant);
                });

                block.setScissorTest(scissorEnabled);
                changed = true;
            }

            bool depthTestEnabled = block.depthTestEnabled();
            if (ImGui::Checkbox("Depth test", &depthTestEnabled)) {
                const RenderStagePass tempPass = currentStagePass;
                RegisterUndo<bool, false>(_parent, GFX::PushConstantType::BOOL, !depthTestEnabled, depthTestEnabled, "Depth test", [material, stateHash, tempPass](const bool& oldVal) {
                    RenderStateBlock block = RenderStateBlock::get(stateHash);
                    block.depthTestEnabled(oldVal);
                    material->setRenderStateBlock(block.getHash(), tempPass._stage, tempPass._passType, tempPass._variant);
                });

                block.depthTestEnabled(depthTestEnabled);
                changed = true;
            }

            if (ImGui::Checkbox("Stencil test", &stencilEnabled)) {
                const RenderStagePass tempPass = currentStagePass;
                RegisterUndo<bool, false>(_parent, GFX::PushConstantType::BOOL, !stencilEnabled, stencilEnabled, "Stencil test", [material, stateHash, tempPass, stencilRef, sFailOp, sZFailOp, sPassOp, sFunc](const bool& oldVal) {
                    RenderStateBlock block = RenderStateBlock::get(stateHash);
                    block.setStencil(oldVal, stencilRef, sFailOp, sZFailOp, sPassOp, sFunc);
                    material->setRenderStateBlock(block.getHash(), tempPass._stage, tempPass._passType, tempPass._variant);
                });

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

        const ShadingMode crtMode = material->shadingMode();
        const char* crtModeName = TypeUtil::ShadingModeToString(crtMode);
        if (ImGui::CollapsingHeader(Util::StringFormat("Shading Mode [ %s ]", crtModeName).c_str())) {
            const auto diffuseSetter = [material](const void* data) {
                material->baseColour(*(FColour4*)data);
            };
            {
                static UndoEntry<I32> modeUndo = {};
                ImGui::PushItemWidth(250);
                if (ImGui::BeginCombo("[Shading Mode]", crtModeName, ImGuiComboFlags_PopupAlignLeft)) {
                    for (U8 n = 0; n < to_U8(ShadingMode::COUNT); ++n) {
                        const ShadingMode mode = static_cast<ShadingMode>(n);
                        const bool isSelected = crtMode == mode;

                        if (ImGui::Selectable(TypeUtil::ShadingModeToString(mode), isSelected)) {

                            modeUndo._type = GFX::PushConstantType::INT;
                            modeUndo._name = "Shading Mode";
                            modeUndo._oldVal = to_I32(crtMode);
                            modeUndo._newVal = to_I32(mode);
                            modeUndo._dataSetter = [material, mode](const I32& data) {
                                material->shadingMode(mode);
                            };
                            _context.editor().registerUndoEntry(modeUndo);

                            material->shadingMode(mode);
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();
            }
            I32 id = 0;
            ApplyAllButton(id, material, false, [&material]() {
                if (material->baseMaterial() != nullptr) {
                    material->baseMaterial()->shadingMode(material->shadingMode(), true);
                }
            });

            bool fromTexture = false;
            Texture* texture = nullptr;
            { //Base colour
                ImGui::Separator();
                ImGui::PushItemWidth(250);
                FColour4 diffuse = material->getBaseColour(fromTexture, texture);
                if (colourInput4(_parent, "[Albedo]", diffuse, readOnly, diffuseSetter)) {
                    diffuseSetter(diffuse._v);
                    ret = true;
                }
                ImGui::PopItemWidth();
                if (fromTexture && ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Albedo is sampled from a texture. Base colour possibly unused!");
                }
                ApplyAllButton(id, material, fromTexture || readOnly, [&material]() {
                    if (material->baseMaterial() != nullptr) {
                        material->baseMaterial()->baseColour(material->baseColour(), true);
                    }
                });
                if (PreviewTextureButton(id, texture, !fromTexture)) {
                    _previewTexture = texture;
                }
                ImGui::Separator();
            }
            { //Emissive
                ImGui::Separator();
                const auto emissiveSetter = [material](const void* data) {
                    material->emissiveColour(*(FColour3*)data);
                };
                ImGui::PushItemWidth(250);
                FColour3 emissive = material->getEmissive(fromTexture, texture);
                if (colourInput3(_parent, "[Emissive]", emissive, fromTexture || readOnly, emissiveSetter)) {
                    emissiveSetter(emissive._v);
                    ret = true;
                }
                ImGui::PopItemWidth();
                if (fromTexture && ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Control managed by application (e.g. is overriden by a texture)");
                }
                ApplyAllButton(id, material, fromTexture || readOnly, [&material]() {
                    if (material->baseMaterial() != nullptr) {
                        material->baseMaterial()->emissiveColour(material->emissive(), true);
                    }
                });
                if (PreviewTextureButton(id, texture, !fromTexture)) {
                    _previewTexture = texture;
                }
                ImGui::Separator();
            }
            { // Metallic
                ImGui::Separator();
                F32 metallic = material->getMetallic(fromTexture, texture);
                EditorComponentField tempField = {};
                tempField._name = "[Metallic]";
                tempField._basicType = GFX::PushConstantType::FLOAT;
                tempField._type = EditorComponentFieldType::SLIDER_TYPE;
                tempField._readOnly = fromTexture || readOnly;
                if (fromTexture) {
                    tempField._tooltip = "Control managed by application (e.g. is overriden by a texture)";
                }
                tempField._data = &metallic;
                tempField._range = { 0.0f, 1.0f };
                tempField._dataSetter = [material](const void* metallic) {
                    material->metallic(*static_cast<const F32*>(metallic));
                };
                
                ImGui::PushItemWidth(175);
                ret = processBasicField(tempField) || ret;
                ImGui::PopItemWidth();

                ImGui::SameLine();
                ImGui::Text(tempField._name.c_str());
                ApplyAllButton(id, material, tempField._readOnly, [&material]() {
                    if (material->baseMaterial() != nullptr) {
                        material->baseMaterial()->metallic(material->metallic(), true);
                    }
                });
                if (PreviewTextureButton(id, texture, !fromTexture)) {
                    _previewTexture = texture;
                }
                ImGui::Separator();
            }
            { // Roughness
                ImGui::Separator();
                F32 roughness = material->getRoughness(fromTexture, texture);
                EditorComponentField tempField = {};
                tempField._name = "[Roughness]";
                tempField._basicType = GFX::PushConstantType::FLOAT;
                tempField._type = EditorComponentFieldType::SLIDER_TYPE;
                tempField._readOnly = fromTexture || readOnly;
                if (fromTexture) {
                    tempField._tooltip = "Control managed by application (e.g. is overriden by a texture)";
                }
                tempField._data = &roughness;
                tempField._range = { 0.0f, 1.0f };
                tempField._dataSetter = [material](const void* roughness) {
                    material->roughness(*static_cast<const F32*>(roughness));
                };
                ImGui::PushItemWidth(175);
                ret = processBasicField(tempField) || ret;
                ImGui::PopItemWidth();
                ImGui::SameLine();
                ImGui::Text(tempField._name.c_str());
                ApplyAllButton(id, material, tempField._readOnly, [&material]() {
                    if (material->baseMaterial() != nullptr) {
                        material->baseMaterial()->roughness(material->roughness(), true);
                    }
                });
                if (PreviewTextureButton(id, texture, !fromTexture)) {
                    _previewTexture = texture;
                }
                ImGui::Separator();
            }
            { // Parallax
                ImGui::Separator();
                F32 parallax = material->parallaxFactor();
                EditorComponentField tempField = {};
                tempField._name = "[Parallax]";
                tempField._basicType = GFX::PushConstantType::FLOAT;
                tempField._type = EditorComponentFieldType::SLIDER_TYPE;
                tempField._readOnly = readOnly;
                tempField._data = &parallax;
                tempField._range = { 0.0f, 1.0f };
                tempField._dataSetter = [material](const void* parallax) {
                    material->parallaxFactor(*static_cast<const F32*>(parallax));
                };
                ImGui::PushItemWidth(175);
                ret = processBasicField(tempField) || ret;
                ImGui::PopItemWidth();
                ImGui::SameLine();
                ImGui::Text(tempField._name.c_str());
                ApplyAllButton(id, material, tempField._readOnly, [&material]() {
                    if (material->baseMaterial() != nullptr) {
                        material->baseMaterial()->parallaxFactor(material->parallaxFactor(), true);
                    }
                });
                ImGui::Separator();
            }
            bool doubleSided = material->doubleSided();
            bool receivesShadows = material->receivesShadows();
            bool refractive = material->refractive();

            if (ImGui::Checkbox("[Double Sided]", &doubleSided) && !readOnly) {
                RegisterUndo<bool, false>(_parent, GFX::PushConstantType::BOOL, !doubleSided, doubleSided, "DoubleSided", [material](const bool& oldVal) {
                    material->doubleSided(oldVal);
                });
                material->doubleSided(doubleSided);
                ret = true;
            }
            ApplyAllButton(id, material, false, [&material]() {
                if (material->baseMaterial() != nullptr) {
                    material->baseMaterial()->doubleSided(material->doubleSided(), true);
                }
            });
            if (ImGui::Checkbox("[Receives Shadows]", &receivesShadows) && !readOnly) {
                RegisterUndo<bool, false>(_parent, GFX::PushConstantType::BOOL, !receivesShadows, receivesShadows, "ReceivesShadows", [material](const bool& oldVal) {
                    material->receivesShadows(oldVal);
                });
                material->receivesShadows(receivesShadows);
                ret = true;
            }
            ApplyAllButton(id, material, false, [&material]() {
                if (material->baseMaterial() != nullptr) {
                    material->baseMaterial()->receivesShadows(material->receivesShadows(), true);
                }
            });
            if (ImGui::Checkbox("[Refractive]", &refractive) && !readOnly) {
                RegisterUndo<bool, false>(_parent, GFX::PushConstantType::BOOL, !refractive, refractive, "Refractive", [material](const bool& oldVal) {
                    material->refractive(oldVal);
                });
                material->refractive(refractive);
                ret = true;
            }
            ApplyAllButton(id, material, false, [&material]() {
                if (material->baseMaterial() != nullptr) {
                    material->baseMaterial()->refractive(material->refractive(), true);
                }
            });
        }
        ImGui::Separator();

        if (readOnly) {
            PopReadOnly();
            ret = false;
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
                    static UndoEntry<bool> undo = {};

                    bool val = field.get<bool>();
                    ret = ImGui::Checkbox("", &val);
                    if (ret && !field._readOnly) {
                        RegisterUndo<bool, false>(_parent, GFX::PushConstantType::BOOL, !val, val, name, [&field](const bool& oldVal) {
                            field.set(oldVal);
                        });
                        field.set<bool>(val);
                    }
                }break;
                case GFX::PushConstantType::INT: {
                switch (field._basicTypeSize) {
                    case GFX::PushConstantSize::QWORD: ret = inputOrSlider<I64, 1>(_parent, isSlider, "", name, step, ImGuiDataType_S64, field, flags, field._format); break;
                    case GFX::PushConstantSize::DWORD: ret = inputOrSlider<I32, 1>(_parent, isSlider, "", name, step, ImGuiDataType_S32, field, flags, field._format); break;
                    case GFX::PushConstantSize::WORD:  ret = inputOrSlider<I16, 1>(_parent, isSlider, "", name, step, ImGuiDataType_S16, field, flags, field._format); break;
                    case GFX::PushConstantSize::BYTE:  ret = inputOrSlider<I8,  1>(_parent, isSlider, "", name, step, ImGuiDataType_S8,  field, flags, field._format); break;
                    default: DIVIDE_UNEXPECTED_CALL(); break;
                    }
                }break;
                case GFX::PushConstantType::UINT: {
                    switch (field._basicTypeSize) {
                        case GFX::PushConstantSize::QWORD: ret = inputOrSlider<U64, 1>(_parent, isSlider, "", name, step, ImGuiDataType_U64, field, flags, field._format); break;
                        case GFX::PushConstantSize::DWORD: ret = inputOrSlider<U32, 1>(_parent, isSlider, "", name, step, ImGuiDataType_U32, field, flags, field._format); break;
                        case GFX::PushConstantSize::WORD:  ret = inputOrSlider<U16, 1>(_parent, isSlider, "", name, step, ImGuiDataType_U16, field, flags, field._format); break;
                        case GFX::PushConstantSize::BYTE:  ret = inputOrSlider<U8,  1>(_parent, isSlider, "", name, step, ImGuiDataType_U8,  field, flags, field._format); break;
                    default: DIVIDE_UNEXPECTED_CALL(); break;
                    }
                }break;
                case GFX::PushConstantType::DOUBLE: {
                    ret = inputOrSlider<D64, 1>(_parent, isSlider, "", name, step, ImGuiDataType_Double, field, flags, field._format);
                }break;
                case GFX::PushConstantType::FLOAT: {
                    ret = inputOrSlider<F32, 1>(_parent, isSlider, "", name, step, ImGuiDataType_Float, field, flags, field._format);
                }break;
                case GFX::PushConstantType::IVEC2: {
                    switch (field._basicTypeSize) {
                        case GFX::PushConstantSize::QWORD: ret = inputOrSlider<vec2<I64>, 2>(_parent, isSlider, "", name, step, ImGuiDataType_S64, field, flags, field._format); break;
                        case GFX::PushConstantSize::DWORD: ret = inputOrSlider<vec2<I32>, 2>(_parent, isSlider, "", name, step, ImGuiDataType_S32, field, flags, field._format); break;
                        case GFX::PushConstantSize::WORD:  ret = inputOrSlider<vec2<I16>, 2>(_parent, isSlider, "", name, step, ImGuiDataType_S16, field, flags, field._format); break;
                        case GFX::PushConstantSize::BYTE:  ret = inputOrSlider<vec2<I8>,  2>(_parent, isSlider, "", name, step, ImGuiDataType_S8,  field, flags, field._format); break;
                        default: DIVIDE_UNEXPECTED_CALL(); break;
                    }
                }break;
                case GFX::PushConstantType::IVEC3: {
                    switch (field._basicTypeSize) {
                        case GFX::PushConstantSize::QWORD: ret = inputOrSlider<vec3<I64>, 3>(_parent, isSlider, "", name, step, ImGuiDataType_S64, field, flags, field._format); break;
                        case GFX::PushConstantSize::DWORD: ret = inputOrSlider<vec3<I32>, 3>(_parent, isSlider, "", name, step, ImGuiDataType_S32, field, flags, field._format); break;
                        case GFX::PushConstantSize::WORD:  ret = inputOrSlider<vec3<I16>, 3>(_parent, isSlider, "", name, step, ImGuiDataType_S16, field, flags, field._format); break;
                        case GFX::PushConstantSize::BYTE:  ret = inputOrSlider<vec3<I8>,  3>(_parent, isSlider, "", name, step, ImGuiDataType_S8,  field, flags, field._format); break;
                        default: DIVIDE_UNEXPECTED_CALL(); break;
                    }
                }break;
                case GFX::PushConstantType::IVEC4: {
                    switch (field._basicTypeSize) {
                        case GFX::PushConstantSize::QWORD: ret = inputOrSlider<vec4<I64>, 4>(_parent, isSlider, "", name, step, ImGuiDataType_S64, field, flags, field._format); break;
                        case GFX::PushConstantSize::DWORD: ret = inputOrSlider<vec4<I32>, 4>(_parent, isSlider, "", name, step, ImGuiDataType_S32, field, flags, field._format); break;
                        case GFX::PushConstantSize::WORD:  ret = inputOrSlider<vec4<I16>, 4>(_parent, isSlider, "", name, step, ImGuiDataType_S16, field, flags, field._format); break;
                        case GFX::PushConstantSize::BYTE:  ret = inputOrSlider<vec4<I8>,  4>(_parent, isSlider, "", name, step, ImGuiDataType_S8,  field, flags, field._format); break;
                        default: DIVIDE_UNEXPECTED_CALL(); break;
                    }
                }break;
                case GFX::PushConstantType::UVEC2: {
                    switch (field._basicTypeSize) {
                        case GFX::PushConstantSize::QWORD: ret = inputOrSlider<vec2<U64>, 2>(_parent, isSlider, "", name, step, ImGuiDataType_U64, field, flags, field._format); break;
                        case GFX::PushConstantSize::DWORD: ret = inputOrSlider<vec2<U32>, 2>(_parent, isSlider, "", name, step, ImGuiDataType_U32, field, flags, field._format); break;
                        case GFX::PushConstantSize::WORD:  ret = inputOrSlider<vec2<U16>, 2>(_parent, isSlider, "", name, step, ImGuiDataType_U16, field, flags, field._format); break;
                        case GFX::PushConstantSize::BYTE:  ret = inputOrSlider<vec2<U8>,  2>(_parent, isSlider, "", name, step, ImGuiDataType_U8,  field, flags, field._format); break;
                        default: DIVIDE_UNEXPECTED_CALL(); break;
                    }
                }break;
                case GFX::PushConstantType::UVEC3: {
                    switch (field._basicTypeSize) {
                        case GFX::PushConstantSize::QWORD: ret = inputOrSlider<vec3<U64>, 3>(_parent, isSlider, "", name, step, ImGuiDataType_U64, field, flags, field._format); break;
                        case GFX::PushConstantSize::DWORD: ret = inputOrSlider<vec3<U32>, 3>(_parent, isSlider, "", name, step, ImGuiDataType_U32, field, flags, field._format); break;
                        case GFX::PushConstantSize::WORD:  ret = inputOrSlider<vec3<U16>, 3>(_parent, isSlider, "", name, step, ImGuiDataType_U16, field, flags, field._format); break;
                        case GFX::PushConstantSize::BYTE:  ret = inputOrSlider<vec3<U8>,  3>(_parent, isSlider, "", name, step, ImGuiDataType_U8,  field, flags, field._format); break;
                        default: DIVIDE_UNEXPECTED_CALL(); break;
                    }
                }break;
                case GFX::PushConstantType::UVEC4: {
                    switch (field._basicTypeSize) {
                        case GFX::PushConstantSize::QWORD: ret = inputOrSlider<vec4<U64>, 4>(_parent, isSlider, "", name, step, ImGuiDataType_U64, field, flags, field._format); break;
                        case GFX::PushConstantSize::DWORD: ret = inputOrSlider<vec4<U32>, 4>(_parent, isSlider, "", name, step, ImGuiDataType_U32, field, flags, field._format); break;
                        case GFX::PushConstantSize::WORD:  ret = inputOrSlider<vec4<U16>, 4>(_parent, isSlider, "", name, step, ImGuiDataType_U16, field, flags, field._format); break;
                        case GFX::PushConstantSize::BYTE:  ret = inputOrSlider<vec4<U8>,  4>(_parent, isSlider, "", name, step, ImGuiDataType_U8,  field, flags, field._format); break;
                        default: DIVIDE_UNEXPECTED_CALL(); break;
                    }
                }break;
                case GFX::PushConstantType::VEC2: {
                    ret = inputOrSlider<vec2<F32>, 2>(_parent, isSlider, "", name, step, ImGuiDataType_Float, field, flags, field._format);
                }break;
                case GFX::PushConstantType::VEC3: {
                    ret = inputOrSlider<vec3<F32>, 3>(_parent, isSlider, "", name, step, ImGuiDataType_Float, field, flags, field._format);
                }break;
                case GFX::PushConstantType::VEC4: {
                    ret = inputOrSlider<vec4<F32>, 4>(_parent, isSlider, "", name, step, ImGuiDataType_Float, field, flags, field._format);
                }break;
                case GFX::PushConstantType::DVEC2: {
                    ret = inputOrSlider<vec2<D64>, 2>(_parent, isSlider, "", name, step, ImGuiDataType_Double, field, flags, field._format);
                }break;
                case GFX::PushConstantType::DVEC3: {
                    ret = inputOrSlider<vec3<D64>, 3>(_parent, isSlider, "", name, step, ImGuiDataType_Double, field, flags, field._format);
                }break;
                case GFX::PushConstantType::DVEC4: {
                    ret = inputOrSlider<vec4<D64>, 4>(_parent, isSlider, "", name, step, ImGuiDataType_Double, field, flags, field._format);
                }break;
                case GFX::PushConstantType::IMAT2: {
                    switch (field._basicTypeSize) {
                        case GFX::PushConstantSize::QWORD: ret = inputMatrix<mat2<I64>, 2>(_parent, "", name, step, ImGuiDataType_S64, field, flags, field._format); break;
                        case GFX::PushConstantSize::DWORD: ret = inputMatrix<mat2<I32>, 2>(_parent, "", name, step, ImGuiDataType_S32, field, flags, field._format); break;
                        case GFX::PushConstantSize::WORD:  ret = inputMatrix<mat2<I16>, 2>(_parent, "", name, step, ImGuiDataType_S16, field, flags, field._format); break;
                        case GFX::PushConstantSize::BYTE:  ret = inputMatrix<mat2<I8>,  2>(_parent, "", name, step, ImGuiDataType_S8,  field, flags, field._format); break;
                        default: DIVIDE_UNEXPECTED_CALL(); break;
                    }
                }break;
                case GFX::PushConstantType::IMAT3: {
                    switch (field._basicTypeSize) {
                        case GFX::PushConstantSize::QWORD: ret = inputMatrix<mat3<I64>, 3>(_parent, "", name, step, ImGuiDataType_S64, field, flags, field._format); break;
                        case GFX::PushConstantSize::DWORD: ret = inputMatrix<mat3<I32>, 3>(_parent, "", name, step, ImGuiDataType_S32, field, flags, field._format); break;
                        case GFX::PushConstantSize::WORD:  ret = inputMatrix<mat3<I16>, 3>(_parent, "", name, step, ImGuiDataType_S16, field, flags, field._format); break;
                        case GFX::PushConstantSize::BYTE:  ret = inputMatrix<mat3<I8>,  3>(_parent, "", name, step, ImGuiDataType_S8,  field, flags, field._format); break;
                        default: DIVIDE_UNEXPECTED_CALL(); break;
                    }
                }break;
                case GFX::PushConstantType::IMAT4: {
                    switch (field._basicTypeSize) {
                    case GFX::PushConstantSize::QWORD: ret = inputMatrix<mat4<I64>, 4>(_parent, "", name, step, ImGuiDataType_S64, field, flags, field._format); break;
                    case GFX::PushConstantSize::DWORD: ret = inputMatrix<mat4<I32>, 4>(_parent, "", name, step, ImGuiDataType_S32, field, flags, field._format); break;
                    case GFX::PushConstantSize::WORD:  ret = inputMatrix<mat4<I16>, 4>(_parent, "", name, step, ImGuiDataType_S16, field, flags, field._format); break;
                    case GFX::PushConstantSize::BYTE:  ret = inputMatrix<mat4<I8>,  4>(_parent, "", name, step, ImGuiDataType_S8,  field, flags, field._format); break;
                        default: DIVIDE_UNEXPECTED_CALL(); break;
                    }
                }break;
                case GFX::PushConstantType::UMAT2: {
                    switch (field._basicTypeSize) {
                        case GFX::PushConstantSize::QWORD: ret = inputMatrix<mat2<U64>, 2>(_parent, "", name, step, ImGuiDataType_U64, field, flags, field._format); break;
                        case GFX::PushConstantSize::DWORD: ret = inputMatrix<mat2<U32>, 2>(_parent, "", name, step, ImGuiDataType_U32, field, flags, field._format); break;
                        case GFX::PushConstantSize::WORD:  ret = inputMatrix<mat2<U16>, 2>(_parent, "", name, step, ImGuiDataType_U16, field, flags, field._format); break;
                        case GFX::PushConstantSize::BYTE:  ret = inputMatrix<mat2<U8>,  2>(_parent, "", name, step, ImGuiDataType_U8,  field, flags, field._format); break;
                        default: DIVIDE_UNEXPECTED_CALL(); break;
                    }
                }break;
                case GFX::PushConstantType::UMAT3: {
                    switch (field._basicTypeSize) {
                        case GFX::PushConstantSize::QWORD: ret = inputMatrix<mat3<U64>, 3>(_parent, "", name, step, ImGuiDataType_U64, field, flags, field._format); break;
                        case GFX::PushConstantSize::DWORD: ret = inputMatrix<mat3<U32>, 3>(_parent, "", name, step, ImGuiDataType_U32, field, flags, field._format); break;
                        case GFX::PushConstantSize::WORD:  ret = inputMatrix<mat3<U16>, 3>(_parent, "", name, step, ImGuiDataType_U16, field, flags, field._format); break;
                        case GFX::PushConstantSize::BYTE:  ret = inputMatrix<mat3<U8>,  3>(_parent, "", name, step, ImGuiDataType_U8,  field, flags, field._format); break;
                        default: DIVIDE_UNEXPECTED_CALL(); break;
                    }
                }break;
                case GFX::PushConstantType::UMAT4: {
                    switch (field._basicTypeSize) {
                        case GFX::PushConstantSize::QWORD: ret = inputMatrix<mat4<U64>, 4>(_parent, "", name, step, ImGuiDataType_U64, field, flags, field._format); break;
                        case GFX::PushConstantSize::DWORD: ret = inputMatrix<mat4<U32>, 4>(_parent, "", name, step, ImGuiDataType_U32, field, flags, field._format); break;
                        case GFX::PushConstantSize::WORD:  ret = inputMatrix<mat4<U16>, 4>(_parent, "", name, step, ImGuiDataType_U16, field, flags, field._format); break;
                        case GFX::PushConstantSize::BYTE:  ret = inputMatrix<mat4<U8>,  4>(_parent, "", name, step, ImGuiDataType_U8,  field, flags, field._format); break;
                        default: DIVIDE_UNEXPECTED_CALL(); break;
                    }
                }break;
                case GFX::PushConstantType::MAT2: {
                    ret = inputMatrix<mat2<F32>, 2>(_parent, "", name, step, ImGuiDataType_Float, field, flags, field._format);
                }break;
                case GFX::PushConstantType::MAT3: {
                    ret = inputMatrix<mat3<F32>, 3>(_parent, "", name, step, ImGuiDataType_Float, field, flags, field._format);
                }break;
                case GFX::PushConstantType::MAT4: {
                    ret = inputMatrix<mat4<F32>, 4>(_parent, "", name, step, ImGuiDataType_Float, field, flags, field._format);
                }break;
                case GFX::PushConstantType::DMAT2: {
                    ret = inputMatrix<mat2<D64>, 2>(_parent, "", name, step, ImGuiDataType_Double, field, flags, field._format);
                }break;
                case GFX::PushConstantType::DMAT3: {
                    ret = inputMatrix<mat3<D64>, 3>(_parent, "", name, step, ImGuiDataType_Double, field, flags, field._format);
                }break;
                case GFX::PushConstantType::DMAT4: {
                    ret = inputMatrix<mat4<D64>, 4>(_parent, "", name, step, ImGuiDataType_Double, field, flags, field._format);
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

            ImGui::PopID();

            return ret;
        }

        const char* PropertyWindow::name() const noexcept {
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
