/*
Copyright (c) 2017 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef _DIVIDE_EDITOR_PANEL_MANAGER_H_
#define _DIVIDE_EDITOR_PANEL_MANAGER_H_

#include "Platform/Headers/PlatformDefines.h"
#include "Core/Headers/PlatformContextComponent.h"

namespace Divide {
    namespace Attorney {
        class PanelManagerWidgets;
    };

    class TabbedWindow;
    class PanelManagerPane;
    class ResourceCache;
    FWD_DECLARE_MANAGED_CLASS(Texture);
    class PanelManager : public PlatformContextComponent {
        friend class Attorney::PanelManagerWidgets;

      protected:
        enum class TextureUsage : U8 {
            Tile = 0,
            Numbers,
            Dock,
            COUNT
        };

        enum class PanelPositions : U8 {
            TOP = 0,
            LEFT,
            RIGHT,
            BOTTOM,
            CENTER,
            COUNT
        };

      public:
        PanelManager(PlatformContext& context);
        ~PanelManager();

        bool saveToFile() const;
        bool loadFromFile();

        //ToDo: Move these to the Tab window manager
        bool saveTabsToFile() const;
        bool loadTabsFromFile();

        void init();
        void destroy();
        void idle();
        void draw(const U64 deltaTime);
        void resize(int w, int h);
        void drawDockedWindows(ImGui::PanelManagerWindowData& wd);
        void setPanelManagerBoundsToIncludeMainMenuIfPresent(int displayX = -1, int displayY = -1);

        inline ImGui::PanelManager& ImGuiPanelManager() { return *_manager; }

        inline bool simulationPauseRequested() const { 
            return _sceneStepCount == 0 && _simulationPaused != nullptr && *_simulationPaused;
        }


      protected:
        float calcMainMenuHeight();

      protected:
        static void drawDockedTabWindows(ImGui::PanelManagerWindowData& wd);

      protected:
        bool* _showCentralWindow;
        bool* _simulationPaused;

        U64 _deltaTime;
        U32 _sceneStepCount;
        stringImpl _saveFile;
        std::array<Texture_ptr, to_base(TextureUsage::COUNT)> _textures;
        std::unique_ptr<ImGui::PanelManager> _manager;

        std::array<std::unique_ptr<TabbedWindow>, to_base(PanelPositions::COUNT)> _tabbedWindows;
        // No center toolbar
        std::array<std::unique_ptr<PanelManagerPane>, to_base(PanelPositions::COUNT) - 1> _toolbars;

     public:
        static ResourceCache* s_globalCache;
        static hashMapImpl<U32, Texture_ptr> s_imageEditorCache;
    }; //class PanelManager


    namespace Attorney {
        class PanelManagerWidgets {
          private:
            static ImGui::PanelManager& internalManager(Divide::PanelManager& mgr) {
                return *mgr._manager;
            }

            static void drawDockedTabWindows(ImGui::PanelManagerWindowData& wd) {
                PanelManager::drawDockedTabWindows(wd);
            }

            friend class Divide::TabbedWindow;
            friend class Divide::PanelManagerPane;
        };
    };

}; //namespace Divide

#endif //_DIVIDE_EDITOR_PANEL_MANAGER_H_