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
#ifndef _CORE_CONFIGURATION_H_
#define _CORE_CONFIGURATION_H_

#include "Core/Math/Headers/MathVectors.h"
#include "Utility/Headers/XMLParser.h"

namespace Divide {

enum class RenderDetailLevel : U32;

class Configuration : public XML::IXMLSerializable {
public:
    Configuration();
    ~Configuration();

protected:
    bool fromXML(const char* xmlFile) override;
    bool toXML(const char* xmlFile) const override;

public:
    struct Debug {
        /// If true, load shader source code from cache files
        /// If false, materials recompute shader source code from shader atoms
        /// If true, clear shader cache to apply changes to shader atom source code
        bool useShaderBinaryCache;
        bool useShaderTextCache;
        stringImpl memFile;
        struct Mesh {
            bool playAnimations;
        } mesh;
    } debug;
    
    stringImpl language;
    
    struct Runtime {
        U8 targetDisplay;
        bool windowedMode;
        bool windowResizable;
        bool enableVSync;
        bool adaptiveSync;
        I16  frameRateLimit;
        vec2<U16> splashScreen;
        vec2<U16> resolution;
        F32 simSpeed;
        F32 zNear;
        F32 zFar;
        U8  verticalFOV;
    } runtime;

    struct GUI {
        struct CEGUI {
            bool enabled;
            bool extraStates;
            bool skipRendering;
            bool showDebugCursor;
            stringImpl defaultGUIScheme;
        } cegui;
        stringImpl consoleLayoutFile;
        stringImpl editorLayoutFile;
        bool enableDebugVariableControls;
    } gui;

    struct Rendering {
        stringImpl postAAType;
        U8 postAASamples;
        U8 msaaSamples;
        U8 anisotropicFilteringLevel;
        RenderDetailLevel shadowDetailLevel;
        RenderDetailLevel renderDetailLevel;
        bool enableFog;
        F32 fogDensity;
        vec3<F32> fogColour;
        bool enable3D;
        F32 anaglyphOffset;
        bool enableDepthOfField;
        bool enableBloom;
        F32 bloomFactor;
        bool enableSSAO;
    } rendering;

    stringImpl title;
    stringImpl defaultTextureLocation;
    stringImpl defaultShadersLocation;
};
}; //namespace Divide

#endif//_CORE_CONFIGURATION_H_

