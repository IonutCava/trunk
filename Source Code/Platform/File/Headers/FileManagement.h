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
    extern Str256 g_exePath;
    extern Str256 g_logPath;
    extern Str256 g_assetsLocation;
    extern Str256 g_shadersLocation;
    extern Str256 g_texturesLocation;
    extern Str256 g_heightmapLocation;
    extern Str256 g_climatesLowResLocation;
    extern Str256 g_climatesMedResLocation;
    extern Str256 g_climatesHighResLocation;
    extern Str256 g_imagesLocation;
    extern Str256 g_materialsLocation;
    extern Str256 g_soundsLocation;
    extern Str256 g_xmlDataLocation;
    extern Str256 g_navMeshesLocation;
    extern Str256 g_scenesLocation;
    extern Str256 g_saveLocation;
    extern Str256 g_GUILocation;
    extern Str256 g_fontsPath;
    extern Str256 g_localisationPath;
    extern Str256 g_cacheLocation;
    extern Str256 g_terrainCacheLocation;
    extern Str256 g_geometryCacheLocation;

    namespace Editor {
        extern Str256 g_saveLocation;
        extern Str256 g_tabLayoutFile;
        extern Str256 g_panelLayoutFile;
    };

    namespace Scripts {
        extern Str256 g_scriptsLocation;
        extern Str256 g_scriptsAtomsLocation;
    };

    namespace Textures {
        extern Str256 g_metadataLocation;
    };

    namespace Shaders {
        extern Str256 g_cacheLocation;
        extern Str256 g_cacheLocationText;
        extern Str256 g_cacheLocationBin;

        namespace GLSL {
            // these must match the last 4 characters of the atom file
            extern Str8 g_fragAtomExt;
            extern Str8 g_vertAtomExt;
            extern Str8 g_geomAtomExt;
            extern Str8 g_tescAtomExt;
            extern Str8 g_teseAtomExt;
            extern Str8 g_compAtomExt;
            extern Str8 g_comnAtomExt;

            // Shader subfolder name that contains shader files for OpenGL
            extern Str256 g_parentShaderLoc;
            // Atom folder names in parent shader folder
            extern Str256 g_fragAtomLoc;
            extern Str256 g_vertAtomLoc;
            extern Str256 g_geomAtomLoc;
            extern Str256 g_tescAtomLoc;
            extern Str256 g_teseAtomLoc;
            extern Str256 g_compAtomLoc;
            extern Str256 g_comnAtomLoc;
        }; //namespace GLSL
        namespace HLSL {
            extern Str256 g_parentShaderLoc;
        }; //namespace HLSL
    }; //namespace Shaders

    // include command regex pattern
    extern std::regex g_includePattern;
    // define regex pattern
    extern std::regex g_definePattern;
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
bool readFile(const Str256& filePath, const Str64& fileName, T& contentOut, FileType fileType);
bool writeFile(const Str256& filePath, const Str64& fileName, const bufferPtr content, size_t length, FileType fileType);
bool deleteFile(const Str256& filePath, const Str64& fileName);
bool copyFile(const Str256& sourcePath, const Str64& sourceName, const Str256& targetPath, const Str64& targetName, bool overwrite);
bool findFile(const Str256& filePath, const Str64& fileName, Str256& foundPath);

/// will add '.' automatically at the start of 'extension'
bool hasExtension(const Str256& filePath, const Str8& extension);

stringImpl stripQuotes(const stringImpl& input);
FileWithPath splitPathToNameAndLocation(const stringImpl& input);

bool clearCache();
bool clearCache(CacheType type);

std::string extractFilePathAndName(char* argv0);

}; //namespace Divide

#endif //_PLATFORM_FILE_FILE_MANAGEMENT_H_

#include "FileManagement.inl"
