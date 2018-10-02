#include "stdafx.h"

#include "Headers/PanelManager.h"
#include "Headers/DockedWindow.h"

#include "DockedWindows/Headers/OutputWindow.h"
#include "DockedWindows/Headers/PropertyWindow.h"
#include "DockedWindows/Headers/SceneViewWindow.h"
#include "DockedWindows/Headers/SolutionExplorerWindow.h"

#include "Editor/Headers/Editor.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include <imgui_internal.h>

#ifndef STBI_INCLUDE_STB_IMAGE_H
#ifndef IMGUI_NO_STB_IMAGE_STATIC
#define STB_IMAGE_STATIC
#endif //IMGUI_NO_STB_IMAGE_STATIC
#ifndef IMGUI_NO_STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif //IMGUI_NO_STB_IMAGE_IMPLEMENTATION
#include "imgui/addons/imguibindings/stb_image.h"
#ifndef IMGUI_NO_STB_IMAGE_IMPLEMENTATION
#undef STB_IMAGE_IMPLEMENTATION
#endif //IMGUI_NO_STB_IMAGE_IMPLEMENTATION
#ifndef IMGUI_NO_STB_IMAGE_STATIC
#undef STB_IMAGE_STATIC
#endif //IMGUI_NO_STB_IMAGE_STATIC
#endif //STBI_INCLUDE_STB_IMAGE_H

namespace Divide {

    ResourceCache* PanelManager::s_globalCache = nullptr;
    hashMap<U32, Texture_ptr> PanelManager::s_imageEditorCache;

    namespace {
        static const ImVec2 buttonSize(PanelManager::ButtonWidth, PanelManager::ButtonHeight);
        static const ImVec2 buttonSizeSq(PanelManager::ButtonHeight, PanelManager::ButtonHeight);

         ImTextureID LoadTextureFromMemory(Texture_ptr& target, const stringImpl& name, int width, int height, int channels, const unsigned char* pixels, bool useMipmapsIfPossible=false, bool wraps=true, bool wrapt=true, bool minFilterNearest=false, bool magFilterNearest=false) {
        
            CLAMP(channels, 1, 4);

            ImTextureID texId = NULL;

            SamplerDescriptor sampler = {};
            sampler._minFilter = minFilterNearest ? (useMipmapsIfPossible ? TextureFilter::NEAREST_MIPMAP_NEAREST : TextureFilter::NEAREST)
                                                  : (useMipmapsIfPossible ? TextureFilter::LINEAR_MIPMAP_LINEAR : TextureFilter::LINEAR);

            sampler._magFilter = magFilterNearest ? TextureFilter::NEAREST : TextureFilter::LINEAR;
            sampler._wrapU = wraps ? TextureWrap::REPEAT : TextureWrap::CLAMP;
            sampler._wrapV = wrapt ? TextureWrap::REPEAT : TextureWrap::CLAMP;

            TextureDescriptor descriptor(TextureType::TEXTURE_2D,
                                         channels > 3 ? GFXImageFormat::RGBA8 :
                                                        channels > 2 ? GFXImageFormat::RGB8 :
                                                                       channels > 1 ? GFXImageFormat::RG8 :
                                                                                      GFXImageFormat::RED,
                                         GFXDataFormat::UNSIGNED_BYTE);
            descriptor.setSampler(sampler);
            descriptor.automaticMipMapGeneration(useMipmapsIfPossible);

            ResourceDescriptor textureDescriptor(name);

            textureDescriptor.setThreadedLoading(false);
            textureDescriptor.setFlag(true);
            textureDescriptor.setPropertyDescriptor(descriptor);

            target = CreateResource<Texture>(*PanelManager::s_globalCache, textureDescriptor);
            assert(target);

            Texture::TextureLoadInfo info;

            target->loadData(info, (bufferPtr)pixels, vec2<U16>(width, height));

            texId = reinterpret_cast<void*>((intptr_t)target->getHandle());

            return texId;
        }

        ImTextureID LoadTextureFromMemory(Texture_ptr& target, const stringImpl& name, const unsigned char* filenameInMemory, int filenameInMemorySize, int req_comp = 0) {
            int w, h, n;
            unsigned char* pixels = stbi_load_from_memory(filenameInMemory, filenameInMemorySize, &w, &h, &n, req_comp);
            if (!pixels) {
                return 0;
            }
            ImTextureID ret = LoadTextureFromMemory(target, name, w, h, req_comp > 0 ? req_comp : n, pixels, false, true, true, false, false);

            stbi_image_free(pixels);

            return ret;
        }
        void FreeTextureDelegate(ImTextureID& texid) {
            hashMap<U32, Texture_ptr>::iterator it = PanelManager::s_imageEditorCache.find((U32)(intptr_t)texid);
            if (it != std::cend(PanelManager::s_imageEditorCache)) {
                PanelManager::s_imageEditorCache.erase(it);
            } else {
                // error
            }
        }

        void GenerateOrUpdateTexture(ImTextureID& imtexid, int width, int height, int channels, const unsigned char* pixels, bool useMipmapsIfPossible, bool wraps, bool wrapt, bool minFilterNearest, bool magFilterNearest) {
            Texture_ptr tex = nullptr;
            LoadTextureFromMemory(tex,
                                  Util::StringFormat("temp_edit_tex_%d", PanelManager::s_imageEditorCache.size()),
                                  width,
                                  height,
                                  channels,
                                  pixels,
                                  useMipmapsIfPossible,
                                  wraps,
                                  wrapt,
                                  minFilterNearest,
                                  magFilterNearest);
            if (tex != nullptr) {
                imtexid = reinterpret_cast<void*>((intptr_t)tex->getHandle());
                PanelManager::s_imageEditorCache[tex->getHandle()] = tex;
            }
        }
    };

    PanelManager::PanelManager(PlatformContext& context)
        : PlatformContextComponent(context),
          _deltaTime(0ULL),
          _sceneStepCount(0),
          _simulationPaused(nullptr),
          _showCentralWindow(nullptr),
          _selectedCamera(nullptr),
          _saveFile(Paths::g_saveLocation + Paths::Editor::g_saveLocation + Paths::Editor::g_panelLayoutFile)
    {
        MemoryManager::DELETE_VECTOR(_dockedWindows);
        _dockedWindows.push_back(MemoryManager_NEW SolutionExplorerWindow(*this, context));
        _dockedWindows.push_back(MemoryManager_NEW PropertyWindow(*this, context));
        _dockedWindows.push_back(MemoryManager_NEW OutputWindow(*this));
        _dockedWindows.push_back(MemoryManager_NEW SceneViewWindow(*this));
    }

    PanelManager::~PanelManager()
    {
        s_imageEditorCache.clear();
        MemoryManager::DELETE_VECTOR(_dockedWindows);
    }

    bool PanelManager::saveToFile() const {
        return true;
    }

    bool PanelManager::loadFromFile() {
        return true;
    }

    bool PanelManager::saveTabsToFile() const {
        return true;
    }

    bool PanelManager::loadTabsFromFile() {
        return true;
    }

    void PanelManager::idle() {
        if (_sceneStepCount > 0) {
            _sceneStepCount--;
        }

        /*ImGui::PanelManager::Pane& pane = _toolbars[to_base(PanelPositions::TOP)]->impl();
        ImGui::Toolbar::Button* btn1 = pane.bar.getButton(pane.getSize() - 2);
        if (btn1->isDown) {                          // [***]
            _sceneStepCount = 1;
            btn1->isDown = false;
        }
        ImGui::Toolbar::Button* btn2 = pane.bar.getButton(pane.getSize() - 1);
        if (btn2->isDown) {                          // [****]
            _sceneStepCount = Config::TARGET_FRAME_RATE;
            btn2->isDown = false;
        }*/
    }


    void PanelManager::draw(const U64 deltaTime) {
        _deltaTime = deltaTime;

        ImGuiWindowFlags window_flags = 0;
        //window_flags |= ImGuiWindowFlags_NoTitleBar;
        //window_flags |= ImGuiWindowFlags_NoScrollbar;
        //window_flags |= ImGuiWindowFlags_MenuBar;
        //window_flags |= ImGuiWindowFlags_NoMove;
        //window_flags |= ImGuiWindowFlags_NoResize;
        //window_flags |= ImGuiWindowFlags_NoCollapse;
        //window_flags |= ImGuiWindowFlags_NoNav;

        const float menuBarOffset = 20;

        ImVec2 sizes[] = {
            ImVec2(300, 550),
            ImVec2(300, 550),
            ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y - 550 - menuBarOffset - 3),
            ImVec2(640, 480)
        };

        ImVec2 positions[] = {
            ImVec2(0, menuBarOffset),
            ImVec2(ImGui::GetIO().DisplaySize.x - sizes[1].x, menuBarOffset),
            ImVec2(0, std::max(sizes[0].y, sizes[1].y) + menuBarOffset + 3),
            ImVec2(150, 150)
        };

        U32 i = 0;
        for (DockedWindow* window : _dockedWindows) {
            ImGui::SetNextWindowPos(positions[i], ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(sizes[i++], ImGuiCond_FirstUseEver);

            if (ImGui::Begin(window->name(), NULL, window_flags)) {
                window->draw();
            }
            ImGui::End();
        }
    }

    void PanelManager::init(const vec2<U16>& renderResolution) {
        // double init check
        if (s_globalCache != nullptr) {
            return;
        }

        s_globalCache = &context().kernel().resourceCache();

#   ifdef YES_IMGUIIMAGEEDITOR
        ImGui::ImageEditor::SetGenerateOrUpdateTextureCallback(&GenerateOrUpdateTexture);   // This will be called only with channels=3 or channels=4
        ImGui::ImageEditor::SetFreeTextureCallback(&FreeTextureDelegate);
#   endif
        SamplerDescriptor sampler = {};
        sampler._wrapU = TextureWrap::CLAMP;
        sampler._wrapV = TextureWrap::CLAMP;
        sampler._wrapW = TextureWrap::CLAMP;
        sampler._minFilter = TextureFilter::NEAREST;
        sampler._magFilter = TextureFilter::NEAREST;
        sampler._srgb = false;
        sampler._anisotropyLevel = 0;

        TextureDescriptor descriptor(TextureType::TEXTURE_2D);
        descriptor.setSampler(sampler);

        ResourceDescriptor texture("Panel Manager Texture 1");

        texture.setThreadedLoading(false);
        texture.setFlag(true);
        texture.setResourceName("Tile8x8.png");
        texture.setResourceLocation(Paths::g_assetsLocation + Paths::g_GUILocation);
        texture.setPropertyDescriptor(descriptor);

        _textures[to_base(TextureUsage::Tile)] = CreateResource<Texture>(*s_globalCache, texture);

        texture.name("Panel Manager Texture 2");
        texture.setResourceName("myNumbersTexture.png");
        _textures[to_base(TextureUsage::Numbers)] = CreateResource<Texture>(*s_globalCache, texture);

        // Optional. Loads the layout (just selectedButtons, docked windows sizes and stuff like that)
        loadFromFile();

        // These line is only necessary to accommodate space for the global menu bar we're using:
        setPanelManagerBoundsToIncludeMainMenuIfPresent();

        resize(renderResolution.width, renderResolution.height);
    }

    void PanelManager::destroy() {

    }


    // Here are two static methods useful to handle the change of size of the togglable mainMenu we will use
    // Returns the height of the main menu based on the current font (from: ImGui::CalcMainMenuHeight() in imguihelper.h)
    float PanelManager::calcMainMenuHeight() {
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        ImFont* font = ImGui::GetFont();
        if (!font) {
            if (io.Fonts->Fonts.size()>0) font = io.Fonts->Fonts[0];
            else return (14) + style.FramePadding.y * 2.0f;
        }
        return (io.FontGlobalScale * font->Scale * font->FontSize) + style.FramePadding.y * 2.0f;
    }

    void  PanelManager::setPanelManagerBoundsToIncludeMainMenuIfPresent(int displayX, int displayY) {
        if (displayX <= 0) 
            displayX = (int)ImGui::GetIO().DisplaySize.x;

        if (displayY <= 0) 
            displayY = (int)ImGui::GetIO().DisplaySize.y;

        ImVec4 bounds(0, 0, (float)displayX, (float)displayY);   // (0,0,-1,-1) defaults to (0,0,io.DisplaySize.x,io.DisplaySize.y)
        const float mainMenuHeight = calcMainMenuHeight();
        bounds = ImVec4(0, mainMenuHeight, to_F32(displayX), to_F32(displayY) - mainMenuHeight);
        
    }

    void PanelManager::resize(int w, int h) {
        static ImVec2 initialSize((float)w, (float)h);
        setPanelManagerBoundsToIncludeMainMenuIfPresent(w, h);   // This line is only necessary if we have a global menu bar
    }

    void PanelManager::setSelectedCamera(Camera* camera) {
        _selectedCamera = camera;
    }

    Camera* PanelManager::getSelectedCamera() const {
        return _selectedCamera;
    }


    void PanelManager::setTransformSettings(const TransformSettings& settings) {
        Attorney::EditorPanelManager::setTransformSettings(context().editor(), settings);
    }

    const TransformSettings& PanelManager::getTransformSettings() const {
        return Attorney::EditorPanelManager::getTransformSettings(context().editor());
    }

    bool Attorney::PanelManagerDockedWindows::editorEnableGizmo(Divide::PanelManager& mgr) {
        return Attorney::EditorPanelManager::enableGizmo(mgr.context().editor());
    }

    void Attorney::PanelManagerDockedWindows::editorEnableGizmo(Divide::PanelManager& mgr, bool state) {
        Attorney::EditorPanelManager::enableGizmo(mgr.context().editor(), state);
    }
}; //namespace Divide