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

#ifndef _EDITOR_CONTENT_EXPLORER_WINDOW_H_
#define _EDITOR_CONTENT_EXPLORER_WINDOW_H_

#include "Editor/Widgets/Headers/DockedWindow.h"

namespace Divide {
    FWD_DECLARE_MANAGED_CLASS(Texture);
    FWD_DECLARE_MANAGED_CLASS(Mesh);

    struct Directory {
        stringImpl _path;
        vectorSTDFast<std::pair<Str256, Str64>> _files;
        vectorSTDFast<std::shared_ptr<Directory>> _children;
    };

    class ContentExplorerWindow : public DockedWindow {
    public:
        enum class GeometryFormat : U8 {
            _3DS = 0, //Studio max format
            ASE, //ASCII Scene Export. Old Unreal format
            FBX,
            MD2,
            MD5,
            OBJ,
            X, //DirectX foramt
            COUNT
        };

        ContentExplorerWindow(Editor& parent, const Descriptor& descriptor);
        ~ContentExplorerWindow();

        void drawInternal() override;
        void init();
        void update(const U64 deltaTimeUS);

    private:
        void getDirectoryStructureForPath(const Str256& directoryPath, Directory& directoryOut);
        void printDirectoryStructure(const Directory& dir, bool open) const;

        Texture_ptr getTextureForPath(const Str256& texturePath, const Str64& textureName);
        Mesh_ptr getModelForPath(const Str256& modelPath, const Str64& modelName);
        
    private:
        Texture_ptr _fileIcon = nullptr;
        std::array<Texture_ptr, to_base(GeometryFormat::COUNT)> _geometryIcons;
        mutable const Directory* _selectedDir = nullptr;
        vectorSTDFast<Directory> _currentDirectories;

        hashMap<size_t, Texture_ptr> _loadedTextures;
        hashMap<size_t, Mesh_ptr> _loadedModels;

        bool _textureLoadQueueLocked = false;
        std::stack<std::pair<Str256, Str64>> _textureLoadQueue;

        bool _modelLoadQueueLocked = false;
        std::stack<std::pair<Str256, Str64>> _modelLoadQueue;
    };
}; //namespace Divide

#endif //_EDITOR_CONTENT_EXPLORER_WINDOW_H_