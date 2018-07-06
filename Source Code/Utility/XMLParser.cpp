#include "Headers/XMLParser.h"

#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/XMLEntryData.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Scenes/Headers/SceneInput.h"
#include "Rendering/Headers/Renderer.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Material/Headers/Material.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"

namespace Divide {
namespace XML {

using boost::property_tree::ptree;

bool loadFromXML(IXMLSerializable& object, const char* file) {
    return object.fromXML(file);
}

bool saveToXML(const IXMLSerializable& object, const char* file) {
    return object.toXML(file);
}

namespace {
const ptree& empty_ptree() {
    static ptree t;
    return t;
}

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

void saveTextureXML(const stringImpl &textureNode, std::weak_ptr<Texture> texturePtr, ptree &tree, const stringImpl& operation = "") {
    Texture_ptr texture = texturePtr.lock();

    const SamplerDescriptor &sampler = texture->getCurrentSampler();
    WAIT_FOR_CONDITION(texture->getState() == ResourceState::RES_LOADED);

    std::string node(textureNode.c_str());
    tree.put(node + ".file", texture->getResourceLocation());
    tree.put(node + ".MapU", getWrapModeName(sampler.wrapU()));
    tree.put(node + ".MapV", getWrapModeName(sampler.wrapV()));
    tree.put(node + ".MapW", getWrapModeName(sampler.wrapW()));
    tree.put(node + ".minFilter", getFilterName(sampler.minFilter()));
    tree.put(node + ".magFilter", getFilterName(sampler.magFilter()));
    tree.put(node + ".anisotropy", to_U32(sampler.anisotropyLevel()));

    if (!operation.empty()) {
        tree.put(node + ".operation", operation);
    }
}

Texture_ptr loadTextureXML(ResourceCache& targetCache,
                           const stringImpl &textureNode,
                           const stringImpl &textureName,
                           const ptree& pt) {
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

    U8 anisotropy = to_U8(pt.get(node + ".anisotropy", 0U));

    SamplerDescriptor sampDesc;
    sampDesc.setWrapMode(wrapU, wrapV, wrapW);
    sampDesc.setFilters(minFilterValue, magFilterValue);
    sampDesc.setAnisotropy(anisotropy);

    ResourceDescriptor texture(img_name);
    texture.setResourceLocation(pathName + img_name);
    texture.setPropertyDescriptor<SamplerDescriptor>(sampDesc);

    return CreateResource<Texture>(targetCache, texture);
}

inline stringImpl getRendererTypeName(RendererType type) {
    switch (type) {
        default:
        case RendererType::COUNT:
            return "Unknown_Renderer_Type";
        case RendererType::RENDERER_TILED_FORWARD_SHADING:
            return "Tiled_Forward_Shading_Renderer";
        case RendererType::RENDERER_DEFERRED_SHADING:
            return "Deferred_Shading Renderer";
    }
}
}

void populatePressRelease(PressReleaseActions& actions, const ptree & attributes) {
    actions._onPressAction = attributes.get<U16>("actionDown", 0u);
    actions._onReleaseAction = attributes.get<U16>("actionUp", 0u);
    actions._onLCtrlPressAction = attributes.get<U16>("actionLCtrlDown", 0u);
    actions._onLCtrlReleaseAction = attributes.get<U16>("actionLCtrlUp", 0u);
    actions._onRCtrlPressAction = attributes.get<U16>("actionRCtrlDown", 0u);
    actions._onRCtrlReleaseAction = attributes.get<U16>("actionRCtrlUp", 0u);
    actions._onLAltPressAction = attributes.get<U16>("actionLAltDown", 0u);
    actions._onLAltReleaseAction = attributes.get<U16>("actionLAtlUp", 0u);
    actions._onRAltPressAction = attributes.get<U16>("actionRAltDown", 0u);
    actions._onRAltReleaseAction = attributes.get<U16>("actionRAltUp", 0u);
}

void loadDefaultKeybindings(const stringImpl &file, Scene* scene) {
    ptree pt;
    Console::printfn(Locale::get(_ID("XML_LOAD_DEFAULT_KEY_BINDINGS")), file.c_str());
    read_xml(file.c_str(), pt);

    for(const ptree::value_type & f : pt.get_child("actions", empty_ptree()))
    {
        const ptree & attributes = f.second.get_child("<xmlattr>", empty_ptree());
        scene->input().actionList()
                      .getInputAction(attributes.get<U16>("id", 0))
                      .displayName(attributes.get<std::string>("name", "").c_str());
    }

    PressReleaseActions actions;
    for (const ptree::value_type & f : pt.get_child("keys", empty_ptree()))
    {
        if (f.first.compare("<xmlcomment>") == 0) {
            continue;
        }

        const ptree & attributes = f.second.get_child("<xmlattr>", empty_ptree());
        populatePressRelease(actions, attributes);

        Input::KeyCode key = Input::InputInterface::keyCodeByName(f.second.data().c_str());
        scene->input().addKeyMapping(key, actions);
    }

    for (const ptree::value_type & f : pt.get_child("mouseButtons", empty_ptree()))
    {
        if (f.first.compare("<xmlcomment>") == 0) {
            continue;
        }

        const ptree & attributes = f.second.get_child("<xmlattr>", empty_ptree());
        populatePressRelease(actions, attributes);

        Input::MouseButton btn = Input::InputInterface::mouseButtonByName(f.second.data().c_str());

        scene->input().addMouseMapping(btn, actions);
    }

    const std::string label("joystickButtons.joystick");
    for (U32 i = 0 ; i < to_base(Input::Joystick::COUNT); ++i) {
        Input::Joystick joystick = static_cast<Input::Joystick>(i);
        
        for (const ptree::value_type & f : pt.get_child(label + std::to_string(i + 1), empty_ptree()))
        {
            if (f.first.compare("<xmlcomment>") == 0) {
                continue;
            }

            const ptree & attributes = f.second.get_child("<xmlattr>", empty_ptree());
            populatePressRelease(actions, attributes);

            Input::JoystickElement element = Input::InputInterface::joystickElementByName(f.second.data().c_str());

            scene->input().addJoystickMapping(joystick, element, actions);
        }
    }
}

void loadScene(const stringImpl& scenePath, const stringImpl &sceneName, Scene* scene, const Configuration& config) {
    ParamHandler &par = ParamHandler::instance();
    
    ptree pt;
    Console::printfn(Locale::get(_ID("XML_LOAD_SCENE")), sceneName.c_str());
    std::string sceneLocation(scenePath + "/" + sceneName.c_str());
    std::string sceneDataFile(sceneLocation + ".xml");

    // A scene does not necessarily need external data files
    // Data can be added in code for simple scenes
    if (!fileExists(sceneDataFile.c_str())) {
        loadTerrain((sceneLocation + "/" + "terrain.xml").c_str(), scene);
        loadGeometry((sceneLocation + "/" + "assets.xml").c_str(), scene);
        loadMusicPlaylist((sceneLocation + "/" + "musicPlaylist.xml").c_str(), scene, config);
        return;
    }

    try {
        read_xml(sceneDataFile.c_str(), pt);
    } catch (boost::property_tree::xml_parser_error &e) {
        Console::errorfn(Locale::get(_ID("ERROR_XML_INVALID_SCENE")), sceneName.c_str());
        stringImpl error(e.what());
        error += " [check error log!]";
        throw error.c_str();
    }

    scene->state().renderState().grassVisibility(pt.get("vegetation.grassVisibility", 1000.0f));
    scene->state().renderState().treeVisibility(pt.get("vegetation.treeVisibility", 1000.0f));
    scene->state().renderState().generalVisibility(pt.get("options.visibility", 1000.0f));

    scene->state().windDirX(pt.get("wind.windDirX", 1.0f));
    scene->state().windDirZ(pt.get("wind.windDirZ", 1.0f));
    scene->state().windSpeed(pt.get("wind.windSpeed", 1.0f));

    scene->state().waterLevel(pt.get("water.waterLevel", 0.0f));
    scene->state().waterDepth(pt.get("water.waterDepth", -75.0f));

    if (boost::optional<ptree &> cameraPositionOverride =  pt.get_child_optional("options.cameraStartPosition")) {
        par.setParam(_ID_RT((sceneName + ".options.cameraStartPosition.x").c_str()), pt.get("options.cameraStartPosition.<xmlattr>.x", 0.0f));
        par.setParam(_ID_RT((sceneName + ".options.cameraStartPosition.y").c_str()), pt.get("options.cameraStartPosition.<xmlattr>.y", 0.0f));
        par.setParam(_ID_RT((sceneName + ".options.cameraStartPosition.z").c_str()), pt.get("options.cameraStartPosition.<xmlattr>.z", 0.0f));
        par.setParam(_ID_RT((sceneName + ".options.cameraStartOrientation.xOffsetDegrees").c_str()),
                     pt.get("options.cameraStartPosition.<xmlattr>.xOffsetDegrees", 0.0f));
        par.setParam(_ID_RT((sceneName + ".options.cameraStartOrientation.yOffsetDegrees").c_str()),
                     pt.get("options.cameraStartPosition.<xmlattr>.yOffsetDegrees", 0.0f));
        par.setParam(_ID_RT((sceneName + ".options.cameraStartPositionOverride").c_str()), true);
    } else {
        par.setParam(_ID_RT((sceneName + ".options.cameraStartPositionOverride").c_str()), false);
    }

    if (boost::optional<ptree &> physicsCook = pt.get_child_optional("options.autoCookPhysicsAssets")) {
        par.setParam(_ID_RT((sceneName + ".options.autoCookPhysicsAssets").c_str()), pt.get<bool>("options.autoCookPhysicsAssets", false));
    } else {
        par.setParam(_ID_RT((sceneName + ".options.autoCookPhysicsAssets").c_str()), false);
    }

    if (boost::optional<ptree &> cameraPositionOverride = pt.get_child_optional("options.cameraSpeed")) {
        par.setParam(_ID_RT((sceneName + ".options.cameraSpeed.move").c_str()), pt.get("options.cameraSpeed.<xmlattr>.move", 35.0f));
        par.setParam(_ID_RT((sceneName + ".options.cameraSpeed.turn").c_str()), pt.get("options.cameraSpeed.<xmlattr>.turn", 35.0f));
    } else {
        par.setParam(_ID_RT((sceneName + ".options.cameraSpeed.move").c_str()), 35.0f);
        par.setParam(_ID_RT((sceneName + ".options.cameraSpeed.turn").c_str()), 35.0f);
    }

    vec3<F32> fogColour(config.rendering.fogColour);
    F32 fogDensity = config.rendering.fogDensity;

    if (boost::optional<ptree &> fog = pt.get_child_optional("fog")) {
        fogDensity = pt.get("fog.fogDensity", 0.01f);
        fogColour.set(pt.get<F32>("fog.fogColour.<xmlattr>.r", 0.2f),
                      pt.get<F32>("fog.fogColour.<xmlattr>.g", 0.2f),
                      pt.get<F32>("fog.fogColour.<xmlattr>.b", 0.2f));
    }
    scene->state().fogDescriptor().set(fogColour, fogDensity);

    loadTerrain((sceneLocation + "/" + pt.get("terrain", "")).c_str(), scene);
    loadGeometry((sceneLocation + "/" + pt.get("assets", "")).c_str(), scene);
    loadMusicPlaylist((sceneLocation + "/" + pt.get("musicPlaylist", "")).c_str(), scene, config);
}

void loadMusicPlaylist(const stringImpl& file, Scene* const scene, const Configuration& config) {
    if (!fileExists(file.c_str())) {
        return;
    }
    Console::printfn(Locale::get(_ID("XML_LOAD_MUSIC")), file.c_str());
    ptree pt;
    read_xml(file.c_str(), pt);

    for (const ptree::value_type & f : pt.get_child("backgroundThemes", empty_ptree()))
    {
        const ptree & attributes = f.second.get_child("<xmlattr>", empty_ptree());
        scene->addMusic(MusicType::TYPE_BACKGROUND,
                        attributes.get<std::string>("name", "").c_str(),
                         Paths::g_assetsLocation + attributes.get<std::string>("src", "").c_str());
    }
}

void loadTerrain(const stringImpl &file, Scene *const scene) {
    if (!fileExists(file.c_str())) {
        return;
    }

    U8 count = 0;
    ptree pt;
    Console::printfn(Locale::get(_ID("XML_LOAD_TERRAIN")), file.c_str());
    read_xml(file.c_str(), pt);
    ptree::iterator itTerrain;
    ptree::iterator itTexture;

    for (itTerrain = std::begin(pt.get_child("terrainList")); itTerrain != std::end(pt.get_child("terrainList")); ++itTerrain) {
        // The actual terrain name
        std::string name = itTerrain->second.data();
        // The <name> tag for valid terrains or <xmlcomment> for comments
        std::string tag = itTerrain->first.data();
        // Check and skip commented terrain
        if (tag.find("<xmlcomment>") != stringImpl::npos) {
            continue;
        }
        // Load the rest of the terrain
        std::shared_ptr<TerrainDescriptor> ter = std::make_shared<TerrainDescriptor>((name + "_descriptor").c_str());

        ter->addVariable("terrainName", name.c_str());
        ter->addVariable("heightmapLocation", Paths::g_assetsLocation + pt.get<stringImpl>(name + ".heightmapLocation", Paths::g_heightmapLocation) + "/");
        ter->addVariable("textureLocation", Paths::g_assetsLocation + pt.get<stringImpl>(name + ".textureLocation", Paths::g_imagesLocation) + "/");
        ter->addVariable("heightmap", pt.get<stringImpl>(name + ".heightmap"));
        ter->addVariable("waterCaustics", pt.get<stringImpl>(name + ".waterCaustics"));
        ter->addVariable("underwaterAlbedoTexture", pt.get<stringImpl>(name + ".underwaterAlbedoTexture"));
        ter->addVariable("underwaterDetailTexture", pt.get<stringImpl>(name + ".underwaterDetailTexture"));
        ter->addVariable("underwaterDiffuseScale", pt.get<F32>(name + ".underwaterDiffuseScale"));

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
            ter->addVariable("blendMap" + layerOffsetStr, temp);

            temp = pt.get<stringImpl>(layerName + ".redAlbedo", "");
            if (!temp.empty()) {
                ter->addVariable("redAlbedo" + layerOffsetStr, temp);
            }
            temp = pt.get<stringImpl>(layerName + ".redDetail", "");
            if (!temp.empty()) {
                ter->addVariable("redDetail" + layerOffsetStr, temp);
            }
            temp = pt.get<stringImpl>(layerName + ".greenAlbedo", "");
            if (!temp.empty()) {
                ter->addVariable("greenAlbedo" + layerOffsetStr, temp);
            }
            temp = pt.get<stringImpl>(layerName + ".greenDetail", "");
            if (!temp.empty()) {
                ter->addVariable("greenDetail" + layerOffsetStr, temp);
            }
            temp = pt.get<stringImpl>(layerName + ".blueAlbedo", "");
            if (!temp.empty()) {
                ter->addVariable("blueAlbedo" + layerOffsetStr, temp);
            }
            temp = pt.get<stringImpl>(layerName + ".blueDetail", "");
            if (!temp.empty()) {
                ter->addVariable("blueDetail" + layerOffsetStr, temp);
            }
            temp = pt.get<stringImpl>(layerName + ".alphaAlbedo", "");
            if (!temp.empty()) {
                ter->addVariable("alphaAlbedo" + layerOffsetStr, temp);
            }
            temp = pt.get<stringImpl>(layerName + ".alphaDetail", "");
            if (!temp.empty()) {
                ter->addVariable("alphaDetail" + layerOffsetStr, temp);
            }

            ter->addVariable("diffuseScaleR" + layerOffsetStr, pt.get<F32>(layerName + ".redDiffuseScale", 0.0f));
            ter->addVariable("detailScaleR" + layerOffsetStr, pt.get<F32>(layerName + ".redDetailScale", 0.0f));
            ter->addVariable("diffuseScaleG" + layerOffsetStr, pt.get<F32>(layerName + ".greenDiffuseScale", 0.0f));
            ter->addVariable("detailScaleG" + layerOffsetStr, pt.get<F32>(layerName + ".greenDetailScale", 0.0f));
            ter->addVariable( "diffuseScaleB" + layerOffsetStr, pt.get<F32>(layerName + ".blueDiffuseScale", 0.0f));
            ter->addVariable("detailScaleB" + layerOffsetStr, pt.get<F32>(layerName + ".blueDetailScale", 0.0f));
            ter->addVariable("diffuseScaleA" + layerOffsetStr, pt.get<F32>(layerName + ".alphaDiffuseScale", 0.0f));
            ter->addVariable("detailScaleA" + layerOffsetStr, pt.get<F32>(layerName + ".alphaDetailScale", 0.0f));
        }

        ter->setTextureLayerCount(to_U8(i));
        ter->addVariable("grassMapLocation", Paths::g_assetsLocation + pt.get<stringImpl>(name + ".vegetation.vegetationTextureLocation", Paths::g_imagesLocation) + "/");
        ter->addVariable("grassMap", pt.get<stringImpl>(name + ".vegetation.map"));
        ter->addVariable("grassBillboard1", pt.get<stringImpl>(name + ".vegetation.grassBillboard1", ""));
        ter->addVariable("grassBillboard2", pt.get<stringImpl>(name + ".vegetation.grassBillboard2", ""));
        ter->addVariable("grassBillboard3", pt.get<stringImpl>(name + ".vegetation.grassBillboard3", ""));
        ter->addVariable("grassBillboard4", pt.get<stringImpl>(name + ".vegetation.grassBillboard4", ""));
        ter->setGrassDensity(pt.get<F32>(name + ".vegetation.<xmlattr>.grassDensity"));
        ter->setTreeDensity(pt.get<F32>(name + ".vegetation.<xmlattr>.treeDensity"));
        ter->setGrassScale(pt.get<F32>(name + ".vegetation.<xmlattr>.grassScale"));
        ter->setTreeScale(pt.get<F32>(name + ".vegetation.<xmlattr>.treeScale"));
        ter->set16Bit(pt.get<bool>(name + ".is16Bit", false));
        ter->setPosition(vec3<F32>(pt.get<F32>(name + ".position.<xmlattr>.x", 0.0f),
                                   pt.get<F32>(name + ".position.<xmlattr>.y", 0.0f),
                                   pt.get<F32>(name + ".position.<xmlattr>.z", 0.0f)));
        ter->setScale(vec2<F32>(pt.get<F32>(name + ".scale", 1.0f),
                                pt.get<F32>(name + ".heightFactor", 1.0f)));
        ter->setDimensions(vec2<U16>(pt.get<U16>(name + ".terrainWidth", 0),
                                     pt.get<U16>(name + ".terrainHeight", 0)));
        ter->setAltitudeRange(vec2<F32>(pt.get<F32>(name + ".altitudeRange.<xmlattr>.min", 0.0f),
                                        pt.get<F32>(name + ".altitudeRange.<xmlattr>.max", 255.0f)));
        ter->setActive(pt.get<bool>(name + ".active", true));
        ter->setChunkSize(pt.get<U32>(name + ".targetChunkSize", 256));

        scene->addTerrain(ter);
        count++;
    }

    Console::printfn(Locale::get(_ID("XML_TERRAIN_COUNT")), count);
}

void loadGeometry(const stringImpl &file, Scene *const scene) {
    if (!fileExists(file.c_str())) {
        return;
    }
    Console::printfn(Locale::get(_ID("XML_LOAD_GEOMETRY")), file.c_str());

    ptree pt;
    read_xml(file.c_str(), pt);
    ptree::iterator it;

    if (boost::optional<ptree &> geometry = pt.get_child_optional("geometry")) {
        for (it = std::begin(pt.get_child("geometry")); it != std::end(pt.get_child("geometry")); ++it) {
            std::string name(it->second.data());
            std::string format(it->first.data());
            if (format.find("<xmlcomment>") != stringImpl::npos) {
                continue;
            }
            FileData model;
            model.ItemName = name.c_str();
            model.ModelName = Paths::g_assetsLocation + pt.get<stringImpl>(name + ".model");
            model.position.x = pt.get<F32>(name + ".position.<xmlattr>.x", 1);
            model.position.y = pt.get<F32>(name + ".position.<xmlattr>.y", 1);
            model.position.z = pt.get<F32>(name + ".position.<xmlattr>.z", 1);
            model.orientation.x = pt.get<F32>(name + ".orientation.<xmlattr>.x");
            model.orientation.y = pt.get<F32>(name + ".orientation.<xmlattr>.y");
            model.orientation.z = pt.get<F32>(name + ".orientation.<xmlattr>.z");
            model.scale.x = pt.get<F32>(name + ".scale.<xmlattr>.x");
            model.scale.y = pt.get<F32>(name + ".scale.<xmlattr>.y");
            model.scale.z = pt.get<F32>(name + ".scale.<xmlattr>.z");
            model.type = GeometryType::GEOMETRY;
            model.version = pt.get<F32>(name + ".version");

            if (boost::optional<ptree &> child = pt.get_child_optional(name + ".castsShadows")) {
                model.castsShadows = pt.get<bool>(name + ".castsShadows", false);
            } else {
                model.castsShadows = true;
            }

            if (boost::optional<ptree &> child = pt.get_child_optional(name + ".receivesShadows")) {
                model.receivesShadows = pt.get<bool>(name + ".receivesShadows", false);
            } else {
                model.receivesShadows = true;
            }

            if (boost::optional<ptree &> child = pt.get_child_optional(name + ".staticObject")) {
                model.staticUsage = pt.get<bool>(name + ".staticObject", false);
            } else {
                model.staticUsage = false;
            }

            if (boost::optional<ptree &> child = pt.get_child_optional(name + ".isUnit")) {
                model.isUnit = pt.get<bool>(name + ".isUnit", false);
            } else {
                model.isUnit = false;
            }
            
            if (boost::optional<ptree &> child = pt.get_child_optional(name + ".addToNavigation")) {
                model.navigationUsage = pt.get<bool>(name + ".addToNavigation", false);
            } else {
                model.navigationUsage = false;
            }

            if (boost::optional<ptree &> child = pt.get_child_optional(name + ".useHighNavigationDetail")) {
                model.useHighDetailNavMesh = pt.get<bool>(name + ".useHighNavigationDetail", false);
            } else {
                model.useHighDetailNavMesh = false;
            }

            if (boost::optional<ptree &> child = pt.get_child_optional(name + ".addToPhysics")) {
                model.physicsUsage = pt.get<bool>(name + ".addToPhysics", false);
                model.physicsStatic = !pt.get<bool>(name + ".addToPhysicsGroupPushable", false);
            } else {
                model.physicsUsage = false;
            }

            scene->addModel(model);
        }
    }

    if (boost::optional<ptree &> vegetation = pt.get_child_optional("vegetation")) {
        for (it = std::begin(pt.get_child("vegetation")); it != std::end(pt.get_child("vegetation")); ++it) {
            std::string name = it->second.data();
            std::string format = it->first.data();
            if (format.find("<xmlcomment>") != stringImpl::npos) {
                continue;
            }
            FileData model;
            model.ItemName = name.c_str();
            model.ModelName = Paths::g_assetsLocation + pt.get<stringImpl>(name + ".model");
            model.position.x = pt.get<F32>(name + ".position.<xmlattr>.x");
            model.position.y = pt.get<F32>(name + ".position.<xmlattr>.y");
            model.position.z = pt.get<F32>(name + ".position.<xmlattr>.z");
            model.orientation.x = pt.get<F32>(name + ".orientation.<xmlattr>.x");
            model.orientation.y = pt.get<F32>(name + ".orientation.<xmlattr>.y");
            model.orientation.z = pt.get<F32>(name + ".orientation.<xmlattr>.z");
            model.scale.x = pt.get<F32>(name + ".scale.<xmlattr>.x");
            model.scale.y = pt.get<F32>(name + ".scale.<xmlattr>.y");
            model.scale.z = pt.get<F32>(name + ".scale.<xmlattr>.z");
            model.type = GeometryType::VEGETATION;
            model.version = pt.get<F32>(name + ".version");

            if (boost::optional<ptree &> child = pt.get_child_optional(name + ".castsShadows")) {
                model.castsShadows = pt.get<bool>(name + ".castsShadows", false);
            } else {
                model.castsShadows = true;
            }

            if (boost::optional<ptree &> child = pt.get_child_optional(name + ".receivesShadows")) {
                model.receivesShadows = pt.get<bool>(name + ".receivesShadows", false);
            } else {
                model.receivesShadows = true;
            }
            if (boost::optional<ptree &> child = pt.get_child_optional(name + ".staticObject")) {
                model.staticUsage = pt.get<bool>(name + ".staticObject", false);
            } else {
                model.staticUsage = false;
            }
            if (boost::optional<ptree &> child = pt.get_child_optional(name + ".addToNavigation")) {
                model.navigationUsage =
                    pt.get<bool>(name + ".addToNavigation", false);
            } else {
                model.navigationUsage = false;
            }
            if (boost::optional<ptree &> child = pt.get_child_optional(name + ".useHighNavigationDetail")) {
                model.useHighDetailNavMesh =
                    pt.get<bool>(name + ".useHighNavigationDetail", false);
            } else {
                model.useHighDetailNavMesh = false;
            }
            if (boost::optional<ptree &> child = pt.get_child_optional(name + ".addToPhysics")) {
                model.physicsUsage = pt.get<bool>(name + ".addToPhysics", false);
            } else {
                model.physicsUsage = false;
            }
            scene->addModel(model);
        }
    }

    if (boost::optional<ptree &> primitives = pt.get_child_optional("primitives")) {
        for (it = std::begin(pt.get_child("primitives")); it != std::end(pt.get_child("primitives")); ++it) {
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
            model.orientation.x = pt.get<F32>(name + ".orientation.<xmlattr>.x");
            model.orientation.y = pt.get<F32>(name + ".orientation.<xmlattr>.y");
            model.orientation.z = pt.get<F32>(name + ".orientation.<xmlattr>.z");
            model.scale.x = pt.get<F32>(name + ".scale.<xmlattr>.x");
            model.scale.y = pt.get<F32>(name + ".scale.<xmlattr>.y");
            model.scale.z = pt.get<F32>(name + ".scale.<xmlattr>.z");
            model.colour.r = pt.get<F32>(name + ".colour.<xmlattr>.r");
            model.colour.g = pt.get<F32>(name + ".colour.<xmlattr>.g");
            model.colour.b = pt.get<F32>(name + ".colour.<xmlattr>.b");
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

            if (boost::optional<ptree &> child = pt.get_child_optional(name + ".castsShadows")) {
                model.castsShadows = pt.get<bool>(name + ".castsShadows", false);
            } else {
                model.castsShadows = true;
            }

            if (boost::optional<ptree &> child = pt.get_child_optional(name + ".receivesShadows")) {
                model.receivesShadows = pt.get<bool>(name + ".receivesShadows", false);
            } else {
                model.receivesShadows = true;
            }

            if (boost::optional<ptree &> child = pt.get_child_optional(name + ".staticObject")) {
                model.staticUsage = pt.get<bool>(name + ".staticObject", false);
            } else {
                model.staticUsage = false;
            }

            if (boost::optional<ptree &> child = pt.get_child_optional(name + ".addToNavigation")) {
                model.navigationUsage = pt.get<bool>(name + ".addToNavigation", false);
            } else {
                model.navigationUsage = false;
            }

            if (boost::optional<ptree &> child = pt.get_child_optional(name + ".useHighNavigationDetail")) {
                model.useHighDetailNavMesh = pt.get<bool>(name + ".useHighNavigationDetail", false);
            } else {
                model.useHighDetailNavMesh = false;
            }
            scene->addModel(model);
        }
    }
}

Material_ptr loadMaterial(PlatformContext& context, const stringImpl &file) {
    stringImpl location = Paths::g_xmlDataLocation + Paths::g_scenesLocation +
                          ParamHandler::instance().getParam<stringImpl>(_ID("currentScene")) +
                          "/" + Paths::g_materialsLocation;

    return loadMaterialXML(context, location + file);
}

Material_ptr loadMaterialXML(PlatformContext& context, const stringImpl &matName, bool rendererDependent) {
    ResourceCache& cache = context.gfx().parent().resourceCache();

    stringImpl materialFile(matName);
    if (rendererDependent) {
        materialFile +=
            "-" + getRendererTypeName(context.gfx().getRenderer().getType()) +
            ".xml";
    } else {
        materialFile += ".xml";
    }

    ptree pt;
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

    ResourceDescriptor desc(materialName);

    if (!FindResourceImpl<Material>(cache, desc.getHash())) {
        skip = true;
    }

    Material_ptr mat = CreateResource<Material>(cache, desc);
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
                        loadTextureXML(cache,
                                       "diffuseTexture1",
                                       pt.get("diffuseTexture1.file", "none").c_str(), pt));
    }

    if (boost::optional<ptree &> child =
            pt.get_child_optional("diffuseTexture2")) {
        mat->setTexture(ShaderProgram::TextureUsage::UNIT1,
                        loadTextureXML(cache,
                                       "diffuseTexture2",
                                       pt.get("diffuseTexture2.file", "none").c_str(), pt),
                        getTextureOperation(
                            pt.get<stringImpl>("diffuseTexture2.operation",
                                               "TEX_OP_MULTIPLY")));
    }

    if (boost::optional<ptree &> child = pt.get_child_optional("bumpMap")) {
        mat->setTexture(
            ShaderProgram::TextureUsage::NORMALMAP,
            loadTextureXML(cache, "bumpMap", pt.get("bumpMap.file", "none").c_str(), pt));
        if (child = pt.get_child_optional("bumpMap.method")) {
            mat->setBumpMethod(getBumpMethod(
                pt.get<stringImpl>("bumpMap.method", "NORMAL")));
        }
    }

    if (boost::optional<ptree &> child = pt.get_child_optional("opacityMap")) {
        mat->setTexture(
            ShaderProgram::TextureUsage::OPACITY,
            loadTextureXML(cache, "opacityMap", pt.get("opacityMap.file", "none").c_str(), pt));
    }

    if (boost::optional<ptree &> child = pt.get_child_optional("specularMap")) {
        mat->setTexture(
            ShaderProgram::TextureUsage::SPECULAR,
            loadTextureXML(cache, "specularMap", pt.get("specularMap.file", "none").c_str(), pt));
    }

    for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
        for (U32 i = 0; i < to_base(RenderStage::COUNT); ++i) {
            RenderStage stage = static_cast<RenderStage>(i);
            RenderPassType passType = static_cast<RenderPassType>(pass);

            stringImpl shader = Util::StringFormat("shaderProgram.%s.%s", TypeUtil::renderStageToString(stage), TypeUtil::renderPassTypeToString(passType));

            if (boost::optional<ptree &> child = pt.get_child_optional(shader.c_str())) {
                mat->setShaderProgram(pt.get(shader.c_str(), "NULL").c_str(), RenderStagePass(stage, passType), false);
            }
        }
    }

    return mat;
}

//ToDo: Fix this one day .... -Ionut
void dumpMaterial(PlatformContext& context, Material &mat) {
    if (!mat.isDirty()) {
        return;
    }

    ParamHandler &par = ParamHandler::instance();
    stringImpl file(mat.getName());
    file = file.substr(file.rfind("/") + 1, file.length());

    stringImpl location(Paths::g_xmlDataLocation + Paths::g_scenesLocation  +
                        par.getParam<stringImpl>(_ID("activeScene")) +
                        "/" + Paths::g_materialsLocation);

    stringImpl fileLocation(
        location + file + "-" +
        getRendererTypeName(context.gfx().getRenderer().getType()) + ".xml");

    PREPARE_FILE_FOR_WRITING(fileLocation.c_str());

    pt.put("material.name", file);
    pt.put("material.diffuse.<xmlattr>.r",
                  mat.getColourData()._diffuse.r);
    pt.put("material.diffuse.<xmlattr>.g",
                  mat.getColourData()._diffuse.g);
    pt.put("material.diffuse.<xmlattr>.b",
                  mat.getColourData()._diffuse.b);
    pt.put("material.diffuse.<xmlattr>.a",
                  mat.getColourData()._diffuse.a);
    pt.put("material.specular.<xmlattr>.r",
                  mat.getColourData()._specular.r);
    pt.put("material.specular.<xmlattr>.g",
                  mat.getColourData()._specular.g);
    pt.put("material.specular.<xmlattr>.b",
                  mat.getColourData()._specular.b);
    pt.put("material.specular.<xmlattr>.a",
                  mat.getColourData()._specular.a);
    pt.put("material.shininess.<xmlattr>.v",
                  mat.getColourData()._shininess);
    pt.put("material.emissive.<xmlattr>.r",
                  mat.getColourData()._emissive.y);
    pt.put("material.emissive.<xmlattr>.g",
                  mat.getColourData()._emissive.z);
    pt.put("material.emissive.<xmlattr>.b",
                  mat.getColourData()._emissive.w);
    pt.put("material.doubleSided", mat.isDoubleSided());

    std::weak_ptr<Texture> texture;
    if (!(texture = mat.getTexture(ShaderProgram::TextureUsage::UNIT0)).expired()) {
        saveTextureXML("diffuseTexture1", texture, pt);
    }

    if (!(texture = mat.getTexture(ShaderProgram::TextureUsage::UNIT1)).expired()) {
        saveTextureXML("diffuseTexture2", texture, pt,
                       getTextureOperationName(mat.getTextureOperation()));
    }

    if (!(texture = mat.getTexture(ShaderProgram::TextureUsage::NORMALMAP)).expired()) {
        saveTextureXML("bumpMap", texture, pt);
        pt.put("bumpMap.method", getBumpMethodName(mat.getBumpMethod()));
    }

    if (!(texture = mat.getTexture(ShaderProgram::TextureUsage::OPACITY)).expired()) {
        saveTextureXML("opacityMap", texture, pt);
    }

    if (!(texture = mat.getTexture(ShaderProgram::TextureUsage::SPECULAR)).expired()) {
        saveTextureXML("specularMap", texture, pt);
    }

    for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
        for (U32 i = 0; i < to_base(RenderStage::COUNT); ++i) {
            RenderStage stage = static_cast<RenderStage>(i);
            RenderPassType passType = static_cast<RenderPassType>(pass);

            ShaderProgram_ptr shaderProg = mat.getShaderInfo(RenderStagePass(stage, passType)).getProgram();
            pt.put(Util::StringFormat("shaderProgram.%s.%s", TypeUtil::renderStageToString(stage), TypeUtil::renderPassTypeToString(passType)).c_str(), shaderProg->getName());
        }
    }

    SAVE_FILE(fileLocation.c_str());
}
};  // namespace XML
};  // namespace Divide
