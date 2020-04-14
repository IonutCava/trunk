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

namespace detail {
    bool LoadSave::read(const stringImpl& path) {
        _loadPath = path;
        read_xml(_loadPath.c_str(), XmlTree, boost::property_tree::xml_parser::trim_whitespace);
        return !XmlTree.empty();
    }

    bool LoadSave::prepareSaveFile(const stringImpl& path) const {
        _savePath = path;
        return createFile(_savePath.c_str(), true);
    }

    void LoadSave::write() const {
        write_xml(_savePath.c_str(),
            XmlTree,
            std::locale(),
            boost::property_tree::xml_writer_make_settings<boost::property_tree::iptree::key_type>('\t', 1));
    }
};

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
    vectorSTD<std::string> actionsOut;
    for (const auto it : actionNames) {
        const std::string actionList = attributes.get<std::string>(it.second, "");
        Util::Split<vectorSTD<std::string>, std::string>(actionList.c_str(), ',', actionsOut);
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

void loadDefaultKeyBindings(const stringImpl &file, Scene* scene) {
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

    std::function<void(const ptree& rootNode, SceneNode& graphOut)> readNode;

    readNode = [&readNode](const ptree& rootNode, SceneNode& graphOut) {
        const ptree& attributes = rootNode.get_child("<xmlattr>", empty_ptree());
        for (const ptree::value_type& attribute : attributes) {
            if (attribute.first == "name") {
                graphOut.name = attribute.second.data();
            } else if (attribute.first == "type") {
                graphOut.type = attribute.second.data();
            }
        }
        
        const ptree& children = rootNode.get_child("");
        for (const ptree::value_type& child : children) {
            if (child.first == "node") {
                graphOut.children.emplace_back();
                readNode(child.second, graphOut.children.back());
            }
        }
    };

    ptree pt;
    read_xml(file.c_str(), pt);

    SceneNode rootNode = {};
    for (const ptree::value_type& sceneGraphList : pt.get_child("entities", empty_ptree())) {
        readNode(sceneGraphList.second, rootNode);
        // This may not be needed;
        assert(Util::CompareIgnoreCase(rootNode.type, "ROOT"));
        break;
    }

    scene->addSceneGraphToLoad(rootNode);
}


};  // namespace XML
};  // namespace Divide

