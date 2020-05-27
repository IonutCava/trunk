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

#include "Utility/Headers/XMLParser.h"

namespace Divide {

struct Configuration final : public XML::IXMLSerializable {
    struct Debug {
        /// If true, load shader source code from cache files
        /// If false, materials recompute shader source code from shader atoms
        /// If true, clear shader cache to apply changes to shader atom source code
        I32 dumpCommandBuffersOnFrame = -1;
        bool enableRenderAPIDebugging = false;
        bool enableDebugMsgGroups = false;
        bool useGeometryCache = true;
        bool useVegetationCache = true;
        bool useShaderBinaryCache = true;
        bool useShaderTextCache = true;
        bool enableTreeInstances = true;
        bool enableGrassInstances = true;
        stringImpl memFile = "none";
    } debug = {};
    
    stringImpl language = "enGB";
    
    struct Runtime {
        U8 targetDisplay = 0;
        U8 targetRenderingAPI = 0;
        I16 maxWorkerThreads = -1;
        U8   windowedMode = 0;
        bool windowResizable = false;
        bool maximizeOnStart = true;
        bool enableVSync = true;
        bool adaptiveSync = false;
        I16  frameRateLimit = -1;
        vec2<U16> splashScreenSize = { 400, 300 };
        vec2<U16> windowSize = { 1280, 720 };
        vec2<U16> resolution = { 1024, 768 };
        F32 simSpeed = 1.f;
        F32 zNear = 0.1f;
        F32 zFar = 1000.0f;
        U8  verticalFOV = 60u;
    } runtime = {};

    struct GUI {
        struct CEGUI {
            bool enabled = true;
            bool skipRendering = false;
            stringImpl defaultGUIScheme = "GWEN";
        } cegui = {};
        struct IMGUI {
            bool multiViewportEnabled = true;
            bool windowDecorationsEnabled = true;
            bool dontMergeFloatingWindows = true;
        } imgui = {};
        stringImpl consoleLayoutFile = "console.layout";
    } gui = {};

    struct ShadowSettings {
        U32 shadowMapResolution = 512u;
        U8 MSAASamples = 0u;
        U8 anisotropicFilteringLevel = 0u;
        bool enableBlurring = false;
    };

    struct Rendering {
        U8 MSAASamples = 0u;
        U8 anisotropicFilteringLevel = 16;
        U8 reflectionResolutionFactor = 1;
        I32 terrainDetailLevel = 0;
        I32 terrainTextureQuality = 0;
        I32 numLightsPerScreenTile = -1;
        U8 lightThreadGroupSize = 1;
        bool enableFog = true;
        F32 fogDensity = 1.0f;
        vec3<F32> fogColour = { 0.2f, 0.2f, 0.2f };
        vec4<U16> lodThresholds = { 25u, 45u, 85u, 165u };
        struct PostFX {
            stringImpl postAAType = "FXAA";
            U8 PostAAQualityLevel = 2;
            bool enableDepthOfField = false;
            bool enablePerObjectMotionBlur = true;
            bool enableCameraBlur = true;
            bool enableBloom = false;
            F32 bloomFactor = 0.f;
            F32 velocityScale = 1.f;
            bool enableSSAO = false;
            F32 ssaoRadius = 0.5f;
            F32 ssaoPower = 2.0f;
            U8 ssaoKernelSizeIndex = 0u;
        } postFX = {};
        struct ShadowMapping {
            bool enabled = true;
            F32 softness = 0.f;
            struct CSMSettings : public ShadowSettings
            {
                F32 splitLambda = 0.95f;
                U8  splitCount = 3u;
            } csm;
            ShadowSettings spot = {};
            ShadowSettings point = {};
        } shadowMapping = {};
    } rendering = {};

    stringImpl title = "DIVIDE Framework";
    stringImpl defaultTextureLocation = "textures/";
    stringImpl defaultShadersLocation = "shaders/";

    PROPERTY_RW(bool, changed, false);

    void save();

protected:
    bool fromXML(const char* xmlFile) override;
    bool toXML(const char* xmlFile) const override;
};
}; //namespace Divide

#endif//_CORE_CONFIGURATION_H_

