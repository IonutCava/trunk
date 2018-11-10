#include "stdafx.h"

#include "Headers/SolutionExplorerWindow.h"

#include "Editor/Headers/Editor.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/PlatformContext.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Dynamics/Entities/Units/Headers/Player.h"

namespace Divide {
    namespace {
        constexpr U8 g_maxEntryCount = 32;
        std::deque<F32> g_framerateBuffer;
        std::vector<F32> g_framerateBufferCont;
    };

    SolutionExplorerWindow::SolutionExplorerWindow(Editor& parent, PlatformContext& context, const Descriptor& descriptor)
        : DockedWindow(parent, descriptor),
          PlatformContextComponent(context)
    {
        g_framerateBufferCont.reserve(g_maxEntryCount);
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
                Attorney::EditorSolutionExplorerWindow::setSelectedCamera(_parent, camera);
            }

            ImGui::TreePop();
        }
    }

    void SolutionExplorerWindow::printSceneGraphNode(SceneManager& sceneManager, SceneGraphNode& sgn, bool open) {
        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | (open ? ImGuiTreeNodeFlags_DefaultOpen : 0);

        if (sgn.getSelectionFlag() == SceneGraphNode::SelectionFlag::SELECTION_SELECTED) {
            node_flags |= ImGuiTreeNodeFlags_Selected;
        }

        if (!sgn.hasChildren()) {
            node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;//| ImGuiTreeNodeFlags_NoTreePushOnOpen; 
        }
        if (ImGui::TreeNodeEx((void*)(intptr_t)sgn.getGUID(), node_flags, sgn.name().c_str())) {
            if (ImGui::IsItemClicked()) {
                sceneManager.resetSelection(0);
                sceneManager.setSelected(0, sgn);
                Attorney::EditorSolutionExplorerWindow::setSelectedCamera(_parent, nullptr);
            }
        
            sgn.forEachChild([this, &sceneManager](SceneGraphNode& child) {
                printSceneGraphNode(sceneManager, child, false);
            });

            ImGui::TreePop();
        }
        
        
    }

    void SolutionExplorerWindow::drawInternal() {
        SceneManager& sceneManager = context().kernel().sceneManager();
        Scene& activeScene = sceneManager.getActiveScene();
        SceneGraphNode& root = activeScene.sceneGraph().getRoot();

        ImGui::PushItemWidth(200);
        ImGui::BeginChild("SceneGraph", ImVec2(ImGui::GetWindowContentRegionWidth(), ImGui::GetWindowHeight() * .5f), true, 0);
        if (ImGui::TreeNodeEx(activeScene.name().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, ImGui::GetFontSize() * 3); // Increase spacing to differentiate leaves from expanded contents.
            for (PlayerIndex i = 0; i < static_cast<PlayerIndex>(Config::MAX_LOCAL_PLAYER_COUNT); ++i) {
                printCameraNode(sceneManager, Attorney::SceneManagerCameraAccessor::playerCamera(sceneManager, i));
            }
            printSceneGraphNode(sceneManager, root, true);
            ImGui::PopStyleVar();
            ImGui::TreePop();
        }

        ImGui::Separator();
        ImGui::NewLine();
        ImGui::NewLine();
        ImGui::Text("All scenes");
        ImGui::Separator();

        const vector<stringImpl>& scenes = sceneManager.sceneNameList();
        for (const stringImpl& scene : scenes) {
            if (scene != activeScene.name()) {
                if (ImGui::TreeNodeEx(scene.c_str(), ImGuiTreeNodeFlags_Leaf)) {
                    ImGui::TreePop();
                }
            }
        }

        ImGui::EndChild();
        ImGui::PopItemWidth();
        ImGui::Separator();

        drawTransformSettings();

        ImGui::Separator();

        // Calculate and show framerate
        static F32 max_ms_per_frame = 0;

        static F32 ms_per_frame[g_maxEntryCount] = { 0 };
        static I32 ms_per_frame_idx = 0;
        static F32 ms_per_frame_accum = 0.0f;
        ms_per_frame_accum -= ms_per_frame[ms_per_frame_idx];
        ms_per_frame[ms_per_frame_idx] = ImGui::GetIO().DeltaTime * 1000.0f;
        ms_per_frame_accum += ms_per_frame[ms_per_frame_idx];
        ms_per_frame_idx = (ms_per_frame_idx + 1) % g_maxEntryCount;
        const F32 ms_per_frame_avg = ms_per_frame_accum / g_maxEntryCount;
        if (ms_per_frame_avg + (Config::TARGET_FRAME_RATE / 1000.0f) > max_ms_per_frame) {
            max_ms_per_frame = ms_per_frame_avg + (Config::TARGET_FRAME_RATE / 1000.0f);
        }

        // We need this bit to get a nice "flowing" feel
        g_framerateBuffer.push_back(ms_per_frame_avg);
        if (g_framerateBuffer.size() > g_maxEntryCount) {
            g_framerateBuffer.pop_front();
        }
        g_framerateBufferCont.resize(0);
        g_framerateBufferCont.insert(std::cbegin(g_framerateBufferCont),
                                     std::cbegin(g_framerateBuffer),
                                     std::cend(g_framerateBuffer));
        ImGui::PlotHistogram("",
                             g_framerateBufferCont.data(),
                             to_I32(g_framerateBufferCont.size()),
                             0,
                             Util::StringFormat("%.3f ms/frame (%.1f FPS)", ms_per_frame_avg, ms_per_frame_avg > 0.01f ? 1000.0f / ms_per_frame_avg : 0.0f).c_str(),
                             0.0f,
                             max_ms_per_frame,
                             ImVec2(0, 50));
    }

    void SolutionExplorerWindow::drawTransformSettings() {
         bool enableGizmo = Attorney::EditorSolutionExplorerWindow::editorEnableGizmo(_parent);
         ImGui::Checkbox("Transform Gizmo", &enableGizmo);
         Attorney::EditorSolutionExplorerWindow::editorEnableGizmo(_parent, enableGizmo);
     }
};