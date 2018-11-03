#include "stdafx.h"

#include "Headers/ContentExplorerWindow.h"
#include "Platform/File/Headers/FileManagement.h"

#include <imgui/addons/imguifilesystem/imguifilesystem.h>

//ToDo: Replace this with either the C++17 filesystem lib or with custom stuff in PlatformDefines
#include <boost/filesystem.hpp>
using namespace boost::filesystem;

namespace Divide {
    namespace {
        char* g_extensions[] = {
            "glsl", "cmn", "frag", "vert", "cmp", "geom", "tesc", "tese",  //Shaders
            "ogg", "wav", //Sounds
            "png", "jpg", "jpeg", "tga", "raw", "dds", //Images
            "mtl", "obj", "x", "md5anim", "md5mesh", "bvh", "md2", "ase", //Models
            "chai", //Scripts
            "layout", "looknfeel", "scheme", "xsd", "imageset", "xcf", "txt", "anims",  //CEGUI
            "ttf", "font", //Fonts
            "xml" //General
        };
    };

    ContentExplorerWindow::ContentExplorerWindow(Editor& parent, const Descriptor& descriptor)
        : DockedWindow(parent, descriptor),
         _selectedDir(nullptr)
    {
    }

    ContentExplorerWindow::~ContentExplorerWindow()
    {
    }

    void ContentExplorerWindow::update() {
        _currentDirectories.resize(2);
        
        getDirectoryStructureForPath(Paths::g_assetsLocation, _currentDirectories[0]);
        _currentDirectories[0]._path = "Assets";

        getDirectoryStructureForPath(Paths::g_xmlDataLocation, _currentDirectories[1]);
        _currentDirectories[1]._path = "XML";
    }
    
    void ContentExplorerWindow::getDirectoryStructureForPath(const stringImpl& directoryPath, Directory& directoryOut) {
        path p(directoryPath);
        if (is_directory(p)) {
            directoryOut._path = p.filename().generic_string();
            for (auto&& x : directory_iterator(p)) {
                if (is_regular_file(x.path())) {
                    for (char* extension : g_extensions) {
                        if (hasExtension(x.path().generic_string(), extension)) {
                            directoryOut._files.push_back(x.path().filename().generic_string());
                            break;
                        }
                    }
                } else if (is_directory(x.path())) {
                    directoryOut._children.push_back(std::make_shared<Directory>());
                    getDirectoryStructureForPath(x.path().generic_string(), *directoryOut._children.back());
                }
            }
        }
    }

    void ContentExplorerWindow::printDirectoryStructure(const Directory& dir, bool open) const {
        ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | (open ? ImGuiTreeNodeFlags_DefaultOpen : 0);

        if (dir._children.empty()) {
            nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }

        if (ImGui::TreeNodeEx(dir._path.c_str(), nodeFlags)) {
            if (ImGui::IsItemClicked()) {
                _selectedDir = &dir;
            }

            for (const std::shared_ptr<Directory>& childDirectory : dir._children) {
                printDirectoryStructure(*childDirectory, false);
            }
       
            if (!dir._children.empty()) {
                ImGui::TreePop();
            }
        }
    }

    void ContentExplorerWindow::drawInternal() {
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);

        {
            ImGui::BeginChild("Folders", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.3f, -1), true, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar);
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("Menu")) {
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            for (const Directory& dir : _currentDirectories) {
                printDirectoryStructure(dir, true);
            }

            ImGui::EndChild();
        }
        ImGui::SameLine();
        {
            ImGui::BeginChild("Contents", ImVec2(0, -1), true, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar);
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("Menu")) {
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            ImGui::Columns(4);
            if (_selectedDir != nullptr) {
                for (const std::string& file : _selectedDir->_files) {
                    if (ImGui::Button(file.c_str())) {

                    }
                    ImGui::NextColumn();
                }
            }
            ImGui::EndChild();
        }

        ImGui::PopStyleVar();
    }

}; //namespace Divide
