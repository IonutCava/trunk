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
    extern const char* g_assetsLocation;
    extern const char* g_shadersLocation;
    extern const char* g_texturesLocation;
    extern const char* g_soundsLocation;
    extern const char* g_xmlDataLocation;
    extern const char* g_scenesLocation;
    extern const char* g_GUILocation;
    extern const char* g_FontsPath;

    namespace Shaders {
        extern const char* g_CacheLocation;
        extern const char* g_CacheLocationText;
        extern const char* g_CacheLocationBin;

        namespace GLSL {
            // these must match the last 4 characters of the atom file
            extern const char* g_fragAtomExt;
            extern const char* g_vertAtomExt;
            extern const char* g_geomAtomExt;
            extern const char* g_tescAtomExt;
            extern const char* g_teseAtomExt;
            extern const char* g_compAtomExt;
            extern const char* g_comnAtomExt;

            // Shader subfolder name that contains shader files for OpenGL
            extern const char* g_parentShaderLoc;
            // Atom folder names in parent shader folder
            extern const char* g_fragAtomLoc;
            extern const char* g_vertAtomLoc;
            extern const char* g_geomAtomLoc;
            extern const char* g_tescAtomLoc;
            extern const char* g_teseAtomLoc;
            extern const char* g_compAtomLoc;
            extern const char* g_comnAtomLoc;
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