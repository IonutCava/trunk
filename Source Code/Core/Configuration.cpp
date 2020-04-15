#include "stdafx.h"

#include "Headers/Configuration.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/WindowManager.h"

#include "Utility/Headers/Localization.h"

#include "Platform/Video/Headers/RenderAPIEnums.h"
#include "Platform/File/Headers/FileManagement.h"

namespace Divide {

bool Configuration::fromXML(const char* xmlFile) {
    Console::printfn(Locale::get(_ID("XML_LOAD_CONFIG")), xmlFile);
    if (LoadSave.read(xmlFile)) {
        GET_PARAM(debug.dumpCommandBuffersOnFrame);
        GET_PARAM(debug.enableRenderAPIDebugging);
        GET_PARAM(debug.enableDebugMsgGroups);
        GET_PARAM(debug.useGeometryCache);
        GET_PARAM(debug.useVegetationCache);
        GET_PARAM(debug.enableTreeInstances);
        GET_PARAM(debug.enableGrassInstances);
        GET_PARAM(debug.useShaderTextCache);
        GET_PARAM(debug.useShaderBinaryCache);
        GET_PARAM(debug.showShadowCascadeSplits);
        GET_PARAM(debug.enableLighting);
        GET_PARAM(debug.memFile);
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
        GET_PARAM_ATTRIB(runtime.splashScreenSize, width);
        GET_PARAM_ATTRIB(runtime.splashScreenSize, height);
        GET_PARAM_ATTRIB(runtime.windowSize, width);
        GET_PARAM_ATTRIB(runtime.windowSize, height);
        GET_PARAM_ATTRIB(runtime.resolution, width);
        GET_PARAM_ATTRIB(runtime.resolution, height);
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
        GET_PARAM(rendering.MSAAsamples);
        GET_PARAM(rendering.anisotropicFilteringLevel);
        GET_PARAM(rendering.reflectionResolutionFactor);
        GET_PARAM(rendering.terrainDetailLevel);
        GET_PARAM(rendering.terrainTextureQuality);
        GET_PARAM(rendering.numLightsPerScreenTile);
        GET_PARAM(rendering.lightThreadGroupSize);
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
        GET_PARAM(rendering.postFX.PostAAQualityLevel);
        GET_PARAM(rendering.postFX.enableDepthOfField);
        GET_PARAM(rendering.postFX.enableBloom);
        GET_PARAM(rendering.postFX.bloomFactor);
        GET_PARAM(rendering.postFX.enableSSAO);
        GET_PARAM(rendering.postFX.ssaoRadius);
        GET_PARAM(rendering.postFX.ssaoPower);
        GET_PARAM(rendering.shadowMapping.enabled);
        GET_PARAM(rendering.shadowMapping.shadowMapResolution);
        GET_PARAM(rendering.shadowMapping.MSAAsamples);
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
    if (LoadSave.prepareSaveFile(xmlFile)) {
        PUT_PARAM(debug.dumpCommandBuffersOnFrame);
        PUT_PARAM(debug.enableRenderAPIDebugging);
        PUT_PARAM(debug.enableDebugMsgGroups);
        PUT_PARAM(debug.useGeometryCache);
        PUT_PARAM(debug.useVegetationCache);
        PUT_PARAM(debug.enableTreeInstances);
        PUT_PARAM(debug.enableGrassInstances);
        PUT_PARAM(debug.useShaderTextCache);
        PUT_PARAM(debug.useShaderBinaryCache);
        PUT_PARAM(debug.showShadowCascadeSplits);
        PUT_PARAM(debug.enableLighting);
        PUT_PARAM(debug.memFile);
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
        PUT_PARAM_ATTRIB(runtime.splashScreenSize, width);
        PUT_PARAM_ATTRIB(runtime.splashScreenSize, height);
        PUT_PARAM_ATTRIB(runtime.windowSize, width);
        PUT_PARAM_ATTRIB(runtime.windowSize, height);
        PUT_PARAM_ATTRIB(runtime.resolution, width);
        PUT_PARAM_ATTRIB(runtime.resolution, height);
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
        PUT_PARAM(rendering.MSAAsamples);
        PUT_PARAM(rendering.anisotropicFilteringLevel);
        PUT_PARAM(rendering.reflectionResolutionFactor);
        PUT_PARAM(rendering.terrainDetailLevel);
        PUT_PARAM(rendering.terrainTextureQuality);
        PUT_PARAM(rendering.numLightsPerScreenTile);
        PUT_PARAM(rendering.lightThreadGroupSize);
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
        PUT_PARAM(rendering.postFX.PostAAQualityLevel);
        PUT_PARAM(rendering.postFX.enableDepthOfField);
        PUT_PARAM(rendering.postFX.enableBloom);
        PUT_PARAM(rendering.postFX.bloomFactor);
        PUT_PARAM(rendering.postFX.enableSSAO);
        PUT_PARAM(rendering.postFX.ssaoRadius);
        PUT_PARAM(rendering.postFX.ssaoPower);

        PUT_PARAM(rendering.shadowMapping.enabled);
        PUT_PARAM(rendering.shadowMapping.shadowMapResolution);
        PUT_PARAM(rendering.shadowMapping.MSAAsamples);
        PUT_PARAM(rendering.shadowMapping.anisotropicFilteringLevel);
        PUT_PARAM(rendering.shadowMapping.enableBlurring);
        PUT_PARAM(rendering.shadowMapping.defaultCSMSplitCount);
        PUT_PARAM(rendering.shadowMapping.softness);
        PUT_PARAM(rendering.shadowMapping.splitLambda);

        PUT_PARAM(title);
        PUT_PARAM(defaultTextureLocation);
        PUT_PARAM(defaultShadersLocation);
        LoadSave.write();
        return true;
    }

    return false;
}

void Configuration::save() {
    if (changed()) {
        toXML(LoadSave._loadPath.c_str());
        changed(false);
    }
}
}; //namespace Divide
