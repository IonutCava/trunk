#include "stdafx.h"

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
#include "Platform/File/Headers/FileManagement.h"
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

ComponentType getComponentType(const stringImpl& name) {
    if (Util::CompareIgnoreCase(name, "Animation")) {
        return ComponentType::ANIMATION;
    } else if (Util::CompareIgnoreCase(name, "IK")) {
        return ComponentType::INVERSE_KINEMATICS;
    } else if (Util::CompareIgnoreCase(name, "Ragdoll")) {
        return ComponentType::RAGDOLL;
    } else if (Util::CompareIgnoreCase(name, "Navigation")) {
        return ComponentType::NAVIGATION;
    } else if (Util::CompareIgnoreCase(name, "Unit")) {
        return ComponentType::UNIT;
    } else if (Util::CompareIgnoreCase(name, "Rigidbody")) {
        return ComponentType::RIGID_BODY;
    } else if (Util::CompareIgnoreCase(name, "Selection")) {
        return ComponentType::SELECTION;
    }

    return ComponentType::COUNT;
}

}

void populatePressRelease(PressReleaseActions& actions, const ptree & attributes) {
    actions.actionID(PressReleaseActions::Action::PRESS, attributes.get<U16>("actionDown", 0u));
    actions.actionID(PressReleaseActions::Action::RELEASE, attributes.get<U16>("actionUp", 0u));
    actions.actionID(PressReleaseActions::Action::LEFT_CTRL_PRESS, attributes.get<U16>("actionLCtrlDown", 0u));
    actions.actionID(PressReleaseActions::Action::LEFT_CTRL_RELEASE, attributes.get<U16>("actionLCtrlUp", 0u));
    actions.actionID(PressReleaseActions::Action::RIGHT_CTRL_PRESS, attributes.get<U16>("actionRCtrlDown", 0u));
    actions.actionID(PressReleaseActions::Action::RIGHT_CTRL_RELEASE, attributes.get<U16>("actionRCtrlUp", 0u));
    actions.actionID(PressReleaseActions::Action::LEFT_ALT_PRESS, attributes.get<U16>("actionLAltDown", 0u));
    actions.actionID(PressReleaseActions::Action::LEFT_ALT_RELEASE, attributes.get<U16>("actionLAtlUp", 0u));
    actions.actionID(PressReleaseActions::Action::RIGHT_ALT_PRESS, attributes.get<U16>("actionRAltDown", 0u));
    actions.actionID(PressReleaseActions::Action::RIGHT_ALT_RELEASE, attributes.get<U16>("actionRAltUp", 0u));
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

    vector<stringImpl> nodes;

    ptree pt;
    read_xml(file.c_str(), pt);

    for (const ptree::value_type & entityLists : pt.get_child("entities", empty_ptree())){
        for (const ptree::value_type & sceneGraphLists : entityLists.second.get_child("node", empty_ptree())) {
            for (const ptree::value_type & nodeList : sceneGraphLists.second.get_child("node", empty_ptree())) {
                std::string a = nodeList.first;
                ptree b = nodeList.second;
            }
        }
    }

    for (const stringImpl& name : nodes) {
        FileData model = {};
        model.ItemName = name.c_str();
        model.ModelName = pt.get<stringImpl>(name + ".model");
        model.position.x = pt.get<F32>(name + ".position.<xmlattr>.x", 1);
        model.position.y = pt.get<F32>(name + ".position.<xmlattr>.y", 1);
        model.position.z = pt.get<F32>(name + ".position.<xmlattr>.z", 1);
        model.orientation.x = pt.get<F32>(name + ".orientation.<xmlattr>.x", 0.0f);
        model.orientation.y = pt.get<F32>(name + ".orientation.<xmlattr>.y", 0.0f);
        model.orientation.z = pt.get<F32>(name + ".orientation.<xmlattr>.z", 0.0f);
        model.scale.x = pt.get<F32>(name + ".scale.<xmlattr>.x");
        model.scale.y = pt.get<F32>(name + ".scale.<xmlattr>.y");
        model.scale.z = pt.get<F32>(name + ".scale.<xmlattr>.z");
        if (boost::optional<ptree &> child = pt.get_child_optional(name + ".colour")) {
            model.colour.r = pt.get<F32>(name + ".colour.<xmlattr>.r", 0.8f);
            model.colour.g = pt.get<F32>(name + ".colour.<xmlattr>.g", 0.3f);
            model.colour.b = pt.get<F32>(name + ".colour.<xmlattr>.b", 1.0f);
        }

        if (boost::optional<ptree &> child = pt.get_child_optional(name + ".static")) {
            model.staticUsage = pt.get<bool>(name + ".static", false);
        } else {
            model.staticUsage = false;
        }

        if (Util::CompareIgnoreCase(model.ModelName, "Box3D")) {
            model.data = pt.get<F32>(name + ".size");
        } else if (Util::CompareIgnoreCase(model.ModelName, "Sphere3D")) {
            model.data = pt.get<F32>(name + ".radius");
        }

        if (boost::optional<ptree &> child = pt.get_child_optional(name + ".components")) {
            stringImpl componentList = pt.get<stringImpl>(name + ".components", "");
            vector<stringImpl> components;
            Util::Split<vector<stringImpl>, stringImpl>(componentList.c_str(), ',', components);
            for (const stringImpl& comp : components) {
                ComponentType compType = getComponentType(comp);
                if (compType != ComponentType::COUNT) {
                    model.componentMask |= to_base(compType);
                }
            }
        }

        scene->addModel(model);
    }
}

};  // namespace XML
};  // namespace Divide

