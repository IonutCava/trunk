/*
Copyright (c) 2016 DIVIDE-Studio
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
#include "Core/TemplateLibraries/Headers/String.h"

namespace Divide {

class PlatformContext;
namespace Paths {
    extern stringImplAligned g_assetsLocation;
    extern stringImplAligned g_shadersLocation;
    extern stringImplAligned g_texturesLocation;
    extern stringImplAligned g_imagesLocation;
    extern stringImplAligned g_materialsLocation;
    extern stringImplAligned g_soundsLocation;
    extern stringImplAligned g_xmlDataLocation;
    extern stringImplAligned g_navMeshesLocation;
    extern stringImplAligned g_scenesLocation;
    extern stringImplAligned g_GUILocation;
    extern stringImplAligned g_FontsPath;

    namespace Shaders {
        extern stringImplAligned g_CacheLocation;
        extern stringImplAligned g_CacheLocationText;
        extern stringImplAligned g_CacheLocationBin;

        namespace GLSL {
            // these must match the last 4 characters of the atom file
            extern stringImplAligned g_fragAtomExt;
            extern stringImplAligned g_vertAtomExt;
            extern stringImplAligned g_geomAtomExt;
            extern stringImplAligned g_tescAtomExt;
            extern stringImplAligned g_teseAtomExt;
            extern stringImplAligned g_compAtomExt;
            extern stringImplAligned g_comnAtomExt;

            // Shader subfolder name that contains shader files for OpenGL
            extern stringImplAligned g_parentShaderLoc;
            // Atom folder names in parent shader folder
            extern stringImplAligned g_fragAtomLoc;
            extern stringImplAligned g_vertAtomLoc;
            extern stringImplAligned g_geomAtomLoc;
            extern stringImplAligned g_tescAtomLoc;
            extern stringImplAligned g_teseAtomLoc;
            extern stringImplAligned g_compAtomLoc;
            extern stringImplAligned g_comnAtomLoc;
        }; //namespace GLSL
        namespace HLSL {

        }; //namespace HLSL
    }; //namespace Shaders

    // include command regex pattern
    extern std::regex g_includePattern;

    void updatePaths(const PlatformContext& context);
};

bool FileExists(const char* filePath);

void ReadTextFile(const stringImpl& filePath, stringImpl& contentOut);
stringImpl ReadTextFile(const stringImpl& filePath);

void WriteTextFile(const stringImpl& filePath, const stringImpl& content);

/// will add '.' automatically at the start of 'extension'
bool HasExtension(const stringImpl& filePath, const stringImpl& extension);

std::pair<stringImpl/*fileName*/, stringImpl/*filePath*/> SplitPathToNameAndLocation(const stringImpl& input);

}; //namespace Divide

#endif //_PLATFORM_FILE_FILE_MANAGEMENT_H_