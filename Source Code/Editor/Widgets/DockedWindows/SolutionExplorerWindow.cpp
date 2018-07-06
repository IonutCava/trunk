#include "stdafx.h"

#include "Headers/SolutionExplorerWindow.h"

namespace Divide {
    SolutionExplorerWindow::SolutionExplorerWindow(PanelManager& parent)
        : DockedWindow(parent, "Solution Explorer")
    {

    }

    SolutionExplorerWindow::~SolutionExplorerWindow()
    {

    }

    void SolutionExplorerWindow::draw() {
        // Draw Solution Explorer
        ImGui::Text("%s\n", _name.c_str());
        static float f;
        ImGui::Text("Hello, world!");
        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        //show_test_window ^= ImGui::Button("Test Window");
        //show_another_window ^= ImGui::Button("Another Window");

        // Calculate and show framerate
        static float ms_per_frame[120] = { 0 };
        static int ms_per_frame_idx = 0;
        static float ms_per_frame_accum = 0.0f;
        ms_per_frame_accum -= ms_per_frame[ms_per_frame_idx];
        ms_per_frame[ms_per_frame_idx] = ImGui::GetIO().DeltaTime * 1000.0f;
        ms_per_frame_accum += ms_per_frame[ms_per_frame_idx];
        ms_per_frame_idx = (ms_per_frame_idx + 1) % 120;
        const float ms_per_frame_avg = ms_per_frame_accum / 120;
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", ms_per_frame_avg, 1000.0f / ms_per_frame_avg);
    }
};