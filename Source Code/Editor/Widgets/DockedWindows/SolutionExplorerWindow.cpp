#include "stdafx.h"

#include "Headers/SolutionExplorerWindow.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/PlatformContext.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Dynamics/Entities/Units/Headers/Player.h"
#include "Widgets/Headers/PanelManager.h"

namespace Divide {
    namespace {
        std::deque<F32> g_framerateBuffer;
        std::vector<F32> g_framerateBufferCont;
    };

    SolutionExplorerWindow::SolutionExplorerWindow(PanelManager& parent, PlatformContext& context)
        : DockedWindow(parent, "Solution Explorer"),
          PlatformContextComponent(context)
    {
        g_framerateBufferCont.reserve(256);
    }

    SolutionExplorerWindow::~SolutionExplorerWindow()
    {

    }

    void SolutionExplorerWindow::printCameraNode(SceneManager& sceneManager, Camera* camera) {
        if (camera == nullptr) {
            return;
        }

        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_Leaf;
        if (ImGui::TreeNodeEx((void*)(intptr_t)camera->getGUID(), node_flags, camera->name().c_str())) {
            if (ImGui::IsItemClicked()) {
                sceneManager.resetSelection(0);
                Attorney::PanelManagerDockedWindows::setSelectedCamera(_parent, camera);
            }

            ImGui::TreePop();
        }
    }

    void SolutionExplorerWindow::printSceneGraphNode(SceneManager& sceneManager, SceneGraphNode& sgn) {
        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

        if (sgn.getSelectionFlag() == SceneGraphNode::SelectionFlag::SELECTION_SELECTED) {
            node_flags |= ImGuiTreeNodeFlags_Selected;
        }

        if (!sgn.hasChildren()) {
            node_flags |= ImGuiTreeNodeFlags_Leaf /*| ImGuiTreeNodeFlags_NoTreePushOnOpen*/; // ImGuiTreeNodeFlags_Bullet
        }
        bool node_open = ImGui::TreeNodeEx((void*)(intptr_t)sgn.getGUID(), node_flags, sgn.name().c_str());
        if (ImGui::IsItemClicked()) {
            sceneManager.resetSelection(0);
            sceneManager.setSelected(0, sgn);
            Attorney::PanelManagerDockedWindows::setSelectedCamera(_parent, nullptr);
        }
        if (node_open) {
            sgn.forEachChild([this, &sceneManager](SceneGraphNode& child) {
                printSceneGraphNode(sceneManager, child);
            });
            ImGui::TreePop();
        }
        
        
    }

    void SolutionExplorerWindow::draw() {
        //show_test_window ^= ImGui::Button("Test Window");
        //show_another_window ^= ImGui::Button("Another Window");

        SceneManager& sceneManager = context().kernel().sceneManager();
        Scene& activeScene = sceneManager.getActiveScene();
        SceneGraphNode& root = activeScene.sceneGraph().getRoot();


        if (ImGui::TreeNode(activeScene.name().c_str()))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, ImGui::GetFontSize() * 3); // Increase spacing to differentiate leaves from expanded contents.
            for (PlayerIndex i = 0; i < static_cast<PlayerIndex>(Config::MAX_LOCAL_PLAYER_COUNT); ++i) {
                printCameraNode(sceneManager, Attorney::SceneManagerCameraAccessor::playerCamera(sceneManager, i));
            }
            printSceneGraphNode(sceneManager, root);
            ImGui::PopStyleVar();
            ImGui::TreePop();
        }

        const vector<stringImpl>& scenes = sceneManager.sceneNameList();
        for (const stringImpl& scene : scenes) {
            if (scene != activeScene.name()) {
                if (ImGui::TreeNodeEx(scene.c_str(), ImGuiTreeNodeFlags_Leaf)) {
                    ImGui::TreePop();
                }
            }
        }

        // Calculate and show framerate
        static F32 ms_per_frame[120] = { 0 };
        static I32 ms_per_frame_idx = 0;
        static F32 ms_per_frame_accum = 0.0f;
        ms_per_frame_accum -= ms_per_frame[ms_per_frame_idx];
        ms_per_frame[ms_per_frame_idx] = ImGui::GetIO().DeltaTime * 1000.0f;
        ms_per_frame_accum += ms_per_frame[ms_per_frame_idx];
        ms_per_frame_idx = (ms_per_frame_idx + 1) % 120;
        const F32 ms_per_frame_avg = ms_per_frame_accum / 120;
        g_framerateBuffer.push_back(ms_per_frame_avg);
        if (g_framerateBuffer.size() > 256) {
            g_framerateBuffer.pop_front();
        }
        g_framerateBufferCont.resize(0);
        g_framerateBufferCont.insert(std::cbegin(g_framerateBufferCont),
                                     std::cbegin(g_framerateBuffer),
                                     std::cend(g_framerateBuffer));
        ImGui::PlotHistogram("ms/frame", g_framerateBufferCont.data(), to_I32(g_framerateBufferCont.size()), 0, NULL, 0.0f, 1.0f, ImVec2(0, 80));

        ImGui::Text("%.3f ms/frame (%.1f FPS)", ms_per_frame_avg, 1000.0f / ms_per_frame_avg);

    }
};