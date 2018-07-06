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

#ifndef _PLATFORM_FILE_FILE_MANAGEMENT_H_
#define _PLATFORM_FILE_FILE_MANAGEMENT_H_

#include <regex>
#include "Platform/Headers/PlatformDataTypes.h"
#include "Core/TemplateLibraries/Headers/String.h"
#include "Core/TemplateLibraries/Headers/Vector.h"

namespace Divide {

class PlatformContext;
namespace Paths {
    extern stringImpl g_assetsLocation;
    extern stringImpl g_shadersLocation;
    extern stringImpl g_texturesLocation;
    extern stringImpl g_imagesLocation;
    extern stringImpl g_materialsLocation;
    extern stringImpl g_soundsLocation;
    extern stringImpl g_xmlDataLocation;
    extern stringImpl g_navMeshesLocation;
    extern stringImpl g_scenesLocation;
    extern stringImpl g_saveLocation;
    extern stringImpl g_GUILocation;
    extern stringImpl g_FontsPath;
    extern stringImpl g_LocalisationPath;

    namespace Shaders {
        extern stringImpl g_CacheLocation;
        extern stringImpl g_CacheLocationText;
        extern stringImpl g_CacheLocationBin;

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

    void initPaths();
    void updatePaths(const PlatformContext& context);
};

typedef char Byte; //< For file I/O

enum FileType {
    BINARY = 0,
    TEXT = 1,
    COUNT
};

struct FileWithPath {
    stringImpl _fileName;
    stringImpl _path;
};

bool fileExists(const char* filePath);
bool createFile(const char* filePath, bool overwriteExisting);

bool readFile(const stringImpl& filePath, stringImpl& contentOut, FileType fileType);
bool readFile(const stringImpl& filePath, vectorImpl<U8>& contentOut, FileType fileType);
bool readFile(const stringImpl& filePath, vectorImpl<Byte>& contentOut, FileType fileType);
bool readFile(const stringImpl& filePath, vectorImplFast<U8>& contentOut, FileType fileType);
bool readFile(const stringImpl& filePath, vectorImplFast<Byte>& contentOut, FileType fileType);

bool writeFile(const stringImpl& filePath, const char* content, FileType fileType);
bool writeFile(const stringImpl& filePath, const char* content, size_t length, FileType fileType);
bool writeFile(const stringImpl& filePath, const vectorImpl<U8>& content, FileType fileType);
bool writeFile(const stringImpl& filePath, const vectorImpl<U8>& content, size_t length, FileType fileType);
bool writeFile(const stringImpl& filePath, const vectorImpl<Byte>& content, FileType fileType);
bool writeFile(const stringImpl& filePath, const vectorImpl<Byte>& content, size_t length, FileType fileType);
bool writeFile(const stringImpl& filePath, const vectorImplFast<U8>& content, FileType fileType);
bool writeFile(const stringImpl& filePath, const vectorImplFast<U8>& content, size_t length, FileType fileType);
bool writeFile(const stringImpl& filePath, const vectorImplFast<Byte>& content, FileType fileType);
bool writeFile(const stringImpl& filePath, const vectorImplFast<Byte>& content, size_t length, FileType fileType);

/// will add '.' automatically at the start of 'extension'
bool hasExtension(const stringImpl& filePath, const stringImpl& extension);

FileWithPath splitPathToNameAndLocation(const stringImpl& input);

}; //namespace Divide

#endif //_PLATFORM_FILE_FILE_MANAGEMENT_H_