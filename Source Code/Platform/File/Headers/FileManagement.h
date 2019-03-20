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

#pragma once
#ifndef _PLATFORM_FILE_FILE_MANAGEMENT_H_
#define _PLATFORM_FILE_FILE_MANAGEMENT_H_

#include "FileWithPath.h"

namespace Divide {

enum class CacheType : U8 {
    SHADER_TEXT = 0,
    SHADER_BIN,
    TERRAIN,
    MODELS,
    TEXTURES,
    COUNT
};

struct SysInfo;
class PlatformContext;
namespace Paths {
    extern stringImpl g_exePath;
    extern stringImpl g_logPath;
    extern stringImpl g_assetsLocation;
    extern stringImpl g_shadersLocation;
    extern stringImpl g_texturesLocation;
    extern stringImpl g_heightmapLocation;
    extern stringImpl g_imagesLocation;
    extern stringImpl g_materialsLocation;
    extern stringImpl g_soundsLocation;
    extern stringImpl g_xmlDataLocation;
    extern stringImpl g_navMeshesLocation;
    extern stringImpl g_scenesLocation;
    extern stringImpl g_saveLocation;
    extern stringImpl g_GUILocation;
    extern stringImpl g_fontsPath;
    extern stringImpl g_localisationPath;
    extern stringImpl g_cacheLocation;
    extern stringImpl g_terrainCacheLocation;
    extern stringImpl g_geometryCacheLocation;

    namespace Editor {
        extern stringImpl g_saveLocation;
        extern stringImpl g_tabLayoutFile;
        extern stringImpl g_panelLayoutFile;
    };

    namespace Scripts {
        extern stringImpl g_scriptsLocation;
        extern stringImpl g_scriptsAtomsLocation;
    };

    namespace Textures {
        extern stringImpl g_metadataLocation;
    };

    namespace Shaders {
        extern stringImpl g_cacheLocation;
        extern stringImpl g_cacheLocationText;
        extern stringImpl g_cacheLocationBin;

        namespace GLSL {
            // these must match the last 4 characters of the atom file
            extern stringImpl g_fragAtomExt;
            extern stringImpl g_vertAtomExt;
            extern stringImpl g_geomAtomExt;
            extern stringImpl g_tescAtomExt;
            extern stringImpl g_teseAtomExt;
            extern stringImpl g_compAtomExt;
            extern stringImpl g_comnAtomExt;

            // Shader subfolder name that contains shader files for OpenGL
            extern stringImpl g_parentShaderLoc;
            // Atom folder names in parent shader folder
            extern stringImpl g_fragAtomLoc;
            extern stringImpl g_vertAtomLoc;
            extern stringImpl g_geomAtomLoc;
            extern stringImpl g_tescAtomLoc;
            extern stringImpl g_teseAtomLoc;
            extern stringImpl g_compAtomLoc;
            extern stringImpl g_comnAtomLoc;
        }; //namespace GLSL
        namespace HLSL {
            extern stringImpl g_parentShaderLoc;
        }; //namespace HLSL
    }; //namespace Shaders

    // include command regex pattern
    extern std::regex g_includePattern;
    // use command regex pattern
    extern std::regex g_usePattern;

    
    void initPaths(const SysInfo& info);
    void updatePaths(const PlatformContext& context);
}; //namespace Paths


bool pathExists(const char* filePath);
bool fileExists(const char* filePathAndName);
bool createFile(const char* filePathAndName, bool overwriteExisting);
bool deleteAllFiles(const char* filePath, const char* extension = nullptr);

template<typename T /*requirement: has_assign<T> == true*/>
bool readFile(const stringImpl& filePath, const stringImpl& fileName, T& contentOut, FileType fileType);
bool writeFile(const stringImpl& filePath, const stringImpl& fileName, const bufferPtr content, size_t length, FileType fileType);
bool deleteFile(const stringImpl& filePath, const stringImpl& fileName);
bool copyFile(const stringImpl& sourcePath, const stringImpl& sourceName, const stringImpl& targetPath, const stringImpl& targetName, bool overwrite);

/// will add '.' automatically at the start of 'extension'
bool hasExtension(const stringImpl& filePath, const stringImpl& extension);

FileWithPath splitPathToNameAndLocation(const stringImpl& input);

bool clearCache();
bool clearCache(CacheType type);

std::string extractFilePathAndName(char* argv0);

}; //namespace Divide

#endif //_PLATFORM_FILE_FILE_MANAGEMENT_H_

#include "FileManagement.inl"
