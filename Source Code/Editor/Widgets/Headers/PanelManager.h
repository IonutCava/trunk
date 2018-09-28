/*
Copyright (c) 2018 DIVIDE-Studio
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
        class PanelManagerDockedWindows;
    };

    class Camera;
    class DockedWindow;
    class TabbedWindow;
    class ResourceCache;
    class PropertyWindow;
    class PreferencesWindow;
    class SolutionExplorerWindow;

    struct TransformSettings;

    FWD_DECLARE_MANAGED_CLASS(Texture);
    class PanelManager : public PlatformContextComponent {
        friend class Attorney::PanelManagerWidgets;
        friend class Attorney::PanelManagerDockedWindows;

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
          static const U8 ButtonWidth = 8;
          static const U8 ButtonHeight = 10;

      public:
        PanelManager(PlatformContext& context);
        ~PanelManager();

        bool saveToFile() const;
        bool loadFromFile();

        //ToDo: Move these to the Tab window manager
        bool saveTabsToFile() const;
        bool loadTabsFromFile();

        void init(const vec2<U16>& renderResolution);
        void destroy();
        void idle();
        void draw(const U64 deltaTime);
        void resize(int w, int h);
        void setPanelManagerBoundsToIncludeMainMenuIfPresent(int displayX = -1, int displayY = -1);

        inline bool simulationPauseRequested() const { 
            return _sceneStepCount == 0 && _simulationPaused != nullptr && *_simulationPaused;
        }

        void setTransformSettings(const TransformSettings& settings);
        const TransformSettings& getTransformSettings() const;

      protected:
        F32 calcMainMenuHeight();
        void drawDockedTabWindows(ImGui::PanelManagerWindowData& wd);
        void setSelectedCamera(Camera* camera);
        Camera* getSelectedCamera() const;

      protected:
        bool* _showCentralWindow;
        bool* _simulationPaused;

        U64 _deltaTime;
        U32 _sceneStepCount;
        stringImpl _saveFile;
        Camera* _selectedCamera;
        vector<DockedWindow*> _dockedWindows;
        std::array<Texture_ptr, to_base(TextureUsage::COUNT)> _textures;
     public:

        static ResourceCache* s_globalCache;
        static hashMap<U32, Texture_ptr> s_imageEditorCache;
    }; //class PanelManager


    namespace Attorney {
        class PanelManagerDockedWindows {
            private:
              static bool* showCentralWindow(Divide::PanelManager& mgr) {
                  return mgr._showCentralWindow;
              }

              static void setSelectedCamera(Divide::PanelManager& mgr, Camera* camera) {
                  mgr.setSelectedCamera(camera);
              }

              static Camera* getSelectedCamera(Divide::PanelManager& mgr) {
                  return mgr.getSelectedCamera();
              }

              static bool editorEnableGizmo(Divide::PanelManager& mgr);

              static void editorEnableGizmo(Divide::PanelManager& mgr, bool state);

              friend class Divide::PropertyWindow;
              friend class Divide::PreferencesWindow;
              friend class Divide::SolutionExplorerWindow;
        };
    };

}; //namespace Divide

#endif //_DIVIDE_EDITOR_PANEL_MANAGER_H_