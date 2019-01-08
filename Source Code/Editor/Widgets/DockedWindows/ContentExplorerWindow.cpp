#include "stdafx.h"

#include "Headers/ContentExplorerWindow.h"

#include "Editor/Headers/Editor.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/PlatformContext.h"
#include "Platform/File/Headers/FileManagement.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include <imgui/addons/imguifilesystem/imguifilesystem.h>

//ToDo: Replace this with either the C++17 filesystem lib or with custom stuff in PlatformDefines
#include <boost/filesystem.hpp>
using namespace boost::filesystem;

namespace Divide {
    namespace {
        constexpr size_t MAX_TEXTURE_LOADS_PER_FRAME = 1;

        const char* const g_extensions[] = {
            "glsl", "cmn", "frag", "vert", "cmp", "geom", "tesc", "tese",  //Shaders
            "ogg", "wav", //Sounds
            "png", "jpg", "jpeg", "tga", "raw", "dds", //Images
            "mtl", "obj", "x", "md5anim", "md5mesh", "bvh", "md2", "ase", //Models
            "chai", //Scripts
            "layout", "looknfeel", "scheme", "xsd", "imageset", "xcf", "txt", "anims",  //CEGUI
            "ttf", "font", //Fonts
            "xml" //General
        };

        const char* const g_imageExtensions[] = {
            "png", "jpg", "jpeg", "tga", "raw", "dds"
        };
    };

    ContentExplorerWindow::ContentExplorerWindow(Editor& parent, const Descriptor& descriptor)
        : DockedWindow(parent, descriptor),
         _selectedDir(nullptr)
    {
    }

    ContentExplorerWindow::~ContentExplorerWindow()
    {
        _loadedTextures.clear();
    }

    void ContentExplorerWindow::init() {
        _currentDirectories.resize(2);

        getDirectoryStructureForPath(Paths::g_assetsLocation, _currentDirectories[0]);
        _currentDirectories[0]._path = "Assets";

        getDirectoryStructureForPath(Paths::g_xmlDataLocation, _currentDirectories[1]);
        _currentDirectories[1]._path = "XML";
    }

    void ContentExplorerWindow::update(const U64 deltaTimeUS) {
        ACKNOWLEDGE_UNUSED(deltaTimeUS);

        if (!_textureLoadQueue.empty()) {
            auto file = _textureLoadQueue.top();
            _textureLoadQueue.pop();
            _loadedTextures[_ID((file.first + "/" + file.second).c_str())] = getTextureForPath(file.first, file.second);
        }
    }
    
    void ContentExplorerWindow::getDirectoryStructureForPath(const stringImpl& directoryPath, Directory& directoryOut) {
        path p(directoryPath);
        if (is_directory(p)) {
            directoryOut._path = p.filename().generic_string();
            for (auto&& x : directory_iterator(p)) {
                if (is_regular_file(x.path())) {
                    for (const char* extension : g_extensions) {
                        if (hasExtension(x.path().generic_string(), extension)) {
                            directoryOut._files.push_back({directoryOut._path, x.path().filename().generic_string()});
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
        static Texture_ptr previewTexture = nullptr;

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
                for (auto file : _selectedDir->_files) {
                    Texture_ptr tex = nullptr;
                    for (const char* extension : g_imageExtensions) {
                        if (hasExtension(file.second, extension)) {
                            auto it = _loadedTextures.find(_ID((file.first + "/" + file.second).c_str()));
                            if (it == std::cend(_loadedTextures) || it->second == nullptr) {
                                if (_textureLoadQueue.empty()) {
                                    _textureLoadQueue.push(file);
                                }
                            } else if (it->second->getState() == ResourceState::RES_LOADED) {
                                tex = it->second;
                            }
                            break;
                        }
                    }

                    if (tex != nullptr) {
                        U16 w = tex->getWidth();
                        U16 h = tex->getHeight();
                        F32 aspect = w / to_F32(h);

                        if (ImGui::ImageButton((void *)(intptr_t)tex->getData().getHandle(), ImVec2(64, 64 / aspect))) {
                            previewTexture = tex;
                            ImGui::OpenPopup("Image Preview");
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip(file.second.c_str());
                        }
                        ImGui::Text(file.second.c_str());
                    } else {
                        if (ImGui::Button(file.second.c_str())) {
                            if (ImGui::IsItemHovered()) {
                                ImGui::SetTooltip("Image still loading!");
                            }
                        }
                    }
                    ImGui::NextColumn();
                }
                if (Attorney::EditorGeneralWidget::modalTextureView(_parent, "Image Preview", previewTexture, vec2<F32>(512, 512), true)) {
                    previewTexture = nullptr;
                }
                
            }
            ImGui::EndChild();
        }

        ImGui::PopStyleVar();
    }

    Texture_ptr ContentExplorerWindow::getTextureForPath(const stringImpl& texturePath, const stringImpl& textureName) {
        SamplerDescriptor texturePreviewSampler = {};
        texturePreviewSampler._wrapU = TextureWrap::CLAMP;
        texturePreviewSampler._wrapV = TextureWrap::CLAMP;
        texturePreviewSampler._wrapW = TextureWrap::CLAMP;
        texturePreviewSampler._minFilter = TextureFilter::NEAREST;
        texturePreviewSampler._magFilter = TextureFilter::NEAREST;
        texturePreviewSampler._anisotropyLevel = 0;

        TextureDescriptor texturePreviewDescriptor(TextureType::TEXTURE_2D);
        texturePreviewDescriptor.setSampler(texturePreviewSampler);

        ResourceDescriptor textureResource(textureName);
        textureResource.setThreadedLoading(true);
        textureResource.setFlag(true);
        textureResource.assetName(textureName);
        textureResource.assetLocation(Paths::g_assetsLocation + "/" + texturePath);
        textureResource.setPropertyDescriptor(texturePreviewDescriptor);

        return CreateResource<Texture>(_parent.context().kernel().resourceCache(), textureResource);
    }
}; //namespace Divide
