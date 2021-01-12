#include "stdafx.h"

#include "Headers/Configuration.h"

#include "Utility/Headers/Localization.h"

namespace Divide {

bool Configuration::fromXML(const char* xmlFile) {
    Console::printfn(Locale::get(_ID("XML_LOAD_CONFIG")), xmlFile);
    if (LoadSave.read(xmlFile)) {
        GET_PARAM(debug.enableRenderAPIDebugging);
        GET_PARAM(debug.useGeometryCache);
        GET_PARAM(debug.useVegetationCache);
        GET_PARAM(debug.enableTreeInstances);
        GET_PARAM(debug.enableGrassInstances);
        GET_PARAM(debug.renderFilter.primitives);
        GET_PARAM(debug.renderFilter.meshes);
        GET_PARAM(debug.renderFilter.terrain);
        GET_PARAM(debug.renderFilter.water);
        GET_PARAM(debug.renderFilter.sky);
        GET_PARAM(debug.renderFilter.particles);
        GET_PARAM(debug.useShaderTextCache);
        GET_PARAM(debug.useShaderBinaryCache);
        GET_PARAM(debug.memFile);
        GET_PARAM(language);
        GET_PARAM(runtime.targetDisplay);
        GET_PARAM(runtime.targetRenderingAPI);
        GET_PARAM(runtime.maxWorkerThreads);
        GET_PARAM(runtime.windowedMode);
        GET_PARAM(runtime.windowResizable);
        GET_PARAM(runtime.maximizeOnStart);
        GET_PARAM(runtime.frameRateLimit);
        GET_PARAM(runtime.enableVSync);
        GET_PARAM(runtime.adaptiveSync);
        GET_PARAM_ATTRIB(runtime.splashScreenSize, width);
        GET_PARAM_ATTRIB(runtime.splashScreenSize, height);
        GET_PARAM_ATTRIB(runtime.windowSize, width);
        GET_PARAM_ATTRIB(runtime.windowSize, height);
        GET_PARAM_ATTRIB(runtime.resolution, width);
        GET_PARAM_ATTRIB(runtime.resolution, height);
        GET_PARAM(runtime.cameraViewDistance);
        GET_PARAM(runtime.verticalFOV);
        GET_PARAM(gui.cegui.enabled);
        GET_PARAM(gui.consoleLayoutFile);
        GET_PARAM(terrain.detailLevel);
        GET_PARAM(terrain.textureQuality);
        GET_PARAM(terrain.parallaxMode);
        GET_PARAM(terrain.wireframe);
        GET_PARAM(terrain.showNormals);
        GET_PARAM(rendering.MSAASamples);
        GET_PARAM(rendering.anisotropicFilteringLevel);
        GET_PARAM(rendering.useBindlessTextures);
        GET_PARAM(rendering.reflectionProbeResolution);
        GET_PARAM(rendering.reflectionPlaneResolution);
        GET_PARAM(rendering.numLightsPerCluster);
        GET_PARAM_ATTRIB(rendering.lightClusteredSizes, width);
        GET_PARAM_ATTRIB(rendering.lightClusteredSizes, height);
        GET_PARAM_ATTRIB(rendering.lightClusteredSizes, depth);
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
        GET_PARAM(rendering.postFX.enableAdaptiveToneMapping);
        GET_PARAM(rendering.postFX.enableDepthOfField);
        GET_PARAM(rendering.postFX.enablePerObjectMotionBlur);
        GET_PARAM(rendering.postFX.enableBloom);
        GET_PARAM(rendering.postFX.bloomFactor);
        GET_PARAM(rendering.postFX.bloomThreshold);
        GET_PARAM(rendering.postFX.velocityScale);
        GET_PARAM(rendering.postFX.ssao.enable);
        GET_PARAM(rendering.postFX.ssao.UseHalfResolution);
        GET_PARAM(rendering.postFX.ssao.FullRes.Radius);
        GET_PARAM(rendering.postFX.ssao.FullRes.Bias);
        GET_PARAM(rendering.postFX.ssao.FullRes.Power);
        GET_PARAM(rendering.postFX.ssao.FullRes.KernelSampleCount);
        GET_PARAM(rendering.postFX.ssao.FullRes.Blur);
        GET_PARAM(rendering.postFX.ssao.FullRes.BlurThreshold);
        GET_PARAM(rendering.postFX.ssao.HalfRes.Radius);
        GET_PARAM(rendering.postFX.ssao.HalfRes.Bias);
        GET_PARAM(rendering.postFX.ssao.HalfRes.Power);
        GET_PARAM(rendering.postFX.ssao.HalfRes.KernelSampleCount);
        GET_PARAM(rendering.postFX.ssao.HalfRes.Blur);
        GET_PARAM(rendering.postFX.ssao.HalfRes.BlurThreshold);
        GET_PARAM(rendering.shadowMapping.enabled);
        GET_PARAM(rendering.shadowMapping.csm.enabled);
        GET_PARAM(rendering.shadowMapping.csm.shadowMapResolution);
        GET_PARAM(rendering.shadowMapping.csm.MSAASamples);
        GET_PARAM(rendering.shadowMapping.csm.enableBlurring);
        GET_PARAM(rendering.shadowMapping.csm.splitLambda);
        GET_PARAM(rendering.shadowMapping.csm.splitCount);
        GET_PARAM(rendering.shadowMapping.csm.anisotropicFilteringLevel);
        GET_PARAM(rendering.shadowMapping.spot.enabled);
        GET_PARAM(rendering.shadowMapping.spot.shadowMapResolution);
        GET_PARAM(rendering.shadowMapping.spot.MSAASamples);
        GET_PARAM(rendering.shadowMapping.spot.enableBlurring);
        GET_PARAM(rendering.shadowMapping.spot.anisotropicFilteringLevel);
        GET_PARAM(rendering.shadowMapping.point.enabled);
        GET_PARAM(rendering.shadowMapping.point.shadowMapResolution);

        GET_PARAM(title);
        GET_PARAM(defaultTextureLocation);
        GET_PARAM(defaultShadersLocation);

        if ( rendering.shadowMapping.enabled &&
            !rendering.shadowMapping.csm.enabled &&
            !rendering.shadowMapping.spot.enabled &&
            !rendering.shadowMapping.point.enabled)
        {
            rendering.shadowMapping.enabled = false;
        }

        return true;
    }

    return false;
}

bool Configuration::toXML(const char* xmlFile) const {
    if (LoadSave.prepareSaveFile(xmlFile)) {
        PUT_PARAM(debug.enableRenderAPIDebugging);
        PUT_PARAM(debug.useGeometryCache);
        PUT_PARAM(debug.useVegetationCache);
        PUT_PARAM(debug.enableTreeInstances);
        PUT_PARAM(debug.enableGrassInstances);
        PUT_PARAM(debug.renderFilter.primitives);
        PUT_PARAM(debug.renderFilter.meshes);
        PUT_PARAM(debug.renderFilter.terrain);
        PUT_PARAM(debug.renderFilter.water);
        PUT_PARAM(debug.renderFilter.sky);
        PUT_PARAM(debug.renderFilter.particles);
        PUT_PARAM(debug.useShaderTextCache);
        PUT_PARAM(debug.useShaderBinaryCache);
        PUT_PARAM(debug.memFile);
        PUT_PARAM(language);
        PUT_PARAM(runtime.targetDisplay);
        PUT_PARAM(runtime.targetRenderingAPI);
        PUT_PARAM(runtime.maxWorkerThreads);
        PUT_PARAM(runtime.windowedMode);
        PUT_PARAM(runtime.windowResizable);
        PUT_PARAM(runtime.maximizeOnStart);
        PUT_PARAM(runtime.frameRateLimit);
        PUT_PARAM(runtime.enableVSync);
        PUT_PARAM(runtime.adaptiveSync);
        PUT_PARAM_ATTRIB(runtime.splashScreenSize, width);
        PUT_PARAM_ATTRIB(runtime.splashScreenSize, height);
        PUT_PARAM_ATTRIB(runtime.windowSize, width);
        PUT_PARAM_ATTRIB(runtime.windowSize, height);
        PUT_PARAM_ATTRIB(runtime.resolution, width);
        PUT_PARAM_ATTRIB(runtime.resolution, height);
        PUT_PARAM(runtime.cameraViewDistance);
        PUT_PARAM(runtime.verticalFOV);
        PUT_PARAM(gui.cegui.enabled);
        PUT_PARAM(gui.consoleLayoutFile);
        PUT_PARAM(terrain.detailLevel);
        PUT_PARAM(terrain.textureQuality);
        PUT_PARAM(terrain.parallaxMode);
        PUT_PARAM(terrain.wireframe);
        PUT_PARAM(terrain.showNormals);
        PUT_PARAM(rendering.MSAASamples);
        PUT_PARAM(rendering.anisotropicFilteringLevel);
        PUT_PARAM(rendering.useBindlessTextures);
        PUT_PARAM(rendering.reflectionProbeResolution);
        PUT_PARAM(rendering.reflectionPlaneResolution);
        PUT_PARAM(rendering.numLightsPerCluster);
        PUT_PARAM_ATTRIB(rendering.lightClusteredSizes, width);
        PUT_PARAM_ATTRIB(rendering.lightClusteredSizes, height);
        PUT_PARAM_ATTRIB(rendering.lightClusteredSizes, depth);
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
        PUT_PARAM(rendering.postFX.enableAdaptiveToneMapping);
        PUT_PARAM(rendering.postFX.enableDepthOfField);
        PUT_PARAM(rendering.postFX.enablePerObjectMotionBlur);
        PUT_PARAM(rendering.postFX.enableBloom);
        PUT_PARAM(rendering.postFX.bloomThreshold);
        PUT_PARAM(rendering.postFX.bloomFactor);
        PUT_PARAM(rendering.postFX.velocityScale);
        PUT_PARAM(rendering.postFX.ssao.enable);
        PUT_PARAM(rendering.postFX.ssao.UseHalfResolution);
        PUT_PARAM(rendering.postFX.ssao.FullRes.Radius);
        PUT_PARAM(rendering.postFX.ssao.FullRes.Bias);
        PUT_PARAM(rendering.postFX.ssao.FullRes.Power);
        PUT_PARAM(rendering.postFX.ssao.FullRes.KernelSampleCount);
        PUT_PARAM(rendering.postFX.ssao.FullRes.Blur);
        PUT_PARAM(rendering.postFX.ssao.FullRes.BlurThreshold);
        PUT_PARAM(rendering.postFX.ssao.HalfRes.Radius);
        PUT_PARAM(rendering.postFX.ssao.HalfRes.Bias);
        PUT_PARAM(rendering.postFX.ssao.HalfRes.Power);
        PUT_PARAM(rendering.postFX.ssao.HalfRes.KernelSampleCount);
        PUT_PARAM(rendering.postFX.ssao.HalfRes.Blur);
        PUT_PARAM(rendering.postFX.ssao.HalfRes.BlurThreshold);
        PUT_PARAM(rendering.shadowMapping.enabled);
        PUT_PARAM(rendering.shadowMapping.csm.enabled);
        PUT_PARAM(rendering.shadowMapping.csm.shadowMapResolution);
        PUT_PARAM(rendering.shadowMapping.csm.MSAASamples);
        PUT_PARAM(rendering.shadowMapping.csm.enableBlurring);
        PUT_PARAM(rendering.shadowMapping.csm.splitLambda);
        PUT_PARAM(rendering.shadowMapping.csm.splitCount);
        PUT_PARAM(rendering.shadowMapping.csm.anisotropicFilteringLevel);
        PUT_PARAM(rendering.shadowMapping.spot.enabled);
        PUT_PARAM(rendering.shadowMapping.spot.shadowMapResolution);
        PUT_PARAM(rendering.shadowMapping.spot.MSAASamples);
        PUT_PARAM(rendering.shadowMapping.spot.enableBlurring);
        PUT_PARAM(rendering.shadowMapping.spot.anisotropicFilteringLevel);
        PUT_PARAM(rendering.shadowMapping.point.enabled);
        PUT_PARAM(rendering.shadowMapping.point.shadowMapResolution);

        PUT_PARAM(title);
        PUT_PARAM(defaultTextureLocation);
        PUT_PARAM(defaultShadersLocation);
        LoadSave.write();
        return true;
    }

    return false;
}

void Configuration::save() {
    if (changed() && toXML(LoadSave._loadPath.c_str())) {
        changed(false);
    }
}
}; //namespace Divide
