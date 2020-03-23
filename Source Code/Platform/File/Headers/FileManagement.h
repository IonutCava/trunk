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
struct Paths {
    static Str128 g_exePath;
    static Str64 g_logPath;
    static Str64 g_assetsLocation;
    static Str64 g_shadersLocation;
    static Str64 g_texturesLocation;
    static Str64 g_heightmapLocation;
    static Str64 g_climatesLowResLocation;
    static Str64 g_climatesMedResLocation;
    static Str64 g_climatesHighResLocation;
    static Str64 g_imagesLocation;
    static Str64 g_materialsLocation;
    static Str64 g_soundsLocation;
    static Str64 g_xmlDataLocation;
    static Str64 g_navMeshesLocation;
    static Str64 g_scenesLocation;
    static Str64 g_saveLocation;
    static Str64 g_GUILocation;
    static Str64 g_fontsPath;
    static Str64 g_localisationPath;
    static Str64 g_cacheLocation;
    static Str64 g_terrainCacheLocation;
    static Str64 g_geometryCacheLocation;

    struct Editor {
        static Str64 g_saveLocation;
        static Str64 g_tabLayoutFile;
        static Str64 g_panelLayoutFile;
    };

    struct Scripts {
        static Str64 g_scriptsLocation;
        static Str64 g_scriptsAtomsLocation;
    };

    struct Textures {
        static Str64 g_metadataLocation;
    };

    struct Shaders {
        static Str64 g_cacheLocation;
        static Str64 g_cacheLocationText;
        static Str64 g_cacheLocationBin;

        struct GLSL {
            // these must match the last 4 characters of the atom file
            static Str8 g_fragAtomExt;
            static Str8 g_vertAtomExt;
            static Str8 g_geomAtomExt;
            static Str8 g_tescAtomExt;
            static Str8 g_teseAtomExt;
            static Str8 g_compAtomExt;
            static Str8 g_comnAtomExt;

            // Shader subfolder name that contains shader files for OpenGL
            static Str64 g_parentShaderLoc;
            // Atom folder names in parent shader folder
            static Str64 g_fragAtomLoc;
            static Str64 g_vertAtomLoc;
            static Str64 g_geomAtomLoc;
            static Str64 g_tescAtomLoc;
            static Str64 g_teseAtomLoc;
            static Str64 g_compAtomLoc;
            static Str64 g_comnAtomLoc;
        }; //class GLSL
        struct HLSL {
            static Str64 g_parentShaderLoc;
        }; //class HLSL
    }; //class Shaders

    // include command regex pattern
    static std::regex g_includePattern;
    // define regex pattern
    static std::regex g_definePattern;
    // use command regex pattern
    static std::regex g_usePattern;

    
    static void initPaths(const SysInfo& info);
    static void updatePaths(const PlatformContext& context);
}; //class Paths


bool pathExists(const char* filePath);
bool fileExists(const char* filePathAndName);
bool createFile(const char* filePathAndName, bool overwriteExisting);
bool deleteAllFiles(const char* filePath, const char* extension = nullptr);

template<typename T,
         typename std::enable_if<std::is_same<decltype(has_assign<T>(nullptr)), std::true_type>::value, bool>::type* = nullptr>
bool readFile(const char* filePath, const char* fileName, T& contentOut, FileType fileType);

bool writeFile(const char* filePath, const char* fileName, const bufferPtr content, size_t length, FileType fileType);
bool deleteFile(const char* filePath, const char* fileName);
bool copyFile(const char* sourcePath, const char* sourceName, const char* targetPath, const char* targetName, bool overwrite);
bool findFile(const char* filePath, const char* fileName, stringImpl& foundPath);

/// will add '.' automatically at the start of 'extension'
bool hasExtension(const char* filePath, const Str16& extension);

stringImpl stripQuotes(const char* input);

FileWithPath splitPathToNameAndLocation(const char* input);

bool clearCache();
bool clearCache(CacheType type);

std::string extractFilePathAndName(char* argv0);

}; //namespace Divide

#endif //_PLATFORM_FILE_FILE_MANAGEMENT_H_

#include "FileManagement.inl"
