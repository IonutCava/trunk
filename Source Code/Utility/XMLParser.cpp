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
}

void populatePressRelease(PressReleaseActions& actions, const ptree & attributes) {
    constexpr std::pair<PressReleaseActions::Action, const char*> actionNames[10] = {
        {PressReleaseActions::Action::PRESS, "actionDown"},
        {PressReleaseActions::Action::RELEASE, "actionUp"},
        {PressReleaseActions::Action::LEFT_CTRL_PRESS, "actionLCtrlDown"},
        {PressReleaseActions::Action::LEFT_CTRL_RELEASE, "actionLCtrlUp"},
        {PressReleaseActions::Action::RIGHT_CTRL_PRESS, "actionRCtrlDown"},
        {PressReleaseActions::Action::RIGHT_CTRL_RELEASE, "actionRCtrlUp"},
        {PressReleaseActions::Action::LEFT_ALT_PRESS, "actionLAltDown"},
        {PressReleaseActions::Action::LEFT_ALT_RELEASE, "actionLAtlUp"},
        {PressReleaseActions::Action::RIGHT_ALT_PRESS, "actionRAltDown"},
        {PressReleaseActions::Action::RIGHT_ALT_RELEASE, "actionRAltUp"}
    };

    actions.clear();

    U16 id = 0;
    vector<std::string> actionsOut;
    for (const auto it : actionNames) {
        const std::string actionList = attributes.get<std::string>(it.second, "");
        Util::Split<vector<std::string>, std::string>(actionList.c_str(), ',', actionsOut);
        for (const std::string& it2 : actionsOut) {
            if (!it2.empty()) {
                std::stringstream ss(Util::Trim(it2));
                ss >> id;
                if (!ss.fail()) {
                    actions.insertActionID(it.first, id);
                }
            }
        }
    }
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
                      .displayName(attributes.get<stringImpl>("name", "").c_str());
    }

    PressReleaseActions actions;
    for (const ptree::value_type & f : pt.get_child("keys", empty_ptree()))
    {
        if (f.first.compare("<xmlcomment>") == 0) {
            continue;
        }

        const ptree & attributes = f.second.get_child("<xmlattr>", empty_ptree());
        populatePressRelease(actions, attributes);

        Input::KeyCode key = Input::keyCodeByName(Util::Trim(f.second.data()).c_str());
        scene->input().addKeyMapping(key, actions);
    }

    for (const ptree::value_type & f : pt.get_child("mouseButtons", empty_ptree()))
    {
        if (f.first.compare("<xmlcomment>") == 0) {
            continue;
        }

        const ptree & attributes = f.second.get_child("<xmlattr>", empty_ptree());
        populatePressRelease(actions, attributes);

        Input::MouseButton btn = Input::mouseButtonByName(Util::Trim(f.second.data()).c_str());

        scene->input().addMouseMapping(btn, actions);
    }

    const stringImpl label("joystickButtons.joystick");
    for (U32 i = 0 ; i < to_base(Input::Joystick::COUNT); ++i) {
        const Input::Joystick joystick = static_cast<Input::Joystick>(i);
        
        for (const ptree::value_type & f : pt.get_child(label + std::to_string(i + 1), empty_ptree()))
        {
            if (f.first.compare("<xmlcomment>") == 0) {
                continue;
            }

            const ptree & attributes = f.second.get_child("<xmlattr>", empty_ptree());
            populatePressRelease(actions, attributes);

            Input::JoystickElement element = Input::joystickElementByName(Util::Trim(f.second.data()).c_str());

            scene->input().addJoystickMapping(joystick, element._type, element._elementIndex, actions);
        }
    }
}

void loadScene(const Str256& scenePath, const Str128& sceneName, Scene* scene, const Configuration& config) {
    ParamHandler &par = ParamHandler::instance();
    
    ptree pt;
    Console::printfn(Locale::get(_ID("XML_LOAD_SCENE")), sceneName.c_str());
    const Str256 sceneLocation(scenePath + "/" + sceneName.c_str());
    const Str256 sceneDataFile(sceneLocation + ".xml");

    // A scene does not necessarily need external data files
    // Data can be added in code for simple scenes
    if (!fileExists(sceneDataFile.c_str())) {
        loadSceneGraph(sceneLocation, "assets.xml", scene);
        loadMusicPlaylist(sceneLocation, "musicPlaylist.xml", scene, config);
        return;
    }

    try {
        read_xml(sceneDataFile.c_str(), pt);
    } catch (const boost::property_tree::xml_parser_error &e) {
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

    if (boost::optional<ptree &> waterOverride = pt.get_child_optional("water")) {
        WaterDetails waterDetails = {};
        waterDetails._heightOffset = pt.get("water.waterLevel", 0.0f);
        waterDetails._depth = pt.get("water.waterDepth", -75.0f);
        scene->state().globalWaterBodies().push_back(waterDetails);
    }

    if (boost::optional<ptree &> cameraPositionOverride =  pt.get_child_optional("options.cameraStartPosition")) {
        par.setParam(_ID_32((sceneName + ".options.cameraStartPosition.x").c_str()), pt.get("options.cameraStartPosition.<xmlattr>.x", 0.0f));
        par.setParam(_ID_32((sceneName + ".options.cameraStartPosition.y").c_str()), pt.get("options.cameraStartPosition.<xmlattr>.y", 0.0f));
        par.setParam(_ID_32((sceneName + ".options.cameraStartPosition.z").c_str()), pt.get("options.cameraStartPosition.<xmlattr>.z", 0.0f));
        par.setParam(_ID_32((sceneName + ".options.cameraStartOrientation.xOffsetDegrees").c_str()),
                     pt.get("options.cameraStartPosition.<xmlattr>.xOffsetDegrees", 0.0f));
        par.setParam(_ID_32((sceneName + ".options.cameraStartOrientation.yOffsetDegrees").c_str()),
                     pt.get("options.cameraStartPosition.<xmlattr>.yOffsetDegrees", 0.0f));
        par.setParam(_ID_32((sceneName + ".options.cameraStartPositionOverride").c_str()), true);
    } else {
        par.setParam(_ID_32((sceneName + ".options.cameraStartPositionOverride").c_str()), false);
    }

    if (boost::optional<ptree &> physicsCook = pt.get_child_optional("options.autoCookPhysicsAssets")) {
        par.setParam(_ID_32((sceneName + ".options.autoCookPhysicsAssets").c_str()), pt.get<bool>("options.autoCookPhysicsAssets", false));
    } else {
        par.setParam(_ID_32((sceneName + ".options.autoCookPhysicsAssets").c_str()), false);
    }

    if (boost::optional<ptree &> cameraPositionOverride = pt.get_child_optional("options.cameraSpeed")) {
        par.setParam(_ID_32((sceneName + ".options.cameraSpeed.move").c_str()), pt.get("options.cameraSpeed.<xmlattr>.move", 35.0f));
        par.setParam(_ID_32((sceneName + ".options.cameraSpeed.turn").c_str()), pt.get("options.cameraSpeed.<xmlattr>.turn", 35.0f));
    } else {
        par.setParam(_ID_32((sceneName + ".options.cameraSpeed.move").c_str()), 35.0f);
        par.setParam(_ID_32((sceneName + ".options.cameraSpeed.turn").c_str()), 35.0f);
    }

    vec3<F32> fogColour(config.rendering.fogColour);
    F32 fogDensity = config.rendering.fogDensity;

    if (boost::optional<ptree &> fog = pt.get_child_optional("fog")) {
        fogDensity = pt.get("fog.fogDensity", fogDensity);
        fogColour.set(pt.get<F32>("fog.fogColour.<xmlattr>.r", fogColour.r),
                      pt.get<F32>("fog.fogColour.<xmlattr>.g", fogColour.g),
                      pt.get<F32>("fog.fogColour.<xmlattr>.b", fogColour.b));
    }
    scene->state().renderState().fogDescriptor().set(fogColour, fogDensity);

    vec4<U16> lodThresholds(config.rendering.lodThresholds);

    if (boost::optional<ptree &> fog = pt.get_child_optional("lod")) {
        lodThresholds.set(pt.get<U16>("lod.lodThresholds.<xmlattr>.x", lodThresholds.x),
                          pt.get<U16>("lod.lodThresholds.<xmlattr>.y", lodThresholds.y),
                          pt.get<U16>("lod.lodThresholds.<xmlattr>.z", lodThresholds.z),
                          pt.get<U16>("lod.lodThresholds.<xmlattr>.w", lodThresholds.w));
    }
    scene->state().renderState().lodThresholds().set(lodThresholds);

    loadSceneGraph(sceneLocation, pt.get("assets", ""), scene);
    loadMusicPlaylist(sceneLocation, pt.get("musicPlaylist", ""), scene, config);
}

void loadMusicPlaylist(const Str256& scenePath, const Str64& fileName, Scene* const scene, const Configuration& config) {
    stringImpl file = scenePath + "/" + fileName.c_str();

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
                        attributes.get<stringImpl>("name", "").c_str(),
                        Paths::g_assetsLocation + attributes.get<stringImpl>("src", "").c_str());
    }
}

void loadSceneGraph(const Str256& scenePath, const Str64& fileName, Scene *const scene) {
    stringImpl file = scenePath + "/" + fileName.c_str();
    if (!fileExists(file.c_str())) {
        return;
    }

    Console::printfn(Locale::get(_ID("XML_LOAD_GEOMETRY")), file.c_str());

    std::function<void(const ptree::value_type& rootNode, SceneNode& graphOut)> readNode;

    readNode = [&readNode](const ptree::value_type& rootNode, SceneNode& graphOut) {
        const ptree& attributes = rootNode.second.get_child("<xmlattr>", empty_ptree());
        for (const ptree::value_type& attribute : attributes) {
            if (attribute.first == "name") {
                graphOut.name = attribute.second.data();
            } else if (attribute.first == "type") {
                graphOut.type = attribute.second.data();
            }
        }
        
        const ptree& children = rootNode.second.get_child("");
        for (const ptree::value_type& child : children) {
            if (child.first == "node") {
                graphOut.children.emplace_back();
                readNode(child, graphOut.children.back());
            }
        }
    };

    ptree pt;
    read_xml(file.c_str(), pt);

    vector<SceneNode> sceneGraph;
    for (const ptree::value_type & sceneGraphList : pt.get_child("entities", empty_ptree())){
        sceneGraph.emplace_back();
        readNode(sceneGraphList, sceneGraph.back());
    }

    for (const SceneNode& rootNode : sceneGraph) {
        // This may not be needed;
        assert(Util::CompareIgnoreCase(rootNode.type, "ROOT"));
        scene->addSceneGraph(rootNode);
    }
    //loadTerrain(sceneLocation, pt.get("terrain", ""), scene);
}


};  // namespace XML
};  // namespace Divide

