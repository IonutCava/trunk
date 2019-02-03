#include "stdafx.h"

#include "Headers/Configuration.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/WindowManager.h"

#include "Utility/Headers/Localization.h"

#include "Platform/Video/Headers/RenderAPIEnums.h"
#include "Platform/File/Headers/FileManagement.h"

namespace Divide {

Configuration::Configuration() : XML::IXMLSerializable()
{
    debug.flushCommandBuffersOnFrame = -1;
    debug.enableRenderAPIDebugging = true;
    debug.enableDebugMsgGroups = false;
    debug.useShaderBinaryCache = false;
    debug.useShaderTextCache = false;
    debug.memFile = "none";
    debug.mesh.playAnimations = true;
    language = "enGB";
    runtime.targetDisplay = 0;
    runtime.targetRenderingAPI = to_U8(RenderAPI::OpenGL);
    runtime.useFixedTimestep = true;
    runtime.maxWorkerThreads = -1;
    runtime.windowedMode = to_U8(WindowMode::WINDOWED);
    runtime.windowResizable = false;
    runtime.enableVSync = false;
    runtime.adaptiveSync = true;
    runtime.frameRateLimit = -1;
    runtime.splashScreen.set(400, 300);
    runtime.windowSize.set(1280, 768);
    runtime.resolution.set(1024, 768);
    runtime.simSpeed = 1.0f;
    runtime.zNear = 0.1f;
    runtime.zFar = 700.0f;
    runtime.verticalFOV = 60;

    gui.cegui.enabled = true;
    gui.cegui.extraStates = false;
    gui.cegui.skipRendering = false;
    gui.cegui.showDebugCursor = false;
    gui.cegui.defaultGUIScheme = "GWEN";
    gui.consoleLayoutFile = "console.layout";
    gui.editorLayoutFile = "editor.layout";

    gui.imgui.multiViewportEnabled = false;
    gui.imgui.windowDecorationsEnabled = false;
    gui.imgui.dontMergeFloatingWindows = false;

    rendering.msaaSamples = 0;
    rendering.anisotropicFilteringLevel = 1;
    rendering.renderDetailLevel = RenderDetailLevel::HIGH;
    rendering.multithreadedCommandGeneration = true;
    rendering.batchPassBuffers = true;
    rendering.enableFog = true;
    rendering.fogDensity = 0.01f; 
    rendering.fogColour.set(0.2f);
    rendering.lodThresholds.set(25, 45, 85, 165);
    rendering.postFX.postAAType = "FXAA";
    rendering.postFX.postAASamples = 0;
    rendering.postFX.enableDepthOfField = false;
    rendering.postFX.enableBloom = false;
    rendering.postFX.bloomFactor = 0.4f;
    rendering.postFX.enableSSAO = false;

    rendering.shadowMapping.enabled = true;
    rendering.shadowMapping.shadowMapResolution = 512;
    rendering.shadowMapping.msaaSamples = 0;
    rendering.shadowMapping.anisotropicFilteringLevel = 1;
    rendering.shadowMapping.enableBlurring = false;
    rendering.shadowMapping.defaultCSMSplitCount = 3;
    rendering.shadowMapping.softness = 0.25f;
    rendering.shadowMapping.splitLambda = 0.65f;
    title = "DIVIDE Framework";
    defaultTextureLocation = "textures/";
    defaultShadersLocation = "shaders/";
}

Configuration::~Configuration()
{
}

bool Configuration::fromXML(const char* xmlFile) {
    Console::printfn(Locale::get(_ID("XML_LOAD_CONFIG")), xmlFile);
    LOAD_FILE(xmlFile);
    if (FILE_VALID()) {
        GET_PARAM(debug.flushCommandBuffersOnFrame);
        GET_PARAM(debug.enableRenderAPIDebugging);
        GET_PARAM(debug.enableDebugMsgGroups);
        GET_PARAM(debug.useShaderBinaryCache);
        GET_PARAM(debug.useShaderTextCache);
        GET_PARAM(debug.memFile);
        GET_PARAM(debug.mesh.playAnimations);
        GET_PARAM(language);
        GET_PARAM(runtime.targetDisplay);
        GET_PARAM(runtime.targetRenderingAPI);
        GET_PARAM(runtime.useFixedTimestep);
        GET_PARAM(runtime.maxWorkerThreads);
        GET_PARAM(runtime.windowedMode);
        GET_PARAM(runtime.windowResizable);
        GET_PARAM(runtime.frameRateLimit);
        GET_PARAM(runtime.enableVSync);
        GET_PARAM(runtime.adaptiveSync);
        GET_PARAM_ATTRIB(runtime.splashScreen, w);
        GET_PARAM_ATTRIB(runtime.splashScreen, h);
        GET_PARAM_ATTRIB(runtime.windowSize, w);
        GET_PARAM_ATTRIB(runtime.windowSize, h);
        GET_PARAM_ATTRIB(runtime.resolution, w);
        GET_PARAM_ATTRIB(runtime.resolution, h);
        GET_PARAM(runtime.simSpeed);
        GET_PARAM(runtime.zNear);
        GET_PARAM(runtime.zFar);
        GET_PARAM(runtime.verticalFOV);
        GET_PARAM(gui.cegui.enabled);
        GET_PARAM(gui.cegui.extraStates);
        GET_PARAM(gui.cegui.skipRendering);
        GET_PARAM(gui.cegui.showDebugCursor);
        GET_PARAM(gui.cegui.defaultGUIScheme);
        GET_PARAM(gui.imgui.multiViewportEnabled);
        GET_PARAM(gui.imgui.windowDecorationsEnabled);
        GET_PARAM(gui.imgui.dontMergeFloatingWindows);
        GET_PARAM(gui.consoleLayoutFile);
        GET_PARAM(gui.editorLayoutFile);
        GET_PARAM(rendering.msaaSamples);
        GET_PARAM(rendering.anisotropicFilteringLevel);
        U32 detail = to_I32(rendering.renderDetailLevel);
        GET_TEMP_PARAM(rendering.renderDetailLevel, detail);
        CLAMP(detail, to_U32(RenderDetailLevel::OFF), to_U32(RenderDetailLevel::COUNT) - 1);
        rendering.renderDetailLevel = static_cast<RenderDetailLevel>(detail);
        GET_PARAM(rendering.multithreadedCommandGeneration);
        GET_PARAM(rendering.batchPassBuffers);
        GET_PARAM(rendering.enableFog);
        GET_PARAM(rendering.fogDensity);
        GET_PARAM_ATTRIB(rendering.fogColour, r);
        GET_PARAM_ATTRIB(rendering.fogColour, g);
        GET_PARAM_ATTRIB(rendering.fogColour, b);
        GET_PARAM_ATTRIB(rendering.lodThresholds, x);
        GET_PARAM_ATTRIB(rendering.lodThresholds, y);
        GET_PARAM_ATTRIB(rendering.lodThresholds, z);
        GET_PARAM_ATTRIB(rendering.lodThresholds, w);
        GET_PARAM(rendering.postFX.postAAType);
        GET_PARAM(rendering.postFX.postAASamples);
        GET_PARAM(rendering.postFX.enableDepthOfField);
        GET_PARAM(rendering.postFX.enableBloom);
        GET_PARAM(rendering.postFX.bloomFactor);
        GET_PARAM(rendering.postFX.enableSSAO);

        GET_PARAM(rendering.shadowMapping.enabled);
        GET_PARAM(rendering.shadowMapping.shadowMapResolution);
        GET_PARAM(rendering.shadowMapping.msaaSamples);
        GET_PARAM(rendering.shadowMapping.anisotropicFilteringLevel);
        GET_PARAM(rendering.shadowMapping.enableBlurring);
        GET_PARAM(rendering.shadowMapping.defaultCSMSplitCount);
        GET_PARAM(rendering.shadowMapping.softness);
        GET_PARAM(rendering.shadowMapping.splitLambda);

        GET_PARAM(title);
        GET_PARAM(defaultTextureLocation);
        GET_PARAM(defaultShadersLocation);

        return true;
    }

    return false;
}

bool Configuration::toXML(const char* xmlFile) const {
    PREPARE_FILE_FOR_WRITING(xmlFile);
    PUT_PARAM(debug.flushCommandBuffersOnFrame);
    PUT_PARAM(debug.enableRenderAPIDebugging);
    PUT_PARAM(debug.enableDebugMsgGroups);
    PUT_PARAM(debug.useShaderBinaryCache);
    PUT_PARAM(debug.useShaderTextCache);
    PUT_PARAM(debug.memFile);
    PUT_PARAM(debug.mesh.playAnimations);
    PUT_PARAM(language);
    PUT_PARAM(runtime.targetDisplay);
    PUT_PARAM(runtime.targetRenderingAPI);
    PUT_PARAM(runtime.useFixedTimestep);
    PUT_PARAM(runtime.maxWorkerThreads);
    PUT_PARAM(runtime.windowedMode);
    PUT_PARAM(runtime.windowResizable);
    PUT_PARAM(runtime.frameRateLimit);
    PUT_PARAM(runtime.enableVSync);
    PUT_PARAM(runtime.adaptiveSync);
    PUT_PARAM_ATTRIB(runtime.splashScreen, w);
    PUT_PARAM_ATTRIB(runtime.splashScreen, h);
    PUT_PARAM_ATTRIB(runtime.windowSize, w);
    PUT_PARAM_ATTRIB(runtime.windowSize, h);
    PUT_PARAM_ATTRIB(runtime.resolution, w);
    PUT_PARAM_ATTRIB(runtime.resolution, h);
    PUT_PARAM(runtime.simSpeed);
    PUT_PARAM(runtime.zNear);
    PUT_PARAM(runtime.zFar);
    PUT_PARAM(runtime.verticalFOV);
    PUT_PARAM(gui.cegui.enabled);
    PUT_PARAM(gui.cegui.extraStates);
    PUT_PARAM(gui.cegui.skipRendering);
    PUT_PARAM(gui.cegui.showDebugCursor);
    PUT_PARAM(gui.cegui.defaultGUIScheme);
    PUT_PARAM(gui.imgui.multiViewportEnabled);
    PUT_PARAM(gui.imgui.windowDecorationsEnabled);
    PUT_PARAM(gui.imgui.dontMergeFloatingWindows);
    PUT_PARAM(gui.consoleLayoutFile);
    PUT_PARAM(gui.editorLayoutFile);
    PUT_PARAM(rendering.msaaSamples);
    PUT_PARAM(rendering.anisotropicFilteringLevel);
    PUT_TEMP_PARAM(rendering.renderDetailLevel, to_I32(rendering.renderDetailLevel));
    PUT_PARAM(rendering.multithreadedCommandGeneration);
    PUT_PARAM(rendering.batchPassBuffers);
    PUT_PARAM(rendering.enableFog);
    PUT_PARAM(rendering.fogDensity);
    PUT_PARAM_ATTRIB(rendering.fogColour, r);
    PUT_PARAM_ATTRIB(rendering.fogColour, g);
    PUT_PARAM_ATTRIB(rendering.fogColour, b);
    PUT_PARAM_ATTRIB(rendering.lodThresholds, x);
    PUT_PARAM_ATTRIB(rendering.lodThresholds, y);
    PUT_PARAM_ATTRIB(rendering.lodThresholds, z);
    PUT_PARAM_ATTRIB(rendering.lodThresholds, w);
    PUT_PARAM(rendering.postFX.postAAType);
    PUT_PARAM(rendering.postFX.postAASamples);
    PUT_PARAM(rendering.postFX.enableDepthOfField);
    PUT_PARAM(rendering.postFX.enableBloom);
    PUT_PARAM(rendering.postFX.bloomFactor);
    PUT_PARAM(rendering.postFX.enableSSAO);

    PUT_PARAM(rendering.shadowMapping.enabled);
    PUT_PARAM(rendering.shadowMapping.shadowMapResolution);
    PUT_PARAM(rendering.shadowMapping.msaaSamples);
    PUT_PARAM(rendering.shadowMapping.anisotropicFilteringLevel);
    PUT_PARAM(rendering.shadowMapping.enableBlurring);
    PUT_PARAM(rendering.shadowMapping.defaultCSMSplitCount);
    PUT_PARAM(rendering.shadowMapping.softness);
    PUT_PARAM(rendering.shadowMapping.splitLambda);

    PUT_PARAM(title);
    PUT_PARAM(defaultTextureLocation);
    PUT_PARAM(defaultShadersLocation);
    SAVE_FILE(xmlFile);
    return true;
}

}; //namespace Divide
