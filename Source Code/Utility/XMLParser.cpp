#include "Headers/XMLParser.h"

#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Rendering/Headers/Renderer.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Material/Headers/Material.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"

namespace Divide {
namespace XML {
using boost::property_tree::ptree;
ptree pt;

namespace {
const char *getFilterName(TextureFilter filter) {
    if (filter == TextureFilter::TEXTURE_FILTER_LINEAR) {
        return "TEXTURE_FILTER_LINEAR";
    } else if (filter == TextureFilter::TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST) {
        return "TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST";
    } else if (filter == TextureFilter::TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST) {
        return "TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST";
    } else if (filter == TextureFilter::TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR) {
        return "TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR";
    } else if (filter == TextureFilter::TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR) {
        return "TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR";
    }

    return "TEXTURE_FILTER_NEAREST";
}

TextureFilter getFilter(const char *filter) {
    if (strcmp(filter, "TEXTURE_FILTER_LINEAR") == 0) {
        return TextureFilter::TEXTURE_FILTER_LINEAR;
    } else if (strcmp(filter, "TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST") == 0) {
        return TextureFilter::TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST;
    } else if (strcmp(filter, "TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST") == 0) {
        return TextureFilter::TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST;
    } else if (strcmp(filter, "TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR") == 0) {
        return TextureFilter::TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR;
    } else if (strcmp(filter, "TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR") == 0) {
        return TextureFilter::TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR;
    }

    return TextureFilter::TEXTURE_FILTER_NEAREST;
}

const char *getWrapModeName(TextureWrap wrapMode) {
    if (wrapMode == TextureWrap::TEXTURE_CLAMP) {
        return "TEXTURE_CLAMP";
    } else if (wrapMode == TextureWrap::TEXTURE_CLAMP_TO_EDGE) {
        return "TEXTURE_CLAMP_TO_EDGE";
    } else if (wrapMode == TextureWrap::TEXTURE_CLAMP_TO_BORDER) {
        return "TEXTURE_CLAMP_TO_BORDER";
    } else if (wrapMode == TextureWrap::TEXTURE_DECAL) {
        return "TEXTURE_DECAL";
    }

    return "TEXTURE_REPEAT";
}

TextureWrap getWrapMode(const char *wrapMode) {
    if (strcmp(wrapMode, "TEXTURE_CLAMP") == 0) {
        return TextureWrap::TEXTURE_CLAMP;
    } else if (strcmp(wrapMode, "TEXTURE_CLAMP_TO_EDGE") == 0) {
        return TextureWrap::TEXTURE_CLAMP_TO_EDGE;
    } else if (strcmp(wrapMode, "TEXTURE_CLAMP_TO_BORDER") == 0) {
        return TextureWrap::TEXTURE_CLAMP_TO_BORDER;
    } else if (strcmp(wrapMode, "TEXTURE_DECAL") == 0) {
        return TextureWrap::TEXTURE_DECAL;
    }

    return TextureWrap::TEXTURE_REPEAT;
}

const char *getBumpMethodName(Material::BumpMethod bumpMethod) {
    if (bumpMethod == Material::BumpMethod::BUMP_NORMAL) {
        return "BUMP_NORMAL";
    } else if (bumpMethod == Material::BumpMethod::BUMP_PARALLAX) {
        return "BUMP_PARALLAX";
    } else if (bumpMethod == Material::BumpMethod::BUMP_RELIEF) {
        return "BUMP_RELIEF";
    }

    return "BUMP_NONE";
}

Material::BumpMethod getBumpMethod(const char *bumpMethod) {
    if (strcmp(bumpMethod, "BUMP_NORMAL") == 0) {
        return Material::BumpMethod::BUMP_NORMAL;
    } else if (strcmp(bumpMethod, "BUMP_PARALLAX") == 0) {
        return Material::BumpMethod::BUMP_PARALLAX;
    } else if (strcmp(bumpMethod, "BUMP_RELIEF") == 0) {
        return Material::BumpMethod::BUMP_RELIEF;
    }

    return Material::BumpMethod::BUMP_NONE;
}

const char *getTextureOperationName(Material::TextureOperation textureOp) {
    if (textureOp == Material::TextureOperation::TextureOperation_Multiply) {
        return "TEX_OP_MULTIPLY";
    } else if (textureOp ==
               Material::TextureOperation::TextureOperation_Decal) {
        return "TEX_OP_DECAL";
    } else if (textureOp == Material::TextureOperation::TextureOperation_Add) {
        return "TEX_OP_ADD";
    } else if (textureOp ==
               Material::TextureOperation::TextureOperation_SmoothAdd) {
        return "TEX_OP_SMOOTH_ADD";
    } else if (textureOp ==
               Material::TextureOperation::TextureOperation_SignedAdd) {
        return "TEX_OP_SIGNED_ADD";
    } else if (textureOp ==
               Material::TextureOperation::TextureOperation_Divide) {
        return "TEX_OP_DIVIDE";
    } else if (textureOp ==
               Material::TextureOperation::TextureOperation_Subtract) {
        return "TEX_OP_SUBTRACT";
    }

    return "TEX_OP_REPLACE";
}

Material::TextureOperation getTextureOperation(const char *operation) {
    if (strcmp(operation, "TEX_OP_MULTIPLY") == 0) {
        return Material::TextureOperation::TextureOperation_Multiply;
    } else if (strcmp(operation, "TEX_OP_DECAL") == 0) {
        return Material::TextureOperation::TextureOperation_Decal;
    } else if (strcmp(operation, "TEX_OP_ADD") == 0) {
        return Material::TextureOperation::TextureOperation_Add;
    } else if (strcmp(operation, "TEX_OP_SMOOTH_ADD") == 0) {
        return Material::TextureOperation::TextureOperation_SmoothAdd;
    } else if (strcmp(operation, "TEX_OP_SIGNED_ADD") == 0) {
        return Material::TextureOperation::TextureOperation_SignedAdd;
    } else if (strcmp(operation, "TEX_OP_DIVIDE") == 0) {
        return Material::TextureOperation::TextureOperation_Divide;
    } else if (strcmp(operation, "TEX_OP_SUBTRACT") == 0) {
        return Material::TextureOperation::TextureOperation_Subtract;
    }

    return Material::TextureOperation::TextureOperation_Replace;
}

void saveTextureXML(const std::string &textureNode, Texture *texture,
                    ptree &tree, const std::string &operation = "") {
    const SamplerDescriptor &sampler = texture->getCurrentSampler();
    while (texture->getState() != ResourceState::RES_LOADED) {
        // texture not fully loaded yet
    }

    tree.put(textureNode + ".file",
             stringAlg::fromBase(texture->getResourceLocation()));
    tree.put(textureNode + ".flip", texture->isVerticallyFlipped());
    tree.put(textureNode + ".MapU", getWrapModeName(sampler.wrapU()));
    tree.put(textureNode + ".MapV", getWrapModeName(sampler.wrapV()));
    tree.put(textureNode + ".MapW", getWrapModeName(sampler.wrapW()));
    tree.put(textureNode + ".minFilter", getFilterName(sampler.minFilter()));
    tree.put(textureNode + ".magFilter", getFilterName(sampler.magFilter()));
    tree.put(textureNode + ".anisotropy", (U32)sampler.anisotropyLevel());

    if (!operation.empty()) {
        tree.put(textureNode + ".operation", operation);
    }
}

Texture *loadTextureXML(const std::string &textureNode,
                        const std::string &textureName) {
    std::string img_name(textureName.substr(textureName.find_last_of('/') + 1));
    std::string pathName(textureName.substr(0, textureName.rfind("/") + 1));

    TextureWrap wrapU = getWrapMode(
        pt.get<std::string>(textureNode + ".MapU", "TEXTURE_REPEAT").c_str());
    TextureWrap wrapV = getWrapMode(
        pt.get<std::string>(textureNode + ".MapV", "TEXTURE_REPEAT").c_str());
    TextureWrap wrapW = getWrapMode(
        pt.get<std::string>(textureNode + ".MapW", "TEXTURE_REPEAT").c_str());
    TextureFilter minFilterValue =
        getFilter(pt.get<std::string>(textureNode + ".minFilter",
                                      "TEXTURE_FILTER_LINEAR").c_str());
    TextureFilter magFilterValue =
        getFilter(pt.get<std::string>(textureNode + ".magFilter",
                                      "TEXTURE_FILTER_LINEAR").c_str());
    U32 anisotropy = pt.get(textureNode + ".anisotropy", 0);

    SamplerDescriptor sampDesc;
    sampDesc.setWrapMode(wrapU, wrapV, wrapW);
    sampDesc.setFilters(minFilterValue, magFilterValue);
    sampDesc.setAnisotropy(anisotropy);

    ResourceDescriptor texture(stringAlg::toBase(img_name));
    texture.setResourceLocation(stringAlg::toBase(pathName + img_name));
    texture.setFlag(pt.get(textureNode + ".flip", true));
    texture.setPropertyDescriptor<SamplerDescriptor>(sampDesc);

    return CreateResource<Texture>(texture);
}

inline std::string getRendererTypeName(RendererType type) {
    switch (type) {
        default:
        case RendererType::COUNT:
            return "Unknown_Renderer_Type";
        case RendererType::RENDERER_FORWARD_PLUS:
            return "Forward_Renderer";
        case RendererType::RENDERER_DEFERRED_SHADING:
            return "Deferred_Shading Renderer";
    }
}
}

std::string loadScripts(const std::string &file) {
    ParamHandler &par = ParamHandler::getInstance();
    Console::printfn(Locale::get("XML_LOAD_SCRIPTS"));
    read_xml(file.c_str(), pt);
    std::string activeScene("MainScene");
    par.setParam("testInt", 2);
    par.setParam("testFloat", 3.2f);
    par.setParam("scriptLocation",
                 pt.get<std::string>("scriptLocation", "XML"));
    par.setParam("assetsLocation", pt.get<std::string>("assets", "assets"));
    par.setParam("scenesLocation",
                 pt.get<std::string>("scenesLocation", "Scenes"));
    par.setParam("serverAddress", pt.get<std::string>("server", "127.0.0.1"));
    loadConfig(par.getParam<std::string>("scriptLocation", "XML") + "/" +
               pt.get("config", "config.xml"));
    read_xml(par.getParam<std::string>("scriptLocation", "XML") + "/" +
                 pt.get("startupScene", "scenes.xml"),
             pt);
    activeScene = pt.get("StartupScene", activeScene);

    return activeScene;
}

void loadConfig(const std::string &file) {
    ParamHandler &par = ParamHandler::getInstance();
    pt.clear();
    Console::printfn(Locale::get("XML_LOAD_CONFIG"), file.c_str());
    read_xml(file.c_str(), pt);
    par.setParam("locale", pt.get("language", "enGB"));
    par.setParam("logFile", pt.get("debug.logFile", "none"));
    par.setParam("memFile", pt.get("debug.memFile", "none"));
    par.setParam("simSpeed", pt.get("runtime.simSpeed", 1.0f));
    par.setParam("appTitle", pt.get("title", "DIVIDE Framework"));
    par.setParam("defaultTextureLocation",
                 pt.get("defaultTextureLocation", "textures/"));
    par.setParam("shaderLocation",
                 pt.get("defaultShadersLocation", "shaders/"));

    I32 shadowDetailLevel = pt.get<I32>("rendering.shadowDetailLevel", 2);
    if (shadowDetailLevel <= 0) {
        GFX_DEVICE.shadowDetailLevel(RenderDetailLevel::DETAIL_LOW);
        par.setParam("rendering.enableShadows", false);
    } else {
        GFX_DEVICE.shadowDetailLevel(
            static_cast<RenderDetailLevel>(std::min(shadowDetailLevel, 3) - 1));
        par.setParam("rendering.enableShadows", true);
    }

    GFX_DEVICE.postProcessingEnabled(pt.get("rendering.enablePostFX", false));
    GFX_DEVICE.anaglyphEnabled(pt.get("rendering.enable3D", false));
    par.setParam("rendering.MSAAsampless",
                 std::max(pt.get<I32>("rendering.MSAAsamples", 0), 0));
    par.setParam("rendering.FXAAsamples",
                 std::max(pt.get<I32>("rendering.FXAAsamples", 0), 0));
    par.setParam("GUI.CEGUI.ExtraStates",
                 pt.get("GUI.CEGUI.ExtraStates", false));
    par.setParam("GUI.CEGUI.SkipRendering",
                 pt.get("GUI.CEGUI.SkipRendering", false));
    par.setParam("GUI.defaultScheme", pt.get("GUI.defaultGUIScheme", "GWEN"));
    par.setParam("GUI.consoleLayout",
                 pt.get("GUI.consoleLayoutFile", "console.layout"));
    par.setParam("GUI.editorLayout",
                 pt.get("GUI.editorLayoutFile", "editor.layout"));
    par.setParam(
        "rendering.anisotropicFilteringLevel",
        std::max(pt.get<I32>("rendering.anisotropicFilteringLevel", 1), 1));
    par.setParam("rendering.shadowDetailLevel", shadowDetailLevel);
    par.setParam("rendering.enableFog", pt.get("rendering.enableFog", true));
    vec2<U16> resolution(pt.get("runtime.resolutionWidth", 1024),
                         pt.get("runtime.resolutionHeight", 768));
    par.setParam("runtime.windowedMode",
                 pt.get("rendering.windowedMode", true));
    par.setParam("runtime.allowWindowResize",
                 pt.get("runtime.allowWindowResize", false));
    par.setParam("runtime.enableVSync", pt.get("runtime.enableVSync", false));
    par.setParam("runtime.groundPos",
                 pt.get("runtime.groundPos",
                        -2000.0f));  ///<Safety net for physics actors
    par.setParam("postProcessing.anaglyphOffset",
                 pt.get("rendering.anaglyphOffset", 0.16f));
    par.setParam("postProcessing.enableNoise",
                 pt.get("rendering.enableNoise", false));
    par.setParam("postProcessing.enableDepthOfField",
                 pt.get("rendering.enableDepthOfField", false));
    par.setParam("postProcessing.enableBloom",
                 pt.get("rendering.enableBloom", false));
    par.setParam("postProcessing.enableSSAO",
                 pt.get("rendering.enableSSAO", false));
    par.setParam("postProcessing.bloomFactor",
                 pt.get("rendering.bloomFactor", 0.4f));
    par.setParam("mesh.playAnimations", pt.get("mesh.playAnimations", true));
    par.setParam("rendering.verticalFOV", pt.get("runtime.verticalFOV", 60.0f));
    par.setParam("rendering.zNear", pt.get("runtime.zNear", 0.1f));
    par.setParam("rendering.zFar", pt.get("runtime.zFar", 700.0f));
    Application::getInstance().setResolution(resolution.width,
                                             resolution.height);

    // global fog values
    par.setParam("rendering.sceneState.fogDensity",
                 pt.get("rendering.fogDensity", 0.01f));
    par.setParam("rendering.sceneState.fogColor.r",
                 pt.get<F32>("rendering.fogColor.<xmlattr>.r", 0.2f));
    par.setParam("rendering.sceneState.fogColor.g",
                 pt.get<F32>("rendering.fogColor.<xmlattr>.g", 0.2f));
    par.setParam("rendering.sceneState.fogColor.b",
                 pt.get<F32>("rendering.fogColor.<xmlattr>.b", 0.2f));
}

void loadScene(const std::string &sceneName, SceneManager &sceneMgr) {
    ParamHandler &par = ParamHandler::getInstance();
    pt.clear();
    Console::printfn(Locale::get("XML_LOAD_SCENE"), sceneName.c_str());
    std::string sceneLocation(
        par.getParam<std::string>("scriptLocation") + "/" +
        par.getParam<std::string>("scenesLocation") + "/" + sceneName);
    try {
        read_xml(sceneLocation + ".xml", pt);
    } catch (boost::property_tree::xml_parser_error &e) {
        Console::errorfn(Locale::get("ERROR_XML_INVALID_SCENE"),
                         sceneName.c_str());
        std::string error(e.what());
        error += " [check error log!]";
        throw error.c_str();
    }

    par.setParam("currentScene", sceneName);
    Scene *scene = sceneMgr.createScene(sceneName.c_str());

    if (!scene) {
        Console::errorfn(Locale::get("ERROR_XML_LOAD_INVALID_SCENE"));
        return;
    }

    sceneMgr.setActiveScene(scene);
    scene->setName(sceneName.c_str());
    scene->state().getGrassVisibility() =
        pt.get("vegetation.grassVisibility", 1000.0f);
    scene->state().getTreeVisibility() =
        pt.get("vegetation.treeVisibility", 1000.0f);
    scene->state().getGeneralVisibility() =
        pt.get("options.visibility", 1000.0f);

    scene->state().getWindDirX() = pt.get("wind.windDirX", 1.0f);
    scene->state().getWindDirZ() = pt.get("wind.windDirZ", 1.0f);
    scene->state().getWindSpeed() = pt.get("wind.windSpeed", 1.0f);

    scene->state().getWaterLevel() = pt.get("water.waterLevel", 0.0f);
    scene->state().getWaterDepth() = pt.get("water.waterDepth", -75);

    if (boost::optional<ptree &> cameraPositionOverride =
            pt.get_child_optional("options.cameraStartPosition")) {
        par.setParam("options.cameraStartPosition.x",
                     pt.get("options.cameraStartPosition.<xmlattr>.x", 0.0f));
        par.setParam("options.cameraStartPosition.y",
                     pt.get("options.cameraStartPosition.<xmlattr>.y", 0.0f));
        par.setParam("options.cameraStartPosition.z",
                     pt.get("options.cameraStartPosition.<xmlattr>.z", 0.0f));
        par.setParam(
            "options.cameraStartOrientation.xOffsetDegrees",
            pt.get("options.cameraStartPosition.<xmlattr>.xOffsetDegrees",
                   0.0f));
        par.setParam(
            "options.cameraStartOrientation.yOffsetDegrees",
            pt.get("options.cameraStartPosition.<xmlattr>.yOffsetDegrees",
                   0.0f));
        par.setParam("options.cameraStartPositionOverride", true);
    } else {
        par.setParam("options.cameraStartPositionOverride", false);
    }

    if (boost::optional<ptree &> physicsCook =
            pt.get_child_optional("options.autoCookPhysicsAssets")) {
        par.setParam("options.autoCookPhysicsAssets",
                     pt.get<bool>("options.autoCookPhysicsAssets", false));
    } else {
        par.setParam("options.autoCookPhysicsAssets", false);
    }

    if (boost::optional<ptree &> cameraPositionOverride =
            pt.get_child_optional("options.cameraSpeed")) {
        par.setParam("options.cameraSpeed.move",
                     pt.get("options.cameraSpeed.<xmlattr>.move", 35.0f));
        par.setParam("options.cameraSpeed.turn",
                     pt.get("options.cameraSpeed.<xmlattr>.turn", 35.0f));
    } else {
        par.setParam("options.cameraSpeed.move", 35.0f);
        par.setParam("options.cameraSpeed.turn", 35.0f);
    }

    if (boost::optional<ptree &> fog = pt.get_child_optional("fog")) {
        par.setParam("rendering.sceneState.fogDensity",
                     pt.get("fog.fogDensity", 0.01f));
        par.setParam("rendering.sceneState.fogColor.r",
                     pt.get<F32>("fog.fogColor.<xmlattr>.r", 0.2f));
        par.setParam("rendering.sceneState.fogColor.g",
                     pt.get<F32>("fog.fogColor.<xmlattr>.g", 0.2f));
        par.setParam("rendering.sceneState.fogColor.b",
                     pt.get<F32>("fog.fogColor.<xmlattr>.b", 0.2f));
    }

    scene->state().getFogDesc()._fogDensity =
        par.getParam<F32>("rendering.sceneState.fogDensity");
    scene->state().getFogDesc()._fogColor.set(
        par.getParam<F32>("rendering.sceneState.fogColor.r"),
        par.getParam<F32>("rendering.sceneState.fogColor.g"),
        par.getParam<F32>("rendering.sceneState.fogColor.b"));

    loadTerrain(sceneLocation + "/" + pt.get("terrain", "terrain.xml"), scene);
    loadGeometry(sceneLocation + "/" + pt.get("assets", "assets.xml"), scene);
}

void loadTerrain(const std::string &file, Scene *const scene) {
    U8 count = 0;
    pt.clear();
    Console::printfn(Locale::get("XML_LOAD_TERRAIN"), file.c_str());
    read_xml(file, pt);
    ptree::iterator itTerrain;
    ptree::iterator itTexture;
    stringImpl assetLocation(
        ParamHandler::getInstance().getParam<stringImpl>("assetsLocation") +
        "/");

    for (itTerrain = std::begin(pt.get_child("terrainList"));
         itTerrain != std::end(pt.get_child("terrainList")); ++itTerrain) {
        // The actual terrain name
        std::string name = itTerrain->second.data();
        // The <name> tag for valid terrains or <xmlcomment> for comments
        std::string tag = itTerrain->first.data();
        // Check and skip commented terrain
        if (tag.find("<xmlcomment>") != std::string::npos) {
            continue;
        }
        // Load the rest of the terrain
        TerrainDescriptor *ter = CreateResource<TerrainDescriptor>(
            ResourceDescriptor(stringAlg::toBase(name + "_descriptor")));
        ter->addVariable("terrainName", stringAlg::toBase(name));
        ter->addVariable("heightmap",
                         assetLocation + stringAlg::toBase(pt.get<std::string>(
                                             name + ".heightmap")));
        ter->addVariable("waterCaustics",
                         assetLocation + stringAlg::toBase(pt.get<std::string>(
                                             name + ".waterCaustics")));
        ter->addVariable(
            "underwaterAlbedoTexture",
            assetLocation + stringAlg::toBase(pt.get<std::string>(
                                name + ".underwaterAlbedoTexture")));
        ter->addVariable(
            "underwaterDetailTexture",
            assetLocation + stringAlg::toBase(pt.get<std::string>(
                                name + ".underwaterDetailTexture")));

        ter->addVariable("underwaterDiffuseScale",
                         pt.get<F32>(name + ".underwaterDiffuseScale"));

        I32 i = 0;
        stringImpl temp;
        stringImpl layerOffsetStr;
        for (itTexture = std::begin(pt.get_child(name + ".textureLayers"));
             itTexture != std::end(pt.get_child(name + ".textureLayers"));
             ++itTexture, ++i) {
            std::string layerName(itTexture->second.data());
            stringImpl format(stringAlg::toBase(itTexture->first.data()));

            if (format.find("<xmlcomment>") != std::string::npos) {
                i--;
                continue;
            }

            layerName = name + ".textureLayers." + format.c_str();

            layerOffsetStr = std::to_string(i);
            temp = stringAlg::toBase(
                pt.get<std::string>(layerName + ".blendMap", ""));
            DIVIDE_ASSERT(!temp.empty(), "Blend Map for terrain missing!");
            ter->addVariable("blendMap" + layerOffsetStr, assetLocation + temp);

            temp = stringAlg::toBase(
                pt.get<std::string>(layerName + ".redAlbedo", ""));
            if (!temp.empty()) {
                ter->addVariable("redAlbedo" + layerOffsetStr,
                                 assetLocation + temp);
            }
            temp = stringAlg::toBase(
                pt.get<std::string>(layerName + ".redDetail", ""));
            if (!temp.empty()) {
                ter->addVariable("redDetail" + layerOffsetStr,
                                 assetLocation + temp);
            }
            temp = stringAlg::toBase(
                pt.get<std::string>(layerName + ".greenAlbedo", ""));
            if (!temp.empty()) {
                ter->addVariable("greenAlbedo" + layerOffsetStr,
                                 assetLocation + temp);
            }
            temp = stringAlg::toBase(
                pt.get<std::string>(layerName + ".greenDetail", ""));
            if (!temp.empty()) {
                ter->addVariable("greenDetail" + layerOffsetStr,
                                 assetLocation + temp);
            }
            temp = stringAlg::toBase(
                pt.get<std::string>(layerName + ".blueAlbedo", ""));
            if (!temp.empty()) {
                ter->addVariable("blueAlbedo" + layerOffsetStr,
                                 assetLocation + temp);
            }
            temp = stringAlg::toBase(
                pt.get<std::string>(layerName + ".blueDetail", ""));
            if (!temp.empty()) {
                ter->addVariable("blueDetail" + layerOffsetStr,
                                 assetLocation + temp);
            }
            temp = stringAlg::toBase(
                pt.get<std::string>(layerName + ".alphaAlbedo", ""));
            if (!temp.empty()) {
                ter->addVariable("alphaAlbedo" + layerOffsetStr,
                                 assetLocation + temp);
            }
            temp = stringAlg::toBase(
                pt.get<std::string>(layerName + ".alphaDetail", ""));
            if (!temp.empty()) {
                ter->addVariable("alphaDetail" + layerOffsetStr,
                                 assetLocation + temp);
            }

            ter->addVariable("diffuseScaleR" + layerOffsetStr,
                             pt.get<F32>(layerName + ".redDiffuseScale", 0.0f));
            ter->addVariable("detailScaleR" + layerOffsetStr,
                             pt.get<F32>(layerName + ".redDetailScale", 0.0f));
            ter->addVariable(
                "diffuseScaleG" + layerOffsetStr,
                pt.get<F32>(layerName + ".greenDiffuseScale", 0.0f));
            ter->addVariable(
                "detailScaleG" + layerOffsetStr,
                pt.get<F32>(layerName + ".greenDetailScale", 0.0f));
            ter->addVariable(
                "diffuseScaleB" + layerOffsetStr,
                pt.get<F32>(layerName + ".blueDiffuseScale", 0.0f));
            ter->addVariable("detailScaleB" + layerOffsetStr,
                             pt.get<F32>(layerName + ".blueDetailScale", 0.0f));
            ter->addVariable(
                "diffuseScaleA" + layerOffsetStr,
                pt.get<F32>(layerName + ".alphaDiffuseScale", 0.0f));
            ter->addVariable(
                "detailScaleA" + layerOffsetStr,
                pt.get<F32>(layerName + ".alphaDetailScale", 0.0f));
        }

        ter->setTextureLayerCount(i);
        ter->addVariable("grassMap",
                         assetLocation + stringAlg::toBase(pt.get<std::string>(
                                             name + ".vegetation.map")));
        ter->addVariable(
            "grassBillboard1",
            assetLocation + stringAlg::toBase(pt.get<std::string>(
                                name + ".vegetation.grassBillboard1", "")));
        ter->addVariable(
            "grassBillboard2",
            assetLocation + stringAlg::toBase(pt.get<std::string>(
                                name + ".vegetation.grassBillboard2", "")));
        ter->addVariable(
            "grassBillboard3",
            assetLocation + stringAlg::toBase(pt.get<std::string>(
                                name + ".vegetation.grassBillboard3", "")));
        ter->addVariable(
            "grassBillboard4",
            assetLocation + stringAlg::toBase(pt.get<std::string>(
                                name + ".vegetation.grassBillboard4", "")));
        ter->setGrassDensity(
            pt.get<F32>(name + ".vegetation.<xmlattr>.grassDensity"));
        ter->setTreeDensity(
            pt.get<F32>(name + ".vegetation.<xmlattr>.treeDensity"));
        ter->setGrassScale(
            pt.get<F32>(name + ".vegetation.<xmlattr>.grassScale"));
        ter->setTreeScale(
            pt.get<F32>(name + ".vegetation.<xmlattr>.treeScale"));
        ter->set16Bit(pt.get<bool>(name + ".is16Bit", false));
        ter->setPosition(
            vec3<F32>(pt.get<F32>(name + ".position.<xmlattr>.x", 0.0f),
                      pt.get<F32>(name + ".position.<xmlattr>.y", 0.0f),
                      pt.get<F32>(name + ".position.<xmlattr>.z", 0.0f)));
        ter->setScale(vec2<F32>(pt.get<F32>(name + ".scale", 1.0f),
                                pt.get<F32>(name + ".heightFactor", 1.0f)));
        ter->setDimensions(vec2<U16>(pt.get<U16>(name + ".terrainWidth", 0),
                                     pt.get<U16>(name + ".terrainHeight", 0)));
        ter->setAltitudeRange(vec2<F32>(
            pt.get<F32>(name + ".altitudeRange.<xmlattr>.min", 0.0f),
            pt.get<F32>(name + ".altitudeRange.<xmlattr>.max", 255.0f)));
        ter->setActive(pt.get<bool>(name + ".active", true));
        ter->setChunkSize(pt.get<U32>(name + ".nodeChunkSize", 256));
        ter->setCreatePXActor(pt.get<bool>(name + ".addToPhysics", false));

        scene->addTerrain(ter);
        count++;
    }

    Console::printfn(Locale::get("XML_TERRAIN_COUNT"), count);
}

void loadGeometry(const std::string &file, Scene *const scene) {
    pt.clear();
    Console::printfn(Locale::get("XML_LOAD_GEOMETRY"), file.c_str());
    read_xml(file, pt);
    ptree::iterator it;
    std::string assetLocation =
        ParamHandler::getInstance().getParam<std::string>("assetsLocation") +
        "/";

    if (boost::optional<ptree &> geometry = pt.get_child_optional("geometry")) {
        for (it = std::begin(pt.get_child("geometry"));
             it != std::end(pt.get_child("geometry")); ++it) {
            std::string name(it->second.data());
            std::string format(it->first.data());
            if (format.find("<xmlcomment>") != std::string::npos) continue;
            FileData model;
            model.ItemName = stringAlg::toBase(name);
            model.ModelName = stringAlg::toBase(
                assetLocation + pt.get<std::string>(name + ".model"));
            model.position.x = pt.get<F32>(name + ".position.<xmlattr>.x", 1);
            model.position.y = pt.get<F32>(name + ".position.<xmlattr>.y", 1);
            model.position.z = pt.get<F32>(name + ".position.<xmlattr>.z", 1);
            model.orientation.x =
                pt.get<F32>(name + ".orientation.<xmlattr>.x");
            model.orientation.y =
                pt.get<F32>(name + ".orientation.<xmlattr>.y");
            model.orientation.z =
                pt.get<F32>(name + ".orientation.<xmlattr>.z");
            model.scale.x = pt.get<F32>(name + ".scale.<xmlattr>.x");
            model.scale.y = pt.get<F32>(name + ".scale.<xmlattr>.y");
            model.scale.z = pt.get<F32>(name + ".scale.<xmlattr>.z");
            model.type = GeometryType::GEOMETRY;
            model.version = pt.get<F32>(name + ".version");

            if (boost::optional<ptree &> child =
                    pt.get_child_optional(name + ".castsShadows")) {
                model.castsShadows =
                    pt.get<bool>(name + ".castsShadows", false);
            } else {
                model.castsShadows = true;
            }
            if (boost::optional<ptree &> child =
                    pt.get_child_optional(name + ".receivesShadows")) {
                model.receivesShadows =
                    pt.get<bool>(name + ".receivesShadows", false);
            } else {
                model.receivesShadows = true;
            }
            if (boost::optional<ptree &> child =
                    pt.get_child_optional(name + ".staticObject")) {
                model.staticUsage = pt.get<bool>(name + ".staticObject", false);
            } else {
                model.staticUsage = false;
            }
            if (boost::optional<ptree &> child =
                    pt.get_child_optional(name + ".addToNavigation")) {
                model.navigationUsage =
                    pt.get<bool>(name + ".addToNavigation", false);
            } else {
                model.navigationUsage = false;
            }
            if (boost::optional<ptree &> child =
                    pt.get_child_optional(name + ".useHighNavigationDetail")) {
                model.useHighDetailNavMesh =
                    pt.get<bool>(name + ".useHighNavigationDetail", false);
            } else {
                model.useHighDetailNavMesh = false;
            }
            if (boost::optional<ptree &> child =
                    pt.get_child_optional(name + ".addToPhysics")) {
                model.physicsUsage =
                    pt.get<bool>(name + ".addToPhysics", false);
                model.physicsPushable =
                    pt.get<bool>(name + ".addToPhysicsGroupPushable", false);
            } else {
                model.physicsUsage = false;
            }

            scene->addModel(model);
        }
    }

    if (boost::optional<ptree &> vegetation =
            pt.get_child_optional("vegetation")) {
        for (it = std::begin(pt.get_child("vegetation"));
             it != std::end(pt.get_child("vegetation")); ++it) {
            std::string name = it->second.data();
            std::string format = it->first.data();
            if (format.find("<xmlcomment>") != std::string::npos) continue;
            FileData model;
            model.ItemName = name.c_str();
            model.ModelName =
                (assetLocation + pt.get<std::string>(name + ".model")).c_str();
            model.position.x = pt.get<F32>(name + ".position.<xmlattr>.x");
            model.position.y = pt.get<F32>(name + ".position.<xmlattr>.y");
            model.position.z = pt.get<F32>(name + ".position.<xmlattr>.z");
            model.orientation.x =
                pt.get<F32>(name + ".orientation.<xmlattr>.x");
            model.orientation.y =
                pt.get<F32>(name + ".orientation.<xmlattr>.y");
            model.orientation.z =
                pt.get<F32>(name + ".orientation.<xmlattr>.z");
            model.scale.x = pt.get<F32>(name + ".scale.<xmlattr>.x");
            model.scale.y = pt.get<F32>(name + ".scale.<xmlattr>.y");
            model.scale.z = pt.get<F32>(name + ".scale.<xmlattr>.z");
            model.type = GeometryType::VEGETATION;
            model.version = pt.get<F32>(name + ".version");
            if (boost::optional<ptree &> child =
                    pt.get_child_optional(name + ".castsShadows")) {
                model.castsShadows =
                    pt.get<bool>(name + ".castsShadows", false);
            } else {
                model.castsShadows = true;
            }
            if (boost::optional<ptree &> child =
                    pt.get_child_optional(name + ".receivesShadows")) {
                model.receivesShadows =
                    pt.get<bool>(name + ".receivesShadows", false);
            } else {
                model.receivesShadows = true;
            }
            if (boost::optional<ptree &> child =
                    pt.get_child_optional(name + ".staticObject")) {
                model.staticUsage = pt.get<bool>(name + ".staticObject", false);
            } else {
                model.staticUsage = false;
            }
            if (boost::optional<ptree &> child =
                    pt.get_child_optional(name + ".addToNavigation")) {
                model.navigationUsage =
                    pt.get<bool>(name + ".addToNavigation", false);
            } else {
                model.navigationUsage = false;
            }
            if (boost::optional<ptree &> child =
                    pt.get_child_optional(name + ".useHighNavigationDetail")) {
                model.useHighDetailNavMesh =
                    pt.get<bool>(name + ".useHighNavigationDetail", false);
            } else {
                model.useHighDetailNavMesh = false;
            }
            if (boost::optional<ptree &> child =
                    pt.get_child_optional(name + ".addToPhysics")) {
                model.physicsUsage =
                    pt.get<bool>(name + ".addToPhysics", false);
            } else {
                model.physicsUsage = false;
            }
            scene->addModel(model);
        }
    }

    if (boost::optional<ptree &> primitives =
            pt.get_child_optional("primitives")) {
        for (it = std::begin(pt.get_child("primitives"));
             it != std::end(pt.get_child("primitives")); ++it) {
            std::string name(it->second.data());
            std::string format(it->first.data());
            if (format.find("<xmlcomment>") != std::string::npos) {
                continue;
            }
            FileData model;
            model.ItemName = stringAlg::toBase(name);
            model.ModelName =
                stringAlg::toBase(pt.get<std::string>(name + ".model"));
            model.position.x = pt.get<F32>(name + ".position.<xmlattr>.x");
            model.position.y = pt.get<F32>(name + ".position.<xmlattr>.y");
            model.position.z = pt.get<F32>(name + ".position.<xmlattr>.z");
            model.orientation.x =
                pt.get<F32>(name + ".orientation.<xmlattr>.x");
            model.orientation.y =
                pt.get<F32>(name + ".orientation.<xmlattr>.y");
            model.orientation.z =
                pt.get<F32>(name + ".orientation.<xmlattr>.z");
            model.scale.x = pt.get<F32>(name + ".scale.<xmlattr>.x");
            model.scale.y = pt.get<F32>(name + ".scale.<xmlattr>.y");
            model.scale.z = pt.get<F32>(name + ".scale.<xmlattr>.z");
            model.color.r = pt.get<F32>(name + ".color.<xmlattr>.r");
            model.color.g = pt.get<F32>(name + ".color.<xmlattr>.g");
            model.color.b = pt.get<F32>(name + ".color.<xmlattr>.b");
            /*The data variable stores a float variable (not void*) that can
             * represent anything you want*/
            /*For Text3D, it's the line width and for Box3D it's the edge
             * length*/
            if (model.ModelName.compare("Text3D") == 0) {
                model.data = pt.get<F32>(name + ".lineWidth");
                model.data2 =
                    stringAlg::toBase(pt.get<std::string>(name + ".text"));
                model.data3 =
                    stringAlg::toBase(pt.get<std::string>(name + ".fontName"));
            } else if (model.ModelName.compare("Box3D") == 0) {
                model.data = pt.get<F32>(name + ".size");
            } else if (model.ModelName.compare("Sphere3D") == 0) {
                model.data = pt.get<F32>(name + ".radius");
            } else {
                model.data = 0;
            }

            model.type = GeometryType::PRIMITIVE;
            model.version = pt.get<F32>(name + ".version");

            if (boost::optional<ptree &> child =
                    pt.get_child_optional(name + ".castsShadows")) {
                model.castsShadows =
                    pt.get<bool>(name + ".castsShadows", false);
            } else {
                model.castsShadows = true;
            }

            if (boost::optional<ptree &> child =
                    pt.get_child_optional(name + ".receivesShadows")) {
                model.receivesShadows =
                    pt.get<bool>(name + ".receivesShadows", false);
            } else {
                model.receivesShadows = true;
            }

            if (boost::optional<ptree &> child =
                    pt.get_child_optional(name + ".staticObject")) {
                model.staticUsage = pt.get<bool>(name + ".staticObject", false);
            } else {
                model.staticUsage = false;
            }

            if (boost::optional<ptree &> child =
                    pt.get_child_optional(name + ".addToNavigation")) {
                model.navigationUsage =
                    pt.get<bool>(name + ".addToNavigation", false);
            } else {
                model.navigationUsage = false;
            }

            if (boost::optional<ptree &> child =
                    pt.get_child_optional(name + ".useHighNavigationDetail")) {
                model.useHighDetailNavMesh =
                    pt.get<bool>(name + ".useHighNavigationDetail", false);
            } else {
                model.useHighDetailNavMesh = false;
            }
            scene->addModel(model);
        }
    }
}

Material *loadMaterial(const std::string &file) {
    ParamHandler &par = ParamHandler::getInstance();
    std::string location = par.getParam<std::string>("scriptLocation") + "/" +
                           par.getParam<std::string>("scenesLocation") + "/" +
                           par.getParam<std::string>("currentScene") +
                           "/materials/";

    return loadMaterialXML(location + file);
}

Material *loadMaterialXML(const std::string &matName, bool rendererDependent) {
    std::string materialFile(matName);
    if (rendererDependent) {
        materialFile +=
            "-" + getRendererTypeName(GFX_DEVICE.getRenderer().getType()) +
            ".xml";
    } else {
        materialFile += ".xml";
    }

    pt.clear();
    std::ifstream inp;
    inp.open(materialFile.c_str(), std::ifstream::in);

    if (inp.fail()) {
        inp.clear(std::ios::failbit);
        inp.close();
        return nullptr;
    }

    inp.close();
    bool skip = false;
    Console::printfn(Locale::get("XML_LOAD_MATERIAL"), matName.c_str());
    read_xml(materialFile, pt);

    std::string materialName =
        matName.substr(matName.rfind("/") + 1, matName.length());

    if (FindResourceImpl<Material>(materialName.c_str())) {
        skip = true;
    }

    Material *mat =
        CreateResource<Material>(ResourceDescriptor(materialName.c_str()));
    if (skip) {
        return mat;
    }

    // Skip if the material was cooked by a different renderer

    mat->setDiffuse(
        vec4<F32>(pt.get<F32>("material.diffuse.<xmlattr>.r", 0.6f),
                  pt.get<F32>("material.diffuse.<xmlattr>.g", 0.6f),
                  pt.get<F32>("material.diffuse.<xmlattr>.b", 0.6f),
                  pt.get<F32>("material.diffuse.<xmlattr>.a", 1.f)));
    mat->setAmbient(
        vec4<F32>(pt.get<F32>("material.ambient.<xmlattr>.r", 0.6f),
                  pt.get<F32>("material.ambient.<xmlattr>.g", 0.6f),
                  pt.get<F32>("material.ambient.<xmlattr>.b", 0.6f),
                  pt.get<F32>("material.ambient.<xmlattr>.a", 1.f)));
    mat->setSpecular(
        vec4<F32>(pt.get<F32>("material.specular.<xmlattr>.r", 1.f),
                  pt.get<F32>("material.specular.<xmlattr>.g", 1.f),
                  pt.get<F32>("material.specular.<xmlattr>.b", 1.f),
                  pt.get<F32>("material.specular.<xmlattr>.a", 1.f)));
    mat->setEmissive(
        vec3<F32>(pt.get<F32>("material.emissive.<xmlattr>.r", 1.f),
                  pt.get<F32>("material.emissive.<xmlattr>.g", 1.f),
                  pt.get<F32>("material.emissive.<xmlattr>.b", 1.f)));

    mat->setShininess(pt.get<F32>("material.shininess.<xmlattr>.v", 50.f));

    mat->setDoubleSided(pt.get<bool>("material.doubleSided", false));
    mat->setShadingMode(Material::ShadingMode::SHADING_BLINN_PHONG);
    if (boost::optional<ptree &> child =
            pt.get_child_optional("diffuseTexture1")) {
        mat->setTexture(ShaderProgram::TextureUsage::TEXTURE_UNIT0,
                        loadTextureXML("diffuseTexture1",
                                       pt.get("diffuseTexture1.file", "none")));
    }

    if (boost::optional<ptree &> child =
            pt.get_child_optional("diffuseTexture2")) {
        mat->setTexture(ShaderProgram::TextureUsage::TEXTURE_UNIT1,
                        loadTextureXML("diffuseTexture2",
                                       pt.get("diffuseTexture2.file", "none")),
                        getTextureOperation(
                            pt.get<std::string>("diffuseTexture2.operation",
                                                "TEX_OP_MULTIPLY").c_str()));
    }

    if (boost::optional<ptree &> child = pt.get_child_optional("bumpMap")) {
        mat->setTexture(
            ShaderProgram::TextureUsage::TEXTURE_NORMALMAP,
            loadTextureXML("bumpMap", pt.get("bumpMap.file", "none")));
        if (boost::optional<ptree &> child =
                pt.get_child_optional("bumpMap.method")) {
            mat->setBumpMethod(getBumpMethod(
                pt.get<std::string>("bumpMap.method", "BUMP_NORMAL").c_str()));
        }
    }

    if (boost::optional<ptree &> child = pt.get_child_optional("opacityMap")) {
        mat->setTexture(
            ShaderProgram::TextureUsage::TEXTURE_OPACITY,
            loadTextureXML("opacityMap", pt.get("opacityMap.file", "none")));
    }

    if (boost::optional<ptree &> child = pt.get_child_optional("specularMap")) {
        mat->setTexture(
            ShaderProgram::TextureUsage::TEXTURE_SPECULAR,
            loadTextureXML("specularMap", pt.get("specularMap.file", "none")));
    }

    return mat;
}

void dumpMaterial(Material &mat) {
    if (!mat.isDirty()) {
        return;
    }

    ptree pt_writer;
    ParamHandler &par = ParamHandler::getInstance();
    std::string file(mat.getName().c_str());
    file = file.substr(file.rfind("/") + 1, file.length());

    std::string location(par.getParam<std::string>("scriptLocation") + "/" +
                         par.getParam<std::string>("scenesLocation") + "/" +
                         par.getParam<std::string>("currentScene") +
                         "/materials/");

    std::string fileLocation(
        location + file + "-" +
        getRendererTypeName(GFX_DEVICE.getRenderer().getType()) + ".xml");
    pt_writer.clear();
    pt_writer.put("material.name", file);
    pt_writer.put("material.ambient.<xmlattr>.r",
                  mat.getShaderData()._ambient.r);
    pt_writer.put("material.ambient.<xmlattr>.g",
                  mat.getShaderData()._ambient.g);
    pt_writer.put("material.ambient.<xmlattr>.b",
                  mat.getShaderData()._ambient.b);
    pt_writer.put("material.ambient.<xmlattr>.a",
                  mat.getShaderData()._ambient.a);
    pt_writer.put("material.diffuse.<xmlattr>.r",
                  mat.getShaderData()._diffuse.r);
    pt_writer.put("material.diffuse.<xmlattr>.g",
                  mat.getShaderData()._diffuse.g);
    pt_writer.put("material.diffuse.<xmlattr>.b",
                  mat.getShaderData()._diffuse.b);
    pt_writer.put("material.diffuse.<xmlattr>.a",
                  mat.getShaderData()._diffuse.a);
    pt_writer.put("material.specular.<xmlattr>.r",
                  mat.getShaderData()._specular.r);
    pt_writer.put("material.specular.<xmlattr>.g",
                  mat.getShaderData()._specular.g);
    pt_writer.put("material.specular.<xmlattr>.b",
                  mat.getShaderData()._specular.b);
    pt_writer.put("material.specular.<xmlattr>.a",
                  mat.getShaderData()._specular.a);
    pt_writer.put("material.shininess.<xmlattr>.v",
                  mat.getShaderData()._shininess);
    pt_writer.put("material.emissive.<xmlattr>.r",
                  mat.getShaderData()._emissive.y);
    pt_writer.put("material.emissive.<xmlattr>.g",
                  mat.getShaderData()._emissive.z);
    pt_writer.put("material.emissive.<xmlattr>.b",
                  mat.getShaderData()._emissive.w);
    pt_writer.put("material.doubleSided", mat.isDoubleSided());

    Texture *texture = nullptr;

    if ((texture = mat.getTexture(
             ShaderProgram::TextureUsage::TEXTURE_UNIT0)) != nullptr) {
        saveTextureXML("diffuseTexture1", texture, pt_writer);
    }

    if ((texture = mat.getTexture(
             ShaderProgram::TextureUsage::TEXTURE_UNIT1)) != nullptr) {
        saveTextureXML("diffuseTexture2", texture, pt_writer,
                       getTextureOperationName(mat.getTextureOperation()));
    }

    if ((texture = mat.getTexture(
             ShaderProgram::TextureUsage::TEXTURE_NORMALMAP)) != nullptr) {
        saveTextureXML("bumpMap", texture, pt_writer);
        pt_writer.put("bumpMap.method", getBumpMethodName(mat.getBumpMethod()));
    }

    if ((texture = mat.getTexture(
             ShaderProgram::TextureUsage::TEXTURE_OPACITY)) != nullptr) {
        saveTextureXML("opacityMap", texture, pt_writer);
    }

    if ((texture = mat.getTexture(
             ShaderProgram::TextureUsage::TEXTURE_SPECULAR)) != nullptr) {
        saveTextureXML("specularMap", texture, pt_writer);
    }

    ShaderProgram *shaderProg = mat.getShaderInfo().getProgram();
    if (shaderProg) {
        pt_writer.put("shaderProgram.effect",
                      stringAlg::fromBase(shaderProg->getName()));
    }

    shaderProg = mat.getShaderInfo(RenderStage::SHADOW_STAGE).getProgram();
    if (shaderProg) {
        pt_writer.put("shaderProgram.shadowEffect",
                      stringAlg::fromBase(shaderProg->getName()));
    }

    shaderProg = mat.getShaderInfo(RenderStage::Z_PRE_PASS_STAGE).getProgram();
    if (shaderProg) {
        pt_writer.put("shaderProgram.zPrePassEffect",
                      stringAlg::fromBase(shaderProg->getName()));
    }

    FILE *xml = fopen(fileLocation.c_str(), "w");
    fclose(xml);
    write_xml(fileLocation, pt_writer, std::locale(),
              boost::property_tree::xml_writer_make_settings<ptree::key_type>(
                  '\t', 1));
}
};  // namespace XML
};  // namespace Divide