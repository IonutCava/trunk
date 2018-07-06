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
static ptree pt;

namespace {
const char *getFilterName(TextureFilter filter) {
    if (filter == TextureFilter::LINEAR) {
        return "LINEAR";
    } else if (filter == TextureFilter::NEAREST_MIPMAP_NEAREST) {
        return "NEAREST_MIPMAP_NEAREST";
    } else if (filter == TextureFilter::LINEAR_MIPMAP_NEAREST) {
        return "LINEAR_MIPMAP_NEAREST";
    } else if (filter == TextureFilter::NEAREST_MIPMAP_LINEAR) {
        return "NEAREST_MIPMAP_LINEAR";
    } else if (filter == TextureFilter::LINEAR_MIPMAP_LINEAR) {
        return "LINEAR_MIPMAP_LINEAR";
    }

    return "NEAREST";
}

TextureFilter getFilter(const stringImpl& filter) {
    if (filter.compare("LINEAR") == 0) {
        return TextureFilter::LINEAR;
    } else if (filter.compare("NEAREST_MIPMAP_NEAREST") == 0) {
        return TextureFilter::NEAREST_MIPMAP_NEAREST;
    } else if (filter.compare("LINEAR_MIPMAP_NEAREST") == 0) {
        return TextureFilter::LINEAR_MIPMAP_NEAREST;
    } else if (filter.compare("NEAREST_MIPMAP_LINEAR") == 0) {
        return TextureFilter::NEAREST_MIPMAP_LINEAR;
    } else if (filter.compare("LINEAR_MIPMAP_LINEAR") == 0) {
        return TextureFilter::LINEAR_MIPMAP_LINEAR;
    }

    return TextureFilter::NEAREST;
}

const char *getWrapModeName(TextureWrap wrapMode) {
    if (wrapMode == TextureWrap::CLAMP) {
        return "CLAMP";
    } else if (wrapMode == TextureWrap::CLAMP_TO_EDGE) {
        return "CLAMP_TO_EDGE";
    } else if (wrapMode == TextureWrap::CLAMP_TO_BORDER) {
        return "CLAMP_TO_BORDER";
    } else if (wrapMode == TextureWrap::DECAL) {
        return "DECAL";
    }

    return "REPEAT";
}

TextureWrap getWrapMode(const stringImpl& wrapMode) {
    if (wrapMode.compare("CLAMP") == 0) {
        return TextureWrap::CLAMP;
    } else if (wrapMode.compare("CLAMP_TO_EDGE") == 0) {
        return TextureWrap::CLAMP_TO_EDGE;
    } else if (wrapMode.compare("CLAMP_TO_BORDER") == 0) {
        return TextureWrap::CLAMP_TO_BORDER;
    } else if (wrapMode.compare("DECAL") == 0) {
        return TextureWrap::DECAL;
    }

    return TextureWrap::REPEAT;
}

const char *getBumpMethodName(Material::BumpMethod bumpMethod) {
    if (bumpMethod == Material::BumpMethod::NORMAL) {
        return "NORMAL";
    } else if (bumpMethod == Material::BumpMethod::PARALLAX) {
        return "PARALLAX";
    } else if (bumpMethod == Material::BumpMethod::RELIEF) {
        return "RELIEF";
    }

    return "NONE";
}

Material::BumpMethod getBumpMethod(const stringImpl& bumpMethod) {
    if (bumpMethod.compare("NORMAL") == 0) {
        return Material::BumpMethod::NORMAL;
    } else if (bumpMethod.compare("PARALLAX") == 0) {
        return Material::BumpMethod::PARALLAX;
    } else if (bumpMethod.compare("RELIEF") == 0) {
        return Material::BumpMethod::RELIEF;
    }

    return Material::BumpMethod::NONE;
}

const char *getTextureOperationName(Material::TextureOperation textureOp) {
    if (textureOp == Material::TextureOperation::MULTIPLY) {
        return "TEX_OP_MULTIPLY";
    } else if (textureOp ==
               Material::TextureOperation::DECAL) {
        return "TEX_OP_DECAL";
    } else if (textureOp == Material::TextureOperation::ADD) {
        return "TEX_OP_ADD";
    } else if (textureOp ==
               Material::TextureOperation::SMOOTH_ADD) {
        return "TEX_OP_SMOOTH_ADD";
    } else if (textureOp ==
               Material::TextureOperation::SIGNED_ADD) {
        return "TEX_OP_SIGNED_ADD";
    } else if (textureOp ==
               Material::TextureOperation::DIVIDE) {
        return "TEX_OP_DIVIDE";
    } else if (textureOp ==
               Material::TextureOperation::SUBTRACT) {
        return "TEX_OP_SUBTRACT";
    }

    return "TEX_OP_REPLACE";
}

Material::TextureOperation getTextureOperation(const stringImpl& operation) {
    if (operation.compare("TEX_OP_MULTIPLY") == 0) {
        return Material::TextureOperation::MULTIPLY;
    } else if (operation.compare("TEX_OP_DECAL") == 0) {
        return Material::TextureOperation::DECAL;
    } else if (operation.compare("TEX_OP_ADD") == 0) {
        return Material::TextureOperation::ADD;
    } else if (operation.compare("TEX_OP_SMOOTH_ADD") == 0) {
        return Material::TextureOperation::SMOOTH_ADD;
    } else if (operation.compare("TEX_OP_SIGNED_ADD") == 0) {
        return Material::TextureOperation::SIGNED_ADD;
    } else if (operation.compare("TEX_OP_DIVIDE") == 0) {
        return Material::TextureOperation::DIVIDE;
    } else if (operation.compare("TEX_OP_SUBTRACT") == 0) {
        return Material::TextureOperation::SUBTRACT;
    }

    return Material::TextureOperation::REPLACE;
}

void saveTextureXML(const stringImpl &textureNode, Texture *texture,
                    ptree &tree, const stringImpl& operation = "") {
    const SamplerDescriptor &sampler = texture->getCurrentSampler();
    WAIT_FOR_CONDITION(texture->getState() == ResourceState::RES_LOADED);

    std::string node(textureNode.c_str());
    tree.put(node + ".file", texture->getResourceLocation());
    tree.put(node + ".MapU", getWrapModeName(sampler.wrapU()));
    tree.put(node + ".MapV", getWrapModeName(sampler.wrapV()));
    tree.put(node + ".MapW", getWrapModeName(sampler.wrapW()));
    tree.put(node + ".minFilter", getFilterName(sampler.minFilter()));
    tree.put(node + ".magFilter", getFilterName(sampler.magFilter()));
    tree.put(node + ".anisotropy", to_uint(sampler.anisotropyLevel()));

    if (!operation.empty()) {
        tree.put(node + ".operation", operation);
    }
}

Texture *loadTextureXML(const stringImpl &textureNode,
                        const stringImpl &textureName) {
    stringImpl img_name(textureName.substr(textureName.find_last_of('/') + 1));
    stringImpl pathName(textureName.substr(0, textureName.rfind("/") + 1));
    std::string node(textureNode.c_str());

    TextureWrap wrapU = getWrapMode(
        pt.get<stringImpl>(node + ".MapU", "REPEAT"));
    TextureWrap wrapV = getWrapMode(
        pt.get<stringImpl>(node + ".MapV", "REPEAT"));
    TextureWrap wrapW = getWrapMode(
        pt.get<stringImpl>(node + ".MapW", "REPEAT"));
    TextureFilter minFilterValue =
        getFilter(pt.get<stringImpl>(node + ".minFilter", "LINEAR"));
    TextureFilter magFilterValue =
        getFilter(pt.get<stringImpl>(node + ".magFilter", "LINEAR"));

    U8 anisotropy = to_ubyte(pt.get(node + ".anisotropy", 0U));

    SamplerDescriptor sampDesc;
    sampDesc.setWrapMode(wrapU, wrapV, wrapW);
    sampDesc.setFilters(minFilterValue, magFilterValue);
    sampDesc.setAnisotropy(anisotropy);

    ResourceDescriptor texture(img_name);
    texture.setResourceLocation(pathName + img_name);
    texture.setPropertyDescriptor<SamplerDescriptor>(sampDesc);

    return CreateResource<Texture>(texture);
}

inline stringImpl getRendererTypeName(RendererType type) {
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

stringImpl loadScripts(const stringImpl &file) {
    ParamHandler &par = ParamHandler::instance();
    Console::printfn(Locale::get(_ID("XML_LOAD_SCRIPTS")));
    read_xml(file.c_str(), pt);
    stringImpl activeScene("MainScene");
    par.setParam(_ID("testInt"), 2);
    par.setParam(_ID("testFloat"), 3.2f);
    par.setParam(_ID("scriptLocation"),
                 pt.get<stringImpl>("scriptLocation", "XML"));
    par.setParam(_ID("assetsLocation"), pt.get<stringImpl>("assets", "assets"));
    par.setParam(_ID("scenesLocation"),
                 pt.get<stringImpl>("scenesLocation", "Scenes"));
    par.setParam(_ID("serverAddress"), pt.get<stringImpl>("server", "127.0.0.1"));
    loadConfig((par.getParam<stringImpl>(_ID("scriptLocation"), "XML") + "/" +
                stringImpl(pt.get("config", "config.xml").c_str())));
    read_xml(std::string(par.getParam<stringImpl>(_ID("scriptLocation"), "XML").c_str()) + "/" +
             pt.get("startupScene", "scenes.xml"),
             pt);
    activeScene = pt.get("StartupScene", activeScene);

    return activeScene;
}

void loadConfig(const stringImpl &file) {
    ParamHandler &par = ParamHandler::instance();
    pt.clear();
    Console::printfn(Locale::get(_ID("XML_LOAD_CONFIG")), file.c_str());
    read_xml(file.c_str(), pt);
    par.setParam(_ID("locale"), pt.get("language", "enGB"));
    par.setParam(_ID("logFile"), pt.get("debug.logFile", "none"));
    par.setParam(_ID("memFile"), pt.get("debug.memFile", "none"));
    par.setParam(_ID("simSpeed"), pt.get("runtime.simSpeed", 1.0f));
    par.setParam(_ID("appTitle"), pt.get("title", "DIVIDE Framework"));
    par.setParam(_ID("mesh.playAnimations"), pt.get("debug.mesh.playAnimations", true));
    par.setParam(_ID("defaultTextureLocation"),
                 pt.get("defaultTextureLocation", "textures/"));
    par.setParam(_ID("shaderLocation"),
                 pt.get("defaultShadersLocation", "shaders/"));

    I32 shadowDetailLevel = pt.get<I32>("rendering.shadowDetailLevel", 2);
    if (shadowDetailLevel <= 0) {
        GFX_DEVICE.shadowDetailLevel(RenderDetailLevel::LOW);
        par.setParam(_ID("rendering.enableShadows"), false);
    } else {
        GFX_DEVICE.shadowDetailLevel(
            static_cast<RenderDetailLevel>(std::min(shadowDetailLevel, 3) - 1));
        par.setParam(_ID("rendering.enableShadows"), true);
    }

    GFX_DEVICE.anaglyphEnabled(pt.get("rendering.enable3D", false));
    par.setParam(_ID("rendering.MSAAsampless"),
                 std::max(pt.get<I32>("rendering.MSAAsamples", 0), 0));
    par.setParam(_ID("rendering.PostAASamples"),
                 std::max(pt.get<I32>("rendering.PostAASamples", 0), 0));
    par.setParam(_ID("rendering.PostAAType"),
                 pt.get("rendering.PostAAType", "FXAA"));
    par.setParam(_ID("GUI.CEGUI.ExtraStates"),
                 pt.get("GUI.CEGUI.ExtraStates", false));
    par.setParam(_ID("GUI.CEGUI.SkipRendering"),
                 pt.get("GUI.CEGUI.SkipRendering", false));
    par.setParam(_ID("GUI.defaultScheme"), pt.get("GUI.defaultGUIScheme", "GWEN"));
    par.setParam(_ID("GUI.consoleLayout"),
                 pt.get("GUI.consoleLayoutFile", "console.layout"));
    par.setParam(_ID("GUI.editorLayout"),
                 pt.get("GUI.editorLayoutFile", "editor.layout"));
    par.setParam(
        _ID("rendering.anisotropicFilteringLevel"),
        std::max(pt.get<I32>("rendering.anisotropicFilteringLevel", 1), 1));
    par.setParam(_ID("rendering.shadowDetailLevel"), shadowDetailLevel);
    par.setParam(_ID("rendering.enableFog"), pt.get("rendering.enableFog", true));

    par.setParam(_ID("runtime.targetDisplay"), pt.get("runtime.targetDisplay", 0));
    par.setParam(_ID("runtime.startFullScreen"), !pt.get("rendering.windowedMode", true));
    par.setParam(_ID("runtime.windowWidth"), pt.get("runtime.resolution.<xmlattr>.w", 1024));
    par.setParam(_ID("runtime.windowHeight"), pt.get("runtime.resolution.<xmlattr>.h", 768));
    par.setParam(_ID("runtime.splashWidth"), pt.get("runtime.splashScreenSize.<xmlattr>.w", 400));
    par.setParam(_ID("runtime.splashHeight"), pt.get("runtime.splashScreenSize.<xmlattr>.h", 300));

    par.setParam(_ID("runtime.windowResizable"), pt.get("runtime.windowResizable", true));
    par.setParam(_ID("runtime.enableVSync"), pt.get("runtime.enableVSync", false));
    par.setParam(_ID("postProcessing.anaglyphOffset"),
                 pt.get("rendering.anaglyphOffset", 0.16f));
    par.setParam(_ID("postProcessing.enableNoise"),
                 pt.get("rendering.enableNoise", false));
    par.setParam(_ID("postProcessing.enableDepthOfField"),
                 pt.get("rendering.enableDepthOfField", false));
    par.setParam(_ID("postProcessing.enableBloom"),
                 pt.get("rendering.enableBloom", false));
    par.setParam(_ID("postProcessing.enableSSAO"),
                 pt.get("rendering.enableSSAO", false));
    par.setParam(_ID("postProcessing.bloomFactor"),
                 pt.get("rendering.bloomFactor", 0.4f));
    par.setParam(_ID("rendering.verticalFOV"), pt.get("runtime.verticalFOV", 60.0f));
    par.setParam(_ID("rendering.zNear"), pt.get("runtime.zNear", 0.1f));
    par.setParam(_ID("rendering.zFar"), pt.get("runtime.zFar", 700.0f));


    // global fog values
    par.setParam(_ID("rendering.sceneState.fogDensity"),
                 pt.get("rendering.fogDensity", 0.01f));
    par.setParam(_ID("rendering.sceneState.fogColor.r"),
                 pt.get<F32>("rendering.fogColor.<xmlattr>.r", 0.2f));
    par.setParam(_ID("rendering.sceneState.fogColor.g"),
                 pt.get<F32>("rendering.fogColor.<xmlattr>.g", 0.2f));
    par.setParam(_ID("rendering.sceneState.fogColor.b"),
                 pt.get<F32>("rendering.fogColor.<xmlattr>.b", 0.2f));
}

void loadScene(const stringImpl &sceneName, SceneManager &sceneMgr) {
    ParamHandler &par = ParamHandler::instance();
    pt.clear();
    Console::printfn(Locale::get(_ID("XML_LOAD_SCENE")), sceneName.c_str());
    std::string sceneLocation(
        par.getParam<std::string>(_ID("scriptLocation")) + "/" +
        par.getParam<std::string>(_ID("scenesLocation")) + "/" + sceneName.c_str());
    try {
        read_xml(sceneLocation + ".xml", pt);
    } catch (boost::property_tree::xml_parser_error &e) {
        Console::errorfn(Locale::get(_ID("ERROR_XML_INVALID_SCENE")),
                         sceneName.c_str());
        stringImpl error(e.what());
        error += " [check error log!]";
        throw error.c_str();
    }

    par.setParam(_ID("currentScene"), sceneName);
    Scene *scene = sceneMgr.createScene(sceneName);

    if (!scene) {
        Console::errorfn(Locale::get(_ID("ERROR_XML_LOAD_INVALID_SCENE")));
        return;
    }

    sceneMgr.setActiveScene(*scene);
    scene->state().grassVisibility(pt.get("vegetation.grassVisibility", 1000.0f));
    scene->state().treeVisibility(pt.get("vegetation.treeVisibility", 1000.0f));
    scene->state().generalVisibility(pt.get("options.visibility", 1000.0f));

    scene->state().windDirX(pt.get("wind.windDirX", 1.0f));
    scene->state().windDirZ(pt.get("wind.windDirZ", 1.0f));
    scene->state().windSpeed(pt.get("wind.windSpeed", 1.0f));

    scene->state().waterLevel(pt.get("water.waterLevel", 0.0f));
    scene->state().waterDepth(pt.get("water.waterDepth", -75.0f));

    if (boost::optional<ptree &> cameraPositionOverride =
            pt.get_child_optional("options.cameraStartPosition")) {
        par.setParam(_ID("options.cameraStartPosition.x"),
                     pt.get("options.cameraStartPosition.<xmlattr>.x", 0.0f));
        par.setParam(_ID("options.cameraStartPosition.y"),
                     pt.get("options.cameraStartPosition.<xmlattr>.y", 0.0f));
        par.setParam(_ID("options.cameraStartPosition.z"),
                     pt.get("options.cameraStartPosition.<xmlattr>.z", 0.0f));
        par.setParam(
            _ID("options.cameraStartOrientation.xOffsetDegrees"),
            pt.get("options.cameraStartPosition.<xmlattr>.xOffsetDegrees",
                   0.0f));
        par.setParam(
            _ID("options.cameraStartOrientation.yOffsetDegrees"),
            pt.get("options.cameraStartPosition.<xmlattr>.yOffsetDegrees",
                   0.0f));
        par.setParam(_ID("options.cameraStartPositionOverride"), true);
    } else {
        par.setParam(_ID("options.cameraStartPositionOverride"), false);
    }

    if (boost::optional<ptree &> physicsCook =
            pt.get_child_optional("options.autoCookPhysicsAssets")) {
        par.setParam(_ID("options.autoCookPhysicsAssets"),
                     pt.get<bool>("options.autoCookPhysicsAssets", false));
    } else {
        par.setParam(_ID("options.autoCookPhysicsAssets"), false);
    }

    if (boost::optional<ptree &> cameraPositionOverride =
            pt.get_child_optional("options.cameraSpeed")) {
        par.setParam(_ID("options.cameraSpeed.move"),
                     pt.get("options.cameraSpeed.<xmlattr>.move", 35.0f));
        par.setParam(_ID("options.cameraSpeed.turn"),
                     pt.get("options.cameraSpeed.<xmlattr>.turn", 35.0f));
    } else {
        par.setParam(_ID("options.cameraSpeed.move"), 35.0f);
        par.setParam(_ID("options.cameraSpeed.turn"), 35.0f);
    }

    if (boost::optional<ptree &> fog = pt.get_child_optional("fog")) {
        par.setParam(_ID("rendering.sceneState.fogDensity"),
                     pt.get("fog.fogDensity", 0.01f));
        par.setParam(_ID("rendering.sceneState.fogColor.r"),
                     pt.get<F32>("fog.fogColor.<xmlattr>.r", 0.2f));
        par.setParam(_ID("rendering.sceneState.fogColor.g"),
                     pt.get<F32>("fog.fogColor.<xmlattr>.g", 0.2f));
        par.setParam(_ID("rendering.sceneState.fogColor.b"),
                     pt.get<F32>("fog.fogColor.<xmlattr>.b", 0.2f));
    }

    scene->state().fogDescriptor()._fogDensity =
        par.getParam<F32>(_ID("rendering.sceneState.fogDensity")) / 1000.0f;
    scene->state().fogDescriptor()._fogColor.set(
        par.getParam<F32>(_ID("rendering.sceneState.fogColor.r")),
        par.getParam<F32>(_ID("rendering.sceneState.fogColor.g")),
        par.getParam<F32>(_ID("rendering.sceneState.fogColor.b")));

    loadTerrain((sceneLocation + "/" + pt.get("terrain", "terrain.xml")).c_str(), scene);
    loadGeometry((sceneLocation + "/" + pt.get("assets", "assets.xml")).c_str(), scene);
}

void loadTerrain(const stringImpl &file, Scene *const scene) {
    U8 count = 0;
    pt.clear();
    Console::printfn(Locale::get(_ID("XML_LOAD_TERRAIN")), file.c_str());
    read_xml(file.c_str(), pt);
    ptree::iterator itTerrain;
    ptree::iterator itTexture;
    stringImpl assetLocation(
        ParamHandler::instance().getParam<stringImpl>(_ID("assetsLocation")) +
        "/");

    for (itTerrain = std::begin(pt.get_child("terrainList"));
         itTerrain != std::end(pt.get_child("terrainList")); ++itTerrain) {
        // The actual terrain name
        std::string name = itTerrain->second.data();
        // The <name> tag for valid terrains or <xmlcomment> for comments
        std::string tag = itTerrain->first.data();
        // Check and skip commented terrain
        if (tag.find("<xmlcomment>") != stringImpl::npos) {
            continue;
        }
        // Load the rest of the terrain
        TerrainDescriptor *ter = CreateResource<TerrainDescriptor>(
            ResourceDescriptor((name + "_descriptor").c_str()));
        ter->addVariable("terrainName", name.c_str());
        ter->addVariable("heightmap",
                         assetLocation + pt.get<stringImpl>(name + ".heightmap"));
        ter->addVariable("waterCaustics",
                         assetLocation + pt.get<stringImpl>(name + ".waterCaustics"));
        ter->addVariable(
            "underwaterAlbedoTexture",
            assetLocation + pt.get<stringImpl>(name + ".underwaterAlbedoTexture"));
        ter->addVariable(
            "underwaterDetailTexture",
            assetLocation + pt.get<stringImpl>(name + ".underwaterDetailTexture"));

        ter->addVariable("underwaterDiffuseScale",
                         pt.get<F32>(name + ".underwaterDiffuseScale"));

        I32 i = 0;
        stringImpl temp;
        stringImpl layerOffsetStr;
        for (itTexture = std::begin(pt.get_child(name + ".textureLayers"));
             itTexture != std::end(pt.get_child(name + ".textureLayers"));
             ++itTexture, ++i) {
            std::string layerName(itTexture->second.data());
            std::string format(itTexture->first.data());

            if (format.find("<xmlcomment>") != stringImpl::npos) {
                i--;
                continue;
            }

            layerName = name + ".textureLayers." + format;

            layerOffsetStr = to_stringImpl(i);
            temp = pt.get<stringImpl>(layerName + ".blendMap", "");
            DIVIDE_ASSERT(!temp.empty(), "Blend Map for terrain missing!");
            ter->addVariable("blendMap" + layerOffsetStr, assetLocation + temp);

            temp = pt.get<stringImpl>(layerName + ".redAlbedo", "");
            if (!temp.empty()) {
                ter->addVariable("redAlbedo" + layerOffsetStr,
                                 assetLocation + temp);
            }
            temp = pt.get<stringImpl>(layerName + ".redDetail", "");
            if (!temp.empty()) {
                ter->addVariable("redDetail" + layerOffsetStr,
                                 assetLocation + temp);
            }
            temp = pt.get<stringImpl>(layerName + ".greenAlbedo", "");
            if (!temp.empty()) {
                ter->addVariable("greenAlbedo" + layerOffsetStr,
                                 assetLocation + temp);
            }
            temp = pt.get<stringImpl>(layerName + ".greenDetail", "");
            if (!temp.empty()) {
                ter->addVariable("greenDetail" + layerOffsetStr,
                                 assetLocation + temp);
            }
            temp = pt.get<stringImpl>(layerName + ".blueAlbedo", "");
            if (!temp.empty()) {
                ter->addVariable("blueAlbedo" + layerOffsetStr,
                                 assetLocation + temp);
            }
            temp = pt.get<stringImpl>(layerName + ".blueDetail", "");
            if (!temp.empty()) {
                ter->addVariable("blueDetail" + layerOffsetStr,
                                 assetLocation + temp);
            }
            temp = pt.get<stringImpl>(layerName + ".alphaAlbedo", "");
            if (!temp.empty()) {
                ter->addVariable("alphaAlbedo" + layerOffsetStr,
                                 assetLocation + temp);
            }
            temp = pt.get<stringImpl>(layerName + ".alphaDetail", "");
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

        ter->setTextureLayerCount(to_ubyte(i));
        ter->addVariable("grassMap",
                         assetLocation + pt.get<stringImpl>(name + ".vegetation.map"));
        ter->addVariable(
            "grassBillboard1",
            assetLocation + pt.get<stringImpl>(name + ".vegetation.grassBillboard1", ""));
        ter->addVariable(
            "grassBillboard2",
            assetLocation + pt.get<stringImpl>(name + ".vegetation.grassBillboard2", ""));
        ter->addVariable(
            "grassBillboard3",
            assetLocation + pt.get<stringImpl>(name + ".vegetation.grassBillboard3", ""));
        ter->addVariable(
            "grassBillboard4",
            assetLocation + pt.get<stringImpl>(name + ".vegetation.grassBillboard4", ""));
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

        scene->addTerrain(ter);
        count++;
    }

    Console::printfn(Locale::get(_ID("XML_TERRAIN_COUNT")), count);
}

void loadGeometry(const stringImpl &file, Scene *const scene) {
    pt.clear();
    Console::printfn(Locale::get(_ID("XML_LOAD_GEOMETRY")), file.c_str());
    read_xml(file.c_str(), pt);
    ptree::iterator it;
    stringImpl assetLocation =
        ParamHandler::instance().getParam<stringImpl>(_ID("assetsLocation")) +
        "/";

    if (boost::optional<ptree &> geometry = pt.get_child_optional("geometry")) {
        for (it = std::begin(pt.get_child("geometry"));
             it != std::end(pt.get_child("geometry")); ++it) {
            std::string name(it->second.data());
            std::string format(it->first.data());
            if (format.find("<xmlcomment>") != stringImpl::npos) {
                continue;
            }
            FileData model;
            model.ItemName = name.c_str();
            model.ModelName =  assetLocation + pt.get<stringImpl>(name + ".model");
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
                model.physicsStatic =
                    !pt.get<bool>(name + ".addToPhysicsGroupPushable", false);
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
            if (format.find("<xmlcomment>") != stringImpl::npos) {
                continue;
            }
            FileData model;
            model.ItemName = name.c_str();
            model.ModelName = assetLocation + pt.get<stringImpl>(name + ".model");
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
            if (format.find("<xmlcomment>") != stringImpl::npos) {
                continue;
            }
            FileData model;
            model.ItemName = name.c_str();
            model.ModelName = pt.get<stringImpl>(name + ".model");
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
                model.data2 = pt.get<stringImpl>(name + ".text");
                model.data3 = pt.get<stringImpl>(name + ".fontName");
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

Material *loadMaterial(const stringImpl &file) {
    ParamHandler &par = ParamHandler::instance();
    stringImpl location = par.getParam<stringImpl>(_ID("scriptLocation")) + "/" +
                          par.getParam<stringImpl>(_ID("scenesLocation")) + "/" +
                          par.getParam<stringImpl>(_ID("currentScene")) +
                          "/materials/";

    return loadMaterialXML(location + file);
}

Material *loadMaterialXML(const stringImpl &matName, bool rendererDependent) {
    stringImpl materialFile(matName);
    if (rendererDependent) {
        materialFile +=
            "-" + getRendererTypeName(SceneManager::instance().getRenderer().getType()) +
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
    Console::printfn(Locale::get(_ID("XML_LOAD_MATERIAL")), matName.c_str());
    read_xml(materialFile.c_str(), pt);

    stringImpl materialName =
        matName.substr(matName.rfind("/") + 1, matName.length());

    if (FindResourceImpl<Material>(materialName)) {
        skip = true;
    }

    Material *mat =
        CreateResource<Material>(ResourceDescriptor(materialName));
    if (skip) {
        return mat;
    }

    // Skip if the material was cooked by a different renderer
    mat->setShadingMode(Material::ShadingMode::BLINN_PHONG);

    mat->setDiffuse(
        vec4<F32>(pt.get<F32>("material.diffuse.<xmlattr>.r", 0.6f),
                  pt.get<F32>("material.diffuse.<xmlattr>.g", 0.6f),
                  pt.get<F32>("material.diffuse.<xmlattr>.b", 0.6f),
                  pt.get<F32>("material.diffuse.<xmlattr>.a", 1.f)));
    mat->setSpecular(
        vec4<F32>(pt.get<F32>("material.specular.<xmlattr>.r", 1.f),
                  pt.get<F32>("material.specular.<xmlattr>.g", 1.f),
                  pt.get<F32>("material.specular.<xmlattr>.b", 1.f),
                  pt.get<F32>("material.specular.<xmlattr>.a", 1.f)));
    mat->setEmissive(
        vec3<F32>(pt.get<F32>("material.emissive.<xmlattr>.r", 0.f),
                  pt.get<F32>("material.emissive.<xmlattr>.g", 0.f),
                  pt.get<F32>("material.emissive.<xmlattr>.b", 0.f)));

    mat->setShininess(pt.get<F32>("material.shininess.<xmlattr>.v", 50.f));

    mat->setDoubleSided(pt.get<bool>("material.doubleSided", false));
    if (boost::optional<ptree &> child =
            pt.get_child_optional("diffuseTexture1")) {
        mat->setTexture(ShaderProgram::TextureUsage::UNIT0,
                        loadTextureXML("diffuseTexture1",
                                       pt.get("diffuseTexture1.file", "none").c_str()));
    }

    if (boost::optional<ptree &> child =
            pt.get_child_optional("diffuseTexture2")) {
        mat->setTexture(ShaderProgram::TextureUsage::UNIT1,
                        loadTextureXML("diffuseTexture2",
                                       pt.get("diffuseTexture2.file", "none").c_str()),
                        getTextureOperation(
                            pt.get<stringImpl>("diffuseTexture2.operation",
                                               "TEX_OP_MULTIPLY")));
    }

    if (boost::optional<ptree &> child = pt.get_child_optional("bumpMap")) {
        mat->setTexture(
            ShaderProgram::TextureUsage::NORMALMAP,
            loadTextureXML("bumpMap", pt.get("bumpMap.file", "none").c_str()));
        if (child = pt.get_child_optional("bumpMap.method")) {
            mat->setBumpMethod(getBumpMethod(
                pt.get<stringImpl>("bumpMap.method", "NORMAL")));
        }
    }

    if (boost::optional<ptree &> child = pt.get_child_optional("opacityMap")) {
        mat->setTexture(
            ShaderProgram::TextureUsage::OPACITY,
            loadTextureXML("opacityMap", pt.get("opacityMap.file", "none").c_str()));
    }

    if (boost::optional<ptree &> child = pt.get_child_optional("specularMap")) {
        mat->setTexture(
            ShaderProgram::TextureUsage::SPECULAR,
            loadTextureXML("specularMap", pt.get("specularMap.file", "none").c_str()));
    }

    return mat;
}

void dumpMaterial(Material &mat) {
    if (!mat.isDirty()) {
        return;
    }

    ptree pt_writer;
    ParamHandler &par = ParamHandler::instance();
    stringImpl file(mat.getName());
    file = file.substr(file.rfind("/") + 1, file.length());

    stringImpl location(par.getParam<stringImpl>(_ID("scriptLocation")) + "/" +
                        par.getParam<stringImpl>(_ID("scenesLocation")) + "/" +
                        par.getParam<stringImpl>(_ID("currentScene")) +
                        "/materials/");

    stringImpl fileLocation(
        location + file + "-" +
        getRendererTypeName(SceneManager::instance().getRenderer().getType()) + ".xml");
    pt_writer.clear();
    pt_writer.put("material.name", file);
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
             ShaderProgram::TextureUsage::UNIT0)) != nullptr) {
        saveTextureXML("diffuseTexture1", texture, pt_writer);
    }

    if ((texture = mat.getTexture(
             ShaderProgram::TextureUsage::UNIT1)) != nullptr) {
        saveTextureXML("diffuseTexture2", texture, pt_writer,
                       getTextureOperationName(mat.getTextureOperation()));
    }

    if ((texture = mat.getTexture(
             ShaderProgram::TextureUsage::NORMALMAP)) != nullptr) {
        saveTextureXML("bumpMap", texture, pt_writer);
        pt_writer.put("bumpMap.method", getBumpMethodName(mat.getBumpMethod()));
    }

    if ((texture = mat.getTexture(
             ShaderProgram::TextureUsage::OPACITY)) != nullptr) {
        saveTextureXML("opacityMap", texture, pt_writer);
    }

    if ((texture = mat.getTexture(
             ShaderProgram::TextureUsage::SPECULAR)) != nullptr) {
        saveTextureXML("specularMap", texture, pt_writer);
    }

    ShaderProgram *shaderProg = mat.getShaderInfo().getProgram();
    if (shaderProg) {
        pt_writer.put("shaderProgram.effect", shaderProg->getName());
    }

    shaderProg = mat.getShaderInfo(RenderStage::SHADOW).getProgram();
    if (shaderProg) {
        pt_writer.put("shaderProgram.shadowEffect", shaderProg->getName());
    }

    shaderProg = mat.getShaderInfo(RenderStage::Z_PRE_PASS).getProgram();
    if (shaderProg) {
        pt_writer.put("shaderProgram.zPrePassEffect", shaderProg->getName());
    }

    FILE *xml = fopen(fileLocation.c_str(), "w");
    fclose(xml);
    write_xml(fileLocation.c_str(), pt_writer, std::locale(),
              boost::property_tree::xml_writer_make_settings<ptree::key_type>(
                  '\t', 1));
}
};  // namespace XML
};  // namespace Divide
