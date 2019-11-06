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
        I32 flushCommandBuffersOnFrame = -1;
        bool enableRenderAPIDebugging = false;
        bool enableDebugMsgGroups = false;
        bool useGeometryCache = true;
        bool useVegetationCache = true;
        bool useShaderBinaryCache = true;
        bool useShaderTextCache = true;
        bool enableTreeInstances = true;
        bool enableGrassInstances = true;
        stringImpl memFile = "";
        struct Mesh {
            bool playAnimations = true;
        } mesh = {};
    } debug = {};
    
    stringImpl language = "";
    
    struct Runtime {
        U8 targetDisplay = 0;
        U8 targetRenderingAPI = 0;
        bool useFixedTimestep = true;
        I16 maxWorkerThreads = 8;
        U8   windowedMode = 0;
        bool windowResizable = false;
        bool enableVSync = true;
        bool adaptiveSync = false;
        I16  frameRateLimit = -1;
        vec2<U16> splashScreenSize;
        vec2<U16> windowSize;
        vec2<U16> resolution;
        F32 simSpeed = 1.f;
        F32 zNear = 0.1f;
        F32 zFar = 1000.0f;
        U8  verticalFOV = 60u;
    } runtime = {};

    struct GUI {
        struct CEGUI {
            bool enabled = true;
            bool extraStates = false;
            bool skipRendering = false;
            bool showDebugCursor = false;
            stringImpl defaultGUIScheme = "";
        } cegui = {};
        struct IMGUI {
            bool multiViewportEnabled = true;
            bool windowDecorationsEnabled = true;
            bool dontMergeFloatingWindows = true;
        } imgui = {};
        stringImpl consoleLayoutFile = "";
        stringImpl editorLayoutFile = "";
    } gui = {};

    struct Rendering {
        U8 msaaSamples = 0;
        U8 anisotropicFilteringLevel = 16;
        U8 reflectionResolutionFactor = 1;
        I32 terrainDetailLevel = 0;
        I32 terrainTextureQuality = 0;
        I32 numLightsPerScreenTile = 512;
        U8 lightThreadGroupSize = 1;
        bool enableFog = true;
        F32 fogDensity = 1.0f;
        vec3<F32> fogColour = {};
        vec4<U16> lodThresholds = {};
        struct PostFX {
            stringImpl postAAType = "";
            U8 postAASamples = 0;
            bool enableDepthOfField = false;
            bool enableBloom = false;
            F32 bloomFactor = 0.f;
            bool enableSSAO = false;
            F32 ssaoRadius = 0.5f;
            F32 ssaoPower = 2.0f;
        } postFX = {};
        struct ShadowMapping {
            bool enabled = true;
            U32 shadowMapResolution = 512;
            U8 msaaSamples = 0;
            U8 anisotropicFilteringLevel = 0;
            bool enableBlurring = true;
            U8 defaultCSMSplitCount = 3;
            F32 softness = 0.f;
            F32 splitLambda = 0.95f;
        } shadowMapping = {};
    } rendering = {};

    stringImpl title = "No Title";
    stringImpl defaultTextureLocation = "";
    stringImpl defaultShadersLocation = "";
};
}; //namespace Divide

#endif//_CORE_CONFIGURATION_H_

