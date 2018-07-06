#include "stdafx.h"

#include "Headers/PropertyWindow.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/PlatformContext.h"
#include "Managers/Headers/SceneManager.h"

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
        SceneManager& sceneManager = context().kernel().sceneManager();
        Scene& activeScene = sceneManager.getActiveScene();

        const vectorImpl<I64>& selections = activeScene.getCurrentSelection();
        for (I64 nodeGUID : selections) {
            SceneGraphNode* node = activeScene.sceneGraph().findNode(nodeGUID);
            if (node != nullptr) {
                ImGui::Text(node->getName().c_str());

                vectorImpl<EditorComponent*>& editorComp = Attorney::SceneGraphNodeEditor::editorComponents(*node);
                for (EditorComponent* comp : editorComp) {
                    if (ImGui::CollapsingHeader(comp->name().c_str()))
                    {
                        vectorImpl<EditorComponentField>& fields = Attorney::EditorComponentEditor::fields(*comp);
                        for (EditorComponentField& field : fields) {
                            ImGui::Text(field._name.c_str());
                            ImGui::Spacing();
                            processField(field);
                        }
                    }
                }
            }
        }
    }

     void PropertyWindow::processField(EditorComponentField& field) {
        switch (field._type) {
            case EditorComponentFieldType::PUSH_TYPE: {
                processBasicField(field);
            }break;
            case EditorComponentFieldType::BOUNDING_BOX: {
                BoundingBox* bb = static_cast<BoundingBox*>(field._data);
                F32* bbMin = Attorney::BoundingBoxEditor::min(*bb);
                F32* bbMax = Attorney::BoundingBoxEditor::max(*bb);
                ImGui::InputFloat3("- Min ", bbMin);
                ImGui::InputFloat3("- Max", bbMax);
            }break;
            case EditorComponentFieldType::BOUNDING_SPHERE: {
                BoundingSphere* bs = static_cast<BoundingSphere*>(field._data);
                F32* center = Attorney::BoundingSphereEditor::center(*bs);
                F32& radius = Attorney::BoundingSphereEditor::radius(*bs);
                ImGui::InputFloat3("- Center ", center);
                ImGui::InputFloat("- Radius ", &radius);
            }break;
            case EditorComponentFieldType::TRANSFORM: {

            }break;
        };
    }

     void PropertyWindow::processBasicField(EditorComponentField& field) {
         switch (field._basicType) {
             case GFX::PushConstantType::BOOL: {
                 ImGui::SameLine(); ImGui::Checkbox("", (bool*)(field._data));
             }break;
             case GFX::PushConstantType::INT: {
                 ImGui::SameLine(); ImGui::InputInt("", (I32*)(field._data));
             }break;
             case GFX::PushConstantType::UINT: {
                 ImGui::Text("Not currently supported");
             }break;
             case GFX::PushConstantType::DOUBLE: {
                 ImGui::Text("Not currently supported");
             }break;
             case GFX::PushConstantType::FLOAT: {
                 ImGui::SameLine(); ImGui::InputFloat("", (F32*)(field._data));
             }break;
             case GFX::PushConstantType::IVEC2: {
                 ImGui::SameLine(); ImGui::InputInt2("", (I32*)(field._data));
             }break;
             case GFX::PushConstantType::IVEC3: {
                 ImGui::SameLine(); ImGui::InputInt3("", (I32*)(field._data));
             }break;
             case GFX::PushConstantType::IVEC4: {
                 ImGui::SameLine(); ImGui::InputInt4("", (I32*)(field._data));
             }break;
             case GFX::PushConstantType::UVEC2: {
                 ImGui::SameLine(); ImGui::Text("Not currently supported");
             }break;
             case GFX::PushConstantType::UVEC3: {
                 ImGui::SameLine(); ImGui::Text("Not currently supported");
             }break;
             case GFX::PushConstantType::UVEC4: {
                 ImGui::SameLine(); ImGui::Text("Not currently supported");
             }break;
             case GFX::PushConstantType::VEC2: {
                 ImGui::SameLine(); ImGui::InputFloat2("", (F32*)(field._data));
             }break;
             case GFX::PushConstantType::VEC3: {
                 ImGui::SameLine(); ImGui::InputFloat3("", (F32*)(field._data));
             }break;
             case GFX::PushConstantType::VEC4: {
                 ImGui::SameLine(); ImGui::InputFloat4("", (F32*)(field._data));
             }break;
             case GFX::PushConstantType::DVEC2: {
                 ImGui::SameLine(); ImGui::Text("Not currently supported");
             }break;
             case GFX::PushConstantType::DVEC3: {
                 ImGui::SameLine(); ImGui::Text("Not currently supported");
             }break;
             case GFX::PushConstantType::DVEC4: {
                 ImGui::SameLine(); ImGui::Text("Not currently supported");
             }break;
             case GFX::PushConstantType::IMAT2: {
                 mat2<I32>* mat = (mat2<I32>*)(field._data);
                 ImGui::InputInt2("", mat->_vec[0]);
                 ImGui::InputInt2("", mat->_vec[1]);
             }break;
             case GFX::PushConstantType::IMAT3: {
                 mat3<I32>* mat = (mat3<I32>*)(field._data);
                 ImGui::InputInt3("", mat->_vec[0]);
                 ImGui::InputInt3("", mat->_vec[1]);
                 ImGui::InputInt3("", mat->_vec[2]);
             }break;
             case GFX::PushConstantType::IMAT4: {
                 mat4<I32>* mat = (mat4<I32>*)(field._data);
                 ImGui::InputInt4("", mat->_vec[0]);
                 ImGui::InputInt4("", mat->_vec[1]);
                 ImGui::InputInt4("", mat->_vec[2]);
                 ImGui::InputInt4("", mat->_vec[3]);
             }break;
             case GFX::PushConstantType::UMAT2: {
                 ImGui::SameLine(); ImGui::Text("Not currently supported");
             }break;
             case GFX::PushConstantType::UMAT3: {
                 ImGui::SameLine(); ImGui::Text("Not currently supported");
             }break;
             case GFX::PushConstantType::UMAT4: {
                 ImGui::SameLine(); ImGui::Text("Not currently supported");
             }break;
             case GFX::PushConstantType::MAT2: {
                 mat2<F32>* mat = (mat2<F32>*)(field._data);
                 ImGui::InputFloat2("", mat->_vec[0]);
                 ImGui::InputFloat2("", mat->_vec[1]);
             }break;
             case GFX::PushConstantType::MAT3: {
                 mat3<F32>* mat = (mat3<F32>*)(field._data);
                 ImGui::InputFloat3("", mat->_vec[0]);
                 ImGui::InputFloat3("", mat->_vec[1]);
                 ImGui::InputFloat3("", mat->_vec[2]);
             }break;
             case GFX::PushConstantType::MAT4: {
                 mat4<F32>* mat = (mat4<F32>*)(field._data);
                 ImGui::InputFloat4("", mat->_vec[0]);
                 ImGui::InputFloat4("", mat->_vec[1]);
                 ImGui::InputFloat4("", mat->_vec[2]);
                 ImGui::InputFloat4("", mat->_vec[3]);
             }break;
             case GFX::PushConstantType::DMAT2: {
                 ImGui::SameLine(); ImGui::Text("Not currently supported");
             }break;
             case GFX::PushConstantType::DMAT3: {
                 ImGui::SameLine(); ImGui::Text("Not currently supported");
             }break;
             case GFX::PushConstantType::DMAT4: {
                 ImGui::SameLine(); ImGui::Text("Not currently supported");
             }break;
             default: {
                 ImGui::SameLine(); ImGui::Text("error");
             }break;
         }
     }
}; //namespace Divide
