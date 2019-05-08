#include "stdafx.h"

#include "Headers/Scene.h"

#include "Editor/Headers/Editor.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/XMLEntryData.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Debugging/Headers/DebugInterface.h"

#include "Utility/Headers/XMLParser.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Headers/Renderer.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"

#include "Environment/Sky/Headers/Sky.h"
#include "Environment/Water/Headers/Water.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/InfinitePlane.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"

#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Predefined/Headers/Box3D.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"
#include "Geometry/Shapes/Predefined/Headers/Sphere3D.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIConsole.h"


#include "ECS/Components/Headers/UnitComponent.h"
#include "ECS/Components/Headers/BoundsComponent.h"
#include "ECS/Components/Headers/SelectionComponent.h"
#include "ECS/Components/Headers/TransformComponent.h"
#include "ECS/Components/Headers/RigidBodyComponent.h"
#include "ECS/Components/Headers/NavigationComponent.h"
#include "ECS/Components/Headers/SpotLightComponent.h"
#include "ECS/Components/Headers/PointLightComponent.h"
#include "ECS/Components/Headers/DirectionalLightComponent.h"

#include "Dynamics/Entities/Units/Headers/Player.h"

#include "Platform/Headers/PlatformRuntime.h"
#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

#include "Physics/Headers/PhysicsSceneInterface.h"

namespace Divide {

namespace {

vec3<F32> g_PlayerExtents(1.0f, 1.82f, 0.75f);

struct selectionQueueDistanceFrontToBack {
    selectionQueueDistanceFrontToBack(const vec3<F32>& eyePos)
        : _eyePos(eyePos) {}

    bool operator()(SceneGraphNode* a, SceneGraphNode* b) const {
        F32 dist_a =
            a->get<BoundsComponent>()->getBoundingBox().nearestDistanceFromPointSquared(_eyePos);
        F32 dist_b =
            b->get<BoundsComponent>()->getBoundingBox().nearestDistanceFromPointSquared(_eyePos);
        return dist_a > dist_b;
    }

   private:
    vec3<F32> _eyePos;
};

constexpr const char* const g_defaultPlayerName = "Player_%d";
};

Scene::Scene(PlatformContext& context, ResourceCache& cache, SceneManager& parent, const stringImpl& name)
    : Resource(ResourceType::DEFAULT, name),
      PlatformContextComponent(context),
      _parent(parent),
      _resCache(cache),
      _LRSpeedFactor(5.0f),
      _loadComplete(false),
      _cookCollisionMeshesScheduled(false),
      _pxScene(nullptr),
      _sun(nullptr),
      _paramHandler(ParamHandler::instance())
{
    _sceneTimerUS = 0UL;
    _sceneState = MemoryManager_NEW SceneState(*this);
    _input = MemoryManager_NEW SceneInput(*this, _context);
    _sceneGraph = MemoryManager_NEW SceneGraph(*this);
    _aiManager = MemoryManager_NEW AI::AIManager(*this, _context.taskPool(TaskPoolType::HIGH_PRIORITY));
    _lightPool = MemoryManager_NEW LightPool(*this, _context);
    _envProbePool = MemoryManager_NEW SceneEnvironmentProbePool(*this);

    _GUI = MemoryManager_NEW SceneGUIElements(*this, _context.gui());

    _loadingTasks.store(0);

    if (Config::Build::IS_DEBUG_BUILD) {
        RenderStateBlock primitiveDescriptor;
        _linesPrimitive = _context.gfx().newIMP();
        _linesPrimitive->name("LinesRayPick");
        PipelineDescriptor pipeDesc;
        pipeDesc._stateHash = primitiveDescriptor.getHash();
        pipeDesc._shaderProgramHandle = ShaderProgram::defaultShader()->getID();

        _linesPrimitive->pipeline(*_context.gfx().newPipeline(pipeDesc));
    } else {
        _linesPrimitive = nullptr;
    }

}

Scene::~Scene()
{
    MemoryManager::DELETE(_sceneState);
    MemoryManager::DELETE(_input);
    MemoryManager::DELETE(_sceneGraph);
    MemoryManager::DELETE(_aiManager);
    MemoryManager::DELETE(_lightPool);
    MemoryManager::DELETE(_envProbePool);
    MemoryManager::DELETE(_GUI);
    for (IMPrimitive*& prim : _octreePrimitives) {
        MemoryManager::DELETE(prim);
    }
}

bool Scene::onStartup() {
    return true;
}

bool Scene::onShutdown() {
    return true;
}

bool Scene::frameStarted() {
    UniqueLock lk(_perFrameArenaMutex);
    _perFrameArena.clear();
    return true;
}

bool Scene::frameEnded() {
    return true;
}

bool Scene::idle() {  // Called when application is idle
    _sceneGraph->idle();

    Attorney::SceneRenderStateScene::playAnimations(renderState(), _context.configPtr()->debug.mesh.playAnimations);

    if (_cookCollisionMeshesScheduled && checkLoadFlag()) {
        if (_context.gfx().getFrameCount() > 1) {
            _sceneGraph->getRoot().get<RigidBodyComponent>()->cookCollisionMesh(resourceName());
            _cookCollisionMeshesScheduled = false;
        }
    }

    _lightPool->idle();

    {
        SharedLock r_lock(_tasksMutex);
        if (_tasks.empty()) {
            return true;
        }
    }

    UniqueLockShared r_lock(_tasksMutex);
    _tasks.erase(std::remove_if(eastl::begin(_tasks),
                                eastl::end(_tasks),
                                [](Task* handle) -> bool { 
                                    return handle != nullptr && Finished(*handle);
                                }),
                eastl::end(_tasks));

    return true;
}

void Scene::addMusic(MusicType type, const stringImpl& name, const stringImpl& srcFile) {

    FileWithPath fileResult = splitPathToNameAndLocation(srcFile);
    const stringImpl& musicFile = fileResult._fileName;
    const stringImpl& musicFilePath = fileResult._path;

    ResourceDescriptor music(name);
    music.assetName(musicFile);
    music.assetLocation(musicFilePath);
    music.setFlag(true);
    hashAlg::insert(state().music(type),
                    _ID(name.c_str()),
                    CreateResource<AudioDescriptor>(_resCache, music));
}


bool Scene::saveXML() const {
    using boost::property_tree::ptree;

    Console::printfn(Locale::get(_ID("XML_SAVE_SCENE_START")), resourceName().c_str());

    const stringImpl& scenePath = Paths::g_xmlDataLocation + Paths::g_scenesLocation;
    const boost::property_tree::xml_writer_settings<std::string> settings(' ', 4);

    stringImpl sceneLocation(scenePath + "/" + resourceName().c_str());
    stringImpl sceneDataFile(sceneLocation + ".xml");

    createDirectory((sceneLocation + "/collisionMeshes/").c_str());
    createDirectory((sceneLocation + "/navMeshes/").c_str());
    createDirectory((sceneLocation + "/nodes/").c_str());

    // A scene does not necessarily need external data files
    // Data can be added in code for simple scenes
    {
        ParamHandler& par = ParamHandler::instance();

        ptree pt;
        pt.put("assets", "assets.xml");
        pt.put("musicPlaylist", "musicPlaylist.xml");

        pt.put("vegetation.grassVisibility", state().renderState().grassVisibility());
        pt.put("vegetation.treeVisibility", state().renderState().treeVisibility());

        pt.put("wind.windDirX", state().windDirX());
        pt.put("wind.windDirZ", state().windDirZ());
        pt.put("wind.windSpeed", state().windSpeed());

        if (!state().globalWaterBodies().empty()) {
            pt.put("water.waterLevel", state().globalWaterBodies()[0]._heightOffset);
            pt.put("water.waterDepth", state().globalWaterBodies()[0]._depth);
        }

        pt.put("options.visibility", state().renderState().generalVisibility());
        pt.put("options.cameraSpeed.<xmlattr>.move", par.getParam<F32>(_ID((resourceName() + ".options.cameraSpeed.move").c_str())));
        pt.put("options.cameraSpeed.<xmlattr>.turn", par.getParam<F32>(_ID((resourceName() + ".options.cameraSpeed.turn").c_str())));
        pt.put("options.autoCookPhysicsAssets", true);

        pt.put("fog.fogDensity", state().renderState().fogDescriptor().density());
        pt.put("fog.fogColour.<xmlattr>.r", state().renderState().fogDescriptor().colour().r);
        pt.put("fog.fogColour.<xmlattr>.g", state().renderState().fogDescriptor().colour().g);
        pt.put("fog.fogColour.<xmlattr>.b", state().renderState().fogDescriptor().colour().b);

        pt.put("lod.lodThresholds.<xmlattr>.x", state().renderState().lodThresholds().x);
        pt.put("lod.lodThresholds.<xmlattr>.y", state().renderState().lodThresholds().y);
        pt.put("lod.lodThresholds.<xmlattr>.z", state().renderState().lodThresholds().z);
        pt.put("lod.lodThresholds.<xmlattr>.w", state().renderState().lodThresholds().w);

        copyFile(scenePath, resourceName() + ".xml", scenePath, resourceName() + ".xml.bak", true);
        write_xml(sceneDataFile.c_str(), pt, std::locale(), settings);
    }
    sceneGraph().saveToXML();

    //save music
    {
        ptree pt;
        copyFile(sceneLocation + "/", "musicPlaylist.xml", sceneLocation + "/", "musicPlaylist.xml.bak", true);
        write_xml((sceneLocation + "/" + "musicPlaylist.xml.dev").c_str(), pt, std::locale(), settings);
    }

    Console::printfn(Locale::get(_ID("XML_SAVE_SCENE_END")), resourceName().c_str());

    return true;
}

namespace {
    bool isPrimitive(const stringImpl& modelName) {
        static const stringImpl pritimiveNames[] = {
            "BOX_3D",
            //"PATCH_3D", <- Internal
            "QUAD_3D",
            "SPHERE_3D"
        };
        for (const stringImpl& it : pritimiveNames) {
            if (Util::CompareIgnoreCase(modelName, it)) {
                return true;
            }
        }

        return false;
    }
};


void Scene::loadAsset(Task& parentTask, const XML::SceneNode& sceneNode, SceneGraphNode* parent, bool waitForReady) {
    assert(parent != nullptr);

    auto waitForReasoureTask = [&parentTask](const CachedResource_wptr& res) {
        ACKNOWLEDGE_UNUSED(res);
        TaskYield(parentTask);
    };

    const stringImpl& scenePath = Paths::g_xmlDataLocation + Paths::g_scenesLocation;
    stringImpl sceneLocation(scenePath + "/" + resourceName().c_str());
    stringImpl nodePath = sceneLocation + "/nodes/" + parent->name() + "_" + sceneNode.name + ".xml";

    SceneGraphNode* crtNode = parent;
    if (fileExists(nodePath.c_str())) {

        U32 normalMask = to_base(ComponentType::TRANSFORM) |
                         to_base(ComponentType::BOUNDS) |
                         to_base(ComponentType::NETWORKING);


        boost::property_tree::ptree nodeTree;
        read_xml(nodePath, nodeTree);

        auto loadModelComplete = [this, &nodeTree](CachedResource_wptr res) {
            ACKNOWLEDGE_UNUSED(res);
            std::static_pointer_cast<SceneNode>(res.lock())->loadFromXML(nodeTree);
            _loadingTasks.fetch_sub(1);
        };

        stringImpl modelName = nodeTree.get("model", "");

        SceneNode_ptr ret = nullptr;

        bool skipAdd = true;
        if (sceneNode.type == "ROOT") {
            // Nothing to do with the root. It's good that it exists
        } else if (isPrimitive(sceneNode.type)) {// Primitive types (only top level)
            normalMask |= to_base(ComponentType::RENDERING);

            if (!modelName.empty()) {
                _loadingTasks.fetch_add(1);
                ResourceDescriptor item(sceneNode.name);
                item.setOnLoadCallback(loadModelComplete);
                item.assetName(modelName);
                item.waitForReadyCbk(waitForReasoureTask);
                //item.waitForReady(waitForReady);

                if (Util::CompareIgnoreCase(modelName, "BOX_3D")) {
                    ret = CreateResource<Box3D>(_resCache, item);
                } else if (Util::CompareIgnoreCase(modelName, "SPHERE_3D")) {
                    ret = CreateResource<Sphere3D>(_resCache, item);
                } else if (Util::CompareIgnoreCase(modelName, "QUAD_3D")) {
                    P32 quadMask;
                    quadMask.i = 0;
                    quadMask.b[0] = 1;
                    item.setBoolMask(quadMask);
                    ret = CreateResource<Quad3D>(_resCache, item);
                    static_cast<Quad3D*>(ret.get())->setCorner(Quad3D::CornerLocation::TOP_LEFT, vec3<F32>(0, 1, 0));
                    static_cast<Quad3D*>(ret.get())->setCorner(Quad3D::CornerLocation::TOP_RIGHT, vec3<F32>(1, 1, 0));
                    static_cast<Quad3D*>(ret.get())->setCorner(Quad3D::CornerLocation::BOTTOM_LEFT, vec3<F32>(0, 0, 0));
                    static_cast<Quad3D*>(ret.get())->setCorner(Quad3D::CornerLocation::BOTTOM_RIGHT, vec3<F32>(1, 0, 0));
                } else {
                    Console::errorfn(Locale::get(_ID("ERROR_SCENE_UNSUPPORTED_GEOM")), sceneNode.name.c_str());
                }
            }
            if (ret != nullptr) {
                ResourceDescriptor materialDescriptor(sceneNode.name + "_material");
                Material_ptr tempMaterial = CreateResource<Material>(_resCache, materialDescriptor);
                tempMaterial->setShadingMode(Material::ShadingMode::BLINN_PHONG);
                ret->setMaterialTpl(tempMaterial);
            }
            skipAdd = false;
        }
        // Terrain types
        else if (sceneNode.type == "TERRAIN") {
            normalMask |= to_base(ComponentType::RENDERING);
            addTerrain(*parent, nodeTree, sceneNode.name);
        }
        else if (sceneNode.type == "VEGETATION_GRASS") {
            normalMask |= to_base(ComponentType::RENDERING);
            NOP(); //we rebuild grass everytime
        }
        else if (sceneNode.type == "INFINITE_PLANE") {
            normalMask |= to_base(ComponentType::RENDERING);
            addInfPlane(*parent, nodeTree, sceneNode.name);
        } 
        else if (sceneNode.type == "WATER") {
            normalMask |= to_base(ComponentType::RENDERING);
            addWater(*parent, nodeTree, sceneNode.name);
        }
        // Mesh types
        else if (Util::CompareIgnoreCase(sceneNode.type, "MESH")) {
            // No rendering component for meshes. Only for submeshes
            //normalMask |= to_base(ComponentType::RENDERING);
            if (!modelName.empty()) {
                _loadingTasks.fetch_add(1);
                ResourceDescriptor model(modelName);
                model.assetLocation(Paths::g_assetsLocation + "models");
                model.assetName(modelName);
                model.setFlag(true);
                model.setThreadedLoading(true);
                //model.waitForReady(waitForReady);
                model.waitForReadyCbk(waitForReasoureTask);
                model.setOnLoadCallback(loadModelComplete);
                ret = CreateResource<Mesh>(_resCache, model);
            }

            skipAdd = false;
        }
        // Submesh (change component properties, as the meshes should already be loaded)
        else if (Util::CompareIgnoreCase(sceneNode.type, "SUBMESH")) {
            normalMask |= to_base(ComponentType::RENDERING);
            SceneGraphNode* subMesh = parent->findChild(sceneNode.name, false, false);
            if (subMesh != nullptr) {
                subMesh->loadFromXML(nodeTree);
            }
        }
        // Sky
        else if (Util::CompareIgnoreCase(sceneNode.type, "SKY")) {
            //ToDo: Change this - Currently, just load the default sky.
            normalMask |= to_base(ComponentType::RENDERING);
            addSky(*parent, nodeTree, sceneNode.name);
        }
        // Everything else
        else if (Util::CompareIgnoreCase(sceneNode.type, "")) {
            skipAdd = false;
        }

        if (!skipAdd) {
            SceneGraphNodeDescriptor nodeDescriptor = {};
            nodeDescriptor._name = sceneNode.name;
            nodeDescriptor._componentMask = normalMask;
            nodeDescriptor._node = ret;

            for (U8 i = 1; i < to_U8(ComponentType::COUNT); ++i) {
                ComponentType type = ComponentType::_from_integral(1u << i);
                if (nodeTree.count(type._to_string()) != 0) {
                    nodeDescriptor._componentMask |= 1 << i;
                }
            }

            crtNode = parent->addNode(nodeDescriptor);
            crtNode->loadFromXML(nodeTree);
        }
    }

    for (const XML::SceneNode& node : sceneNode.children) {
        //loadAsset(parentTask, node, crtNode, waitForReady);
        Start(*CreateTask(*parentTask._parentPool, &parentTask, [this, node, waitForReady, crtNode](Task& subTask) -> void {
            loadAsset(subTask, node, crtNode, waitForReady);
        }));
    }
}

SceneGraphNode* Scene::addParticleEmitter(const stringImpl& name,
                                          std::shared_ptr<ParticleData> data,
                                          SceneGraphNode& parentNode) {
    DIVIDE_ASSERT(!name.empty(),
                  "Scene::addParticleEmitter error: invalid name specified!");

    ResourceDescriptor particleEmitter(name);
    std::shared_ptr<ParticleEmitter> emitter = CreateResource<ParticleEmitter>(_resCache, particleEmitter);

    DIVIDE_ASSERT(emitter != nullptr,
                  "Scene::addParticleEmitter error: Could not instantiate emitter!");

    if (Runtime::isMainThread()) {
        emitter->initData(data);
    } else {
        _context.app().mainThreadTask([&emitter, &data] { emitter->initData(data); });
    }

    SceneGraphNodeDescriptor particleNodeDescriptor;
    particleNodeDescriptor._node = emitter;
    particleNodeDescriptor._usageContext = NodeUsageContext::NODE_DYNAMIC;
    particleNodeDescriptor._componentMask = to_base(ComponentType::TRANSFORM) |
                                            to_base(ComponentType::BOUNDS) |
                                            to_base(ComponentType::RENDERING) |
                                            to_base(ComponentType::NETWORKING) |
                                            to_base(ComponentType::SELECTION);

    return parentNode.addNode(particleNodeDescriptor);
}

void Scene::addTerrain(SceneGraphNode& parentNode, boost::property_tree::ptree pt, const stringImpl& name) {
    Console::printfn(Locale::get(_ID("XML_LOAD_TERRAIN")), name.c_str());

    // Load the rest of the terrain
    std::shared_ptr<TerrainDescriptor> ter = std::make_shared<TerrainDescriptor>((name + "_descriptor").c_str());
    ter->addVariable("terrainName", name.c_str());
    ter->addVariable("heightmap", pt.get<stringImpl>("heightmap"));
    ter->addVariable("heightmapLocation", pt.get<stringImpl>("heightmapLocation", Paths::g_heightmapLocation));
    ter->addVariable("textureLocation", pt.get<stringImpl>("textureLocation", Paths::g_imagesLocation));
    ter->addVariable("waterCaustics", pt.get<stringImpl>("waterCaustics"));
    ter->addVariable("underwaterAlbedoTexture", pt.get<stringImpl>("underwaterAlbedoTexture"));
    ter->addVariable("underwaterDetailTexture", pt.get<stringImpl>("underwaterDetailTexture"));
    ter->addVariable("underwaterTileScale", pt.get<F32>("underwaterTileScale"));

    I32 i = 0;
    stringImpl temp;
    stringImpl layerOffsetStr;
    for (boost::property_tree::ptree::iterator itTexture = std::begin(pt.get_child("textureLayers"));
        itTexture != std::end(pt.get_child("textureLayers"));
        ++itTexture, ++i) {
        stringImpl layerName(itTexture->second.data());
        stringImpl format(itTexture->first.data());

        if (format.find("<xmlcomment>") != stringImpl::npos) {
            i--;
            continue;
        }

        layerName = "textureLayers." + format;

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

        ter->addVariable("redTileScale" + layerOffsetStr, pt.get<F32>(layerName + ".redTileScale", 1.0f));
        ter->addVariable("greenTileScale" + layerOffsetStr, pt.get<F32>(layerName + ".greenTileScale", 1.0f));
        ter->addVariable("blueTileScale" + layerOffsetStr, pt.get<F32>(layerName + ".blueTileScale", 1.0f));
        ter->addVariable("alphaTileScale" + layerOffsetStr, pt.get<F32>(layerName + ".alphaTileScale", 1.0f));
    }
    
    ter->setTextureLayerCount(to_U8(i));
    ter->addVariable("vegetationTextureLocation", pt.get<stringImpl>("vegetation.vegetationTextureLocation", Paths::g_imagesLocation));
    ter->addVariable("grassMap", pt.get<stringImpl>("vegetation.grassMap"));
    ter->addVariable("treeMap", pt.get<stringImpl>("vegetation.treeMap"));

    for (I32 j = 1; j < 5; ++j) {
        ter->addVariable(Util::StringFormat("grassBillboard%d", j), pt.get<stringImpl>(Util::StringFormat("vegetation.grassBillboard%d", j), ""));
        ter->addVariable(Util::StringFormat("grassScale%d", j), pt.get<F32>(Util::StringFormat("vegetation.grassBillboard%d.<xmlattr>.scale", j), 1.0f));

        ter->addVariable(Util::StringFormat("treeMesh%d", j), pt.get<stringImpl>(Util::StringFormat("vegetation.treeMesh%d",j), ""));
        ter->addVariable(Util::StringFormat("treeScale%d", j), pt.get<F32>(Util::StringFormat("vegetation.treeMesh%d.<xmlattr>.scale", j), 1.0f));
        ter->addVariable(Util::StringFormat("treeRotationX%d", j), pt.get<F32>(Util::StringFormat("vegetation.treeMesh%d.<xmlattr>.rotate_x", j), 0.0f));
        ter->addVariable(Util::StringFormat("treeRotationY%d", j), pt.get<F32>(Util::StringFormat("vegetation.treeMesh%d.<xmlattr>.rotate_y", j), 0.0f));
        ter->addVariable(Util::StringFormat("treeRotationZ%d", j), pt.get<F32>(Util::StringFormat("vegetation.treeMesh%d.<xmlattr>.rotate_z", j), 0.0f));
    }

    ter->set16Bit(pt.get<bool>("is16Bit", false));
    ter->setWireframeDebug(pt.get<bool>("wireframeDebugMode", false));
    ter->setDimensions(vec2<U16>(pt.get<U16>("terrainWidth", 0), pt.get<U16>("terrainHeight", 0)));
    ter->setAltitudeRange(vec2<F32>(pt.get<F32>("altitudeRange.<xmlattr>.min", 0.0f),
                                    pt.get<F32>("altitudeRange.<xmlattr>.max", 255.0f)));
    ter->setTessellationRange(vec3<F32>(pt.get<F32>("tessellationRange.<xmlattr>.min", 10.0f),
                                        pt.get<F32>("tessellationRange.<xmlattr>.max", 150.0f),
                                        pt.get<F32>("tessellationRange.<xmlattr>.chunkSize", 32.0f)));


    auto registerTerrain = [this, name, &parentNode, pt](CachedResource_wptr res) {
        SceneGraphNodeDescriptor terrainNodeDescriptor;
        terrainNodeDescriptor._name = name;
        terrainNodeDescriptor._node = std::static_pointer_cast<Terrain>(res.lock());
        terrainNodeDescriptor._usageContext = NodeUsageContext::NODE_STATIC;
        terrainNodeDescriptor._componentMask = to_base(ComponentType::NAVIGATION) |
                                               to_base(ComponentType::TRANSFORM) |
                                               to_base(ComponentType::RIGID_BODY) |
                                               to_base(ComponentType::BOUNDS) |
                                               to_base(ComponentType::RENDERING) |
                                               to_base(ComponentType::NETWORKING);

        terrainNodeDescriptor._node->loadFromXML(pt);

        SceneGraphNode* terrainTemp = parentNode.addNode(terrainNodeDescriptor);


        NavigationComponent* nComp = terrainTemp->get<NavigationComponent>();
        nComp->navigationContext(NavigationComponent::NavigationContext::NODE_OBSTACLE);

        terrainTemp->get<RigidBodyComponent>()->physicsGroup(PhysicsGroup::GROUP_STATIC);
        terrainTemp->loadFromXML(pt);
        _loadingTasks.fetch_sub(1);
    };

    ResourceDescriptor terrainDescriptor(ter->getVariable("terrainName"));
    terrainDescriptor.setPropertyDescriptor(*ter);
    terrainDescriptor.setThreadedLoading(true);
    terrainDescriptor.setOnLoadCallback(registerTerrain);
    terrainDescriptor.setFlag(ter->getActive());
    terrainDescriptor.waitForReady(false);
    _loadingTasks.fetch_add(1);
    CreateResource<Terrain>(_resCache, terrainDescriptor);
}

void Scene::toggleFlashlight(PlayerIndex idx) {
    SceneGraphNode*& flashLight = _flashLight[idx];
    if (!flashLight) {
        SceneGraphNodeDescriptor lightNodeDescriptor;
        lightNodeDescriptor._serialize = false;
        lightNodeDescriptor._name = Util::StringFormat("Flashlight_%d", idx);
        lightNodeDescriptor._usageContext = NodeUsageContext::NODE_DYNAMIC;
        lightNodeDescriptor._componentMask = to_base(ComponentType::TRANSFORM) |
                                             to_base(ComponentType::BOUNDS) |
                                             to_base(ComponentType::NETWORKING) |
                                             to_base(ComponentType::SPOT_LIGHT);
        flashLight = _sceneGraph->getRoot().addNode(lightNodeDescriptor);
        SpotLightComponent* spotLight = flashLight->get<SpotLightComponent>();
        spotLight->castsShadows(true);
        spotLight->setDiffuseColour(DefaultColours::WHITE);

        hashAlg::insert(_flashLight, idx, flashLight);

        _cameraUpdateMap[idx] = playerCamera(idx)->addUpdateListener([this, idx](const Camera& cam) { 
            if (idx < _scenePlayers.size() && idx < _flashLight.size() && _flashLight[idx]) {
                if (cam.getGUID() == _scenePlayers[getSceneIndexForPlayer(idx)]->getCamera().getGUID()) {
                    TransformComponent* tComp = _flashLight[idx]->get<TransformComponent>();
                    tComp->setPosition(cam.getEye());
                    tComp->setRotationEuler(cam.getEuler());
                }
            }
        });
                     
    }

    flashLight->get<SpotLightComponent>()->toggleEnabled();
}

SceneGraphNode* Scene::addSky(SceneGraphNode& parentNode, boost::property_tree::ptree pt, const stringImpl& nodeName) {
    ResourceDescriptor skyDescriptor("DefaultSky_"+ nodeName);
    skyDescriptor.setID(to_U32(std::floor(Camera::utilityCamera(Camera::UtilityCamera::DEFAULT)->getZPlanes().y * 2)));

    std::shared_ptr<Sky> skyItem = CreateResource<Sky>(_resCache, skyDescriptor);
    DIVIDE_ASSERT(skyItem != nullptr, "Scene::addSky error: Could not create sky resource!");
    skyItem->loadFromXML(pt);

    SceneGraphNodeDescriptor skyNodeDescriptor;
    skyNodeDescriptor._node = skyItem;
    skyNodeDescriptor._name = nodeName;
    skyNodeDescriptor._usageContext = NodeUsageContext::NODE_DYNAMIC;
    skyNodeDescriptor._componentMask = to_base(ComponentType::TRANSFORM) |
                                       to_base(ComponentType::BOUNDS) |
                                       to_base(ComponentType::RENDERING) |
                                       to_base(ComponentType::NETWORKING);

    SceneGraphNode* skyNode = parentNode.addNode(skyNodeDescriptor);
    skyNode->lockVisibility(true);
    skyNode->loadFromXML(pt);

    return skyNode;
}

void Scene::addWater(SceneGraphNode& parentNode, boost::property_tree::ptree pt, const stringImpl& nodeName) {
    auto registerWater = [this, nodeName, &parentNode, pt](CachedResource_wptr res) {
        SceneGraphNodeDescriptor waterNodeDescriptor;
        waterNodeDescriptor._name = nodeName;
        waterNodeDescriptor._node = std::static_pointer_cast<WaterPlane>(res.lock());
        waterNodeDescriptor._usageContext = NodeUsageContext::NODE_STATIC;
        waterNodeDescriptor._componentMask = to_base(ComponentType::NAVIGATION) |
                                            to_base(ComponentType::TRANSFORM) |
                                            to_base(ComponentType::RIGID_BODY) |
                                            to_base(ComponentType::BOUNDS) |
                                            to_base(ComponentType::RENDERING) |
                                            to_base(ComponentType::NETWORKING);

        waterNodeDescriptor._node->loadFromXML(pt);

        SceneGraphNode* waterNode = parentNode.addNode(waterNodeDescriptor);


        NavigationComponent* nComp = waterNode->get<NavigationComponent>();
        nComp->navigationContext(NavigationComponent::NavigationContext::NODE_OBSTACLE);

        waterNode->get<RigidBodyComponent>()->physicsGroup(PhysicsGroup::GROUP_STATIC);
        waterNode->loadFromXML(pt);
        _loadingTasks.fetch_sub(1);
    };

    ResourceDescriptor waterDescriptor("Water_" + nodeName);
    waterDescriptor.setThreadedLoading(true);
    waterDescriptor.setOnLoadCallback(registerWater);
    waterDescriptor.waitForReady(false);
    _loadingTasks.fetch_add(1);

    CreateResource<WaterPlane>(_resCache, waterDescriptor);
}

SceneGraphNode* Scene::addInfPlane(SceneGraphNode& parentNode, boost::property_tree::ptree pt, const stringImpl& nodeName) {
    ResourceDescriptor planeDescriptor("InfPlane_" + nodeName);

    Camera* baseCamera = Camera::utilityCamera(Camera::UtilityCamera::DEFAULT);

    planeDescriptor.setID(to_U32(baseCamera->getZPlanes().max));
    std::shared_ptr<InfinitePlane> planeItem = CreateResource<InfinitePlane>(_resCache, planeDescriptor);
    DIVIDE_ASSERT(planeItem != nullptr, "Scene::addInfPlane error: Could not create infinite plane resource!");
    planeItem->loadFromXML(pt);

    SceneGraphNodeDescriptor planeNodeDescriptor;
    planeNodeDescriptor._node = planeItem;
    planeNodeDescriptor._name = nodeName;
    planeNodeDescriptor._usageContext = NodeUsageContext::NODE_STATIC;
    planeNodeDescriptor._componentMask = to_base(ComponentType::TRANSFORM) |
                                         to_base(ComponentType::BOUNDS) |
                                         to_base(ComponentType::RENDERING);

    auto ret = parentNode.addNode(planeNodeDescriptor);
    ret->loadFromXML(pt);
    return ret;
}

U16 Scene::registerInputActions() {
    _input->flushCache();

    auto none = [](InputParams param) {};
    auto deleteSelection = [this](InputParams param) { 
        PlayerIndex idx = getPlayerIndexForDevice(param._deviceIndex);
        if (!_currentSelection[idx].empty()) {
            _sceneGraph->removeNode(_currentSelection[idx].front());
        }
    };
    auto increaseCameraSpeed = [this](InputParams param){
        Camera& cam = _scenePlayers[getPlayerIndexForDevice(param._deviceIndex)]->getCamera();

        F32 currentCamMoveSpeedFactor = cam.getMoveSpeedFactor();
        if (currentCamMoveSpeedFactor < 50) {
            cam.setMoveSpeedFactor(currentCamMoveSpeedFactor + 1.0f);
            cam.setTurnSpeedFactor(cam.getTurnSpeedFactor() + 1.0f);
        }
    };
    auto decreaseCameraSpeed = [this](InputParams param) {
        Camera& cam = _scenePlayers[getPlayerIndexForDevice(param._deviceIndex)]->getCamera();

        F32 currentCamMoveSpeedFactor = cam.getMoveSpeedFactor();
        if (currentCamMoveSpeedFactor > 1.0f) {
            cam.setMoveSpeedFactor(currentCamMoveSpeedFactor - 1.0f);
            cam.setTurnSpeedFactor(cam.getTurnSpeedFactor() - 1.0f);
        }
    };
    auto increaseResolution = [this](InputParams param) {_context.gfx().increaseResolution();};
    auto decreaseResolution = [this](InputParams param) {_context.gfx().decreaseResolution();};
    auto moveForward = [this](InputParams param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveFB(MoveDirection::POSITIVE);};
    auto moveBackwards = [this](InputParams param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveFB(MoveDirection::NEGATIVE);};
    auto stopMoveFWDBCK = [this](InputParams param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveFB(MoveDirection::NONE);};
    auto strafeLeft = [this](InputParams param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveLR(MoveDirection::NEGATIVE);};
    auto strafeRight = [this](InputParams param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveLR(MoveDirection::POSITIVE);};
    auto stopStrafeLeftRight = [this](InputParams param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveLR(MoveDirection::NONE);};
    auto rollCCW = [this](InputParams param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).roll(MoveDirection::POSITIVE);};
    auto rollCW = [this](InputParams param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).roll(MoveDirection::NEGATIVE);};
    auto stopRollCCWCW = [this](InputParams param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).roll(MoveDirection::NONE);};
    auto turnLeft = [this](InputParams param) { state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleLR(MoveDirection::NEGATIVE);};
    auto turnRight = [this](InputParams param) { state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleLR(MoveDirection::POSITIVE);};
    auto stopTurnLeftRight = [this](InputParams param) { state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleLR(MoveDirection::NONE);};
    auto turnUp = [this](InputParams param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleUD(MoveDirection::NEGATIVE);};
    auto turnDown = [this](InputParams param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleUD(MoveDirection::POSITIVE);};
    auto stopTurnUpDown = [this](InputParams param) {state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleUD(MoveDirection::NONE);};
    auto togglePauseState = [](InputParams param){
        ParamHandler& par = ParamHandler::instance();
        par.setParam(_ID("freezeLoopTime"), !par.getParam(_ID("freezeLoopTime"), false));
    };
    auto toggleDepthOfField = [this](InputParams param) {
        PostFX& postFX = _context.gfx().postFX();
        if (postFX.getFilterState(FilterType::FILTER_DEPTH_OF_FIELD)) {
            postFX.popFilter(FilterType::FILTER_DEPTH_OF_FIELD);
        } else {
            postFX.pushFilter(FilterType::FILTER_DEPTH_OF_FIELD);
        }
    };
    auto toggleBloom = [this](InputParams param) {
        PostFX& postFX = _context.gfx().postFX();
        if (postFX.getFilterState(FilterType::FILTER_BLOOM)) {
            postFX.popFilter(FilterType::FILTER_BLOOM);
        } else {
            postFX.pushFilter(FilterType::FILTER_BLOOM);
        }
    };
    auto toggleSkeletonRendering = [this](InputParams param) {renderState().toggleOption(SceneRenderState::RenderOptions::RENDER_SKELETONS);};
    auto toggleAxisLineRendering = [this](InputParams param) {renderState().toggleAxisLines();};
    auto toggleWireframeRendering = [this](InputParams param) {renderState().toggleOption(SceneRenderState::RenderOptions::RENDER_WIREFRAME);};
    auto toggleGeometryRendering = [this](InputParams param) { renderState().toggleOption(SceneRenderState::RenderOptions::RENDER_GEOMETRY);};
    auto toggleDebugLines = [this](InputParams param) {renderState().toggleOption(SceneRenderState::RenderOptions::RENDER_DEBUG_LINES);};
    auto toggleBoundingBoxRendering = [this](InputParams param) {renderState().toggleOption(SceneRenderState::RenderOptions::RENDER_AABB);};
    auto toggleShadowMapDepthBufferPreview = [this](InputParams param) {
        LightPool::togglePreviewShadowMaps(_context.gfx(), *_lightPool->getLights(LightType::DIRECTIONAL)[0]);

        ParamHandler& par = ParamHandler::instance();
        par.setParam<bool>(_ID("rendering.previewDebugViews"),
                          !par.getParam<bool>(_ID("rendering.previewDebugViews"), false));
    };
    auto takeScreenshot = [this](InputParams param) { _context.gfx().Screenshot("screenshot_"); };
    auto toggleFullScreen = [this](InputParams param) { _context.gfx().toggleFullScreen(); };
    auto toggleFlashLight = [this](InputParams param) { toggleFlashlight(getPlayerIndexForDevice(param._deviceIndex)); };
    auto toggleOctreeRegionRendering = [this](InputParams param) {renderState().toggleOption(SceneRenderState::RenderOptions::RENDER_OCTREE_REGIONS);};
    auto select = [this](InputParams  param) {findSelection(getPlayerIndexForDevice(param._deviceIndex), true); };
    auto multiselect = [this](InputParams  param) {findSelection(getPlayerIndexForDevice(param._deviceIndex), false); };
    auto lockCameraToMouse = [this](InputParams  param) { lockCameraToPlayerMouse(getPlayerIndexForDevice(param._deviceIndex), true); };
    auto releaseCameraFromMouse = [this](InputParams  param) { lockCameraToPlayerMouse(getPlayerIndexForDevice(param._deviceIndex), false); };
    auto rendererDebugView = [this](InputParams param) { ACKNOWLEDGE_UNUSED(param); };
    auto shutdown = [this](InputParams param) { _context.app().RequestShutdown();};
    auto povNavigation = [this](InputParams param) {
        U32 povMask = param._var[0];

        if (povMask & to_base(Input::JoystickPovDirection::UP)) {  // Going up
            state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveFB(MoveDirection::POSITIVE);
        }
        if (povMask & to_base(Input::JoystickPovDirection::DOWN)) {  // Going down
            state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveFB(MoveDirection::NEGATIVE);
        }
        if (povMask & to_base(Input::JoystickPovDirection::RIGHT)) {  // Going right
            state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveLR(MoveDirection::POSITIVE);
        }
        if (povMask & to_base(Input::JoystickPovDirection::LEFT)) {  // Going left
            state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveLR(MoveDirection::NEGATIVE);
        }
        if (povMask == to_base(Input::JoystickPovDirection::CENTERED)) {  // stopped/centered out
            state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveLR(MoveDirection::NONE);
            state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveFB(MoveDirection::NONE);
        }
    };

    auto axisNavigation = [this](InputParams param) {
        I32 axisABS = param._var[0];
        I32 axis = param._var[1];
        //bool isGamepad = param._var[2] == 1;
        I32 deadZone = param._var[3];

        switch (axis) {
            case 0: {
                if (axisABS > deadZone) {
                    state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleUD(MoveDirection::POSITIVE);
                } else if (axisABS < -deadZone) {
                    state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleUD(MoveDirection::NEGATIVE);
                } else {
                    state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleUD(MoveDirection::NONE);
                }
            } break;
            case 1: {
                if (axisABS > deadZone) {
                    state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleLR(MoveDirection::POSITIVE);
                } else if (axisABS < -deadZone) {
                    state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleLR(MoveDirection::NEGATIVE);
                } else {
                    state().playerState(getPlayerIndexForDevice(param._deviceIndex)).angleLR(MoveDirection::NONE);
                }
            } break;

            case 2: {
                if (axisABS < -deadZone) {
                    state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveFB(MoveDirection::POSITIVE);
                } else if (axisABS > deadZone) {
                    state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveFB(MoveDirection::NEGATIVE);
                } else {
                    state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveFB(MoveDirection::NONE);
                }
            } break;
            case 3: {
                if (axisABS < -deadZone) {
                    state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveLR(MoveDirection::NEGATIVE);
                } else if (axisABS > deadZone) {
                    state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveLR(MoveDirection::POSITIVE);
                } else {
                    state().playerState(getPlayerIndexForDevice(param._deviceIndex)).moveLR(MoveDirection::NONE);
                }
            } break;
        }
    };

    auto toggleDebugInterface = [this](InputParams param) {
        _context.debug().toggle(!_context.debug().enabled());
    };

    auto toggleEditor = [this](InputParams param) {
        if (Config::Build::ENABLE_EDITOR) {
            _context.editor().toggle(!_context.editor().running());
        }
    };

    auto toggleConsole = [this](InputParams param) {
        if (Config::Build::ENABLE_EDITOR) {
            _context.gui().getConsole().setVisible(!_context.gui().getConsole().isVisible());
        }
    };

    U16 actionID = 0;
    InputActionList& actions = _input->actionList();
    actions.registerInputAction(actionID++, none);
    actions.registerInputAction(actionID++, deleteSelection);
    actions.registerInputAction(actionID++, increaseCameraSpeed);
    actions.registerInputAction(actionID++, decreaseCameraSpeed);
    actions.registerInputAction(actionID++, increaseResolution);
    actions.registerInputAction(actionID++, decreaseResolution);
    actions.registerInputAction(actionID++, moveForward);
    actions.registerInputAction(actionID++, moveBackwards);
    actions.registerInputAction(actionID++, stopMoveFWDBCK);
    actions.registerInputAction(actionID++, strafeLeft);
    actions.registerInputAction(actionID++, strafeRight);
    actions.registerInputAction(actionID++, stopStrafeLeftRight);
    actions.registerInputAction(actionID++, rollCCW);
    actions.registerInputAction(actionID++, rollCW);
    actions.registerInputAction(actionID++, stopRollCCWCW);
    actions.registerInputAction(actionID++, turnLeft);
    actions.registerInputAction(actionID++, turnRight);
    actions.registerInputAction(actionID++, stopTurnLeftRight);
    actions.registerInputAction(actionID++, turnUp);
    actions.registerInputAction(actionID++, turnDown);
    actions.registerInputAction(actionID++, stopTurnUpDown);
    actions.registerInputAction(actionID++, togglePauseState);
    actions.registerInputAction(actionID++, toggleDepthOfField);
    actions.registerInputAction(actionID++, toggleBloom);
    actions.registerInputAction(actionID++, toggleSkeletonRendering);
    actions.registerInputAction(actionID++, toggleAxisLineRendering);
    actions.registerInputAction(actionID++, toggleWireframeRendering);
    actions.registerInputAction(actionID++, toggleGeometryRendering);
    actions.registerInputAction(actionID++, toggleDebugLines);
    actions.registerInputAction(actionID++, toggleBoundingBoxRendering);
    actions.registerInputAction(actionID++, toggleShadowMapDepthBufferPreview);
    actions.registerInputAction(actionID++, takeScreenshot);
    actions.registerInputAction(actionID++, toggleFullScreen);
    actions.registerInputAction(actionID++, toggleFlashLight);
    actions.registerInputAction(actionID++, toggleOctreeRegionRendering);
    actions.registerInputAction(actionID++, select);
    actions.registerInputAction(actionID++, multiselect);
    actions.registerInputAction(actionID++, lockCameraToMouse);
    actions.registerInputAction(actionID++, releaseCameraFromMouse);
    actions.registerInputAction(actionID++, rendererDebugView);
    actions.registerInputAction(actionID++, shutdown);
    actions.registerInputAction(actionID++, povNavigation);
    actions.registerInputAction(actionID++, axisNavigation);
    actions.registerInputAction(actionID++, toggleDebugInterface);
    actions.registerInputAction(actionID++, toggleEditor);
    actions.registerInputAction(actionID++, toggleConsole);
    
    return actionID;
}

void Scene::loadKeyBindings() {
    XML::loadDefaultKeybindings(Paths::g_xmlDataLocation + "keyBindings.xml", this);
}

bool Scene::lockCameraToPlayerMouse(PlayerIndex index, bool lockState) {
    static bool hadWindowGrab = false;
    static vec2<I32> lastMousePosition;

    state().playerState(index).cameraLockedToMouse(lockState);

    DisplayWindow* window = _context.app().windowManager().getFocusedWindow();
    if (lockState) {
        if (window != nullptr) {
            hadWindowGrab = window->grabState();
        }
        lastMousePosition = WindowManager::GetCursorPosition(true);
    } else {
        state().playerState(index).resetMovement();
        if (window != nullptr) {
            window->grabState(hadWindowGrab);
        }
        _context.app().windowManager().setCursorPosition(lastMousePosition.x, lastMousePosition.y, true);
    }

    WindowManager::ToggleRelativeMouseMode(lockState);

    return true;
}

void Scene::loadDefaultCamera() {
    Camera* baseCamera = Camera::utilityCamera(Camera::UtilityCamera::DEFAULT);
    
    
    // Camera position is overridden in the scene's XML configuration file
    if (!_paramHandler.isParam<bool>(_ID((resourceName() + ".options.cameraStartPositionOverride").c_str()))) {
        return;
    }

    if (_paramHandler.getParam<bool>(_ID((resourceName() + ".options.cameraStartPositionOverride").c_str()))) {
        baseCamera->setEye(vec3<F32>(
            _paramHandler.getParam<F32>(_ID((resourceName() + ".options.cameraStartPosition.x").c_str())),
            _paramHandler.getParam<F32>(_ID((resourceName() + ".options.cameraStartPosition.y").c_str())),
            _paramHandler.getParam<F32>(_ID((resourceName() + ".options.cameraStartPosition.z").c_str()))));
        vec2<F32> camOrientation(_paramHandler.getParam<F32>(_ID((resourceName() + ".options.cameraStartOrientation.xOffsetDegrees").c_str())),
            _paramHandler.getParam<F32>(_ID((resourceName() + ".options.cameraStartOrientation.yOffsetDegrees").c_str())));
        baseCamera->setGlobalRotation(camOrientation.y /*yaw*/, camOrientation.x /*pitch*/);
    } else {
        baseCamera->setEye(vec3<F32>(0, 50, 0));
    }

    baseCamera->setMoveSpeedFactor(_paramHandler.getParam<F32>(_ID((resourceName() + ".options.cameraSpeed.move").c_str()), 1.0f));
    baseCamera->setTurnSpeedFactor(_paramHandler.getParam<F32>(_ID((resourceName() + ".options.cameraSpeed.turn").c_str()), 1.0f));
    baseCamera->setProjection(_context.gfx().renderingData().aspectRatio(),
                              _context.config().runtime.verticalFOV,
                              vec2<F32>(_context.config().runtime.zNear, _context.config().runtime.zFar));

}

bool Scene::loadXML(const stringImpl& name) {
    XML::loadScene(Paths::g_xmlDataLocation + Paths::g_scenesLocation, name, this, _context.config());
    return true;
}

bool Scene::load(const stringImpl& name) {
    setState(ResourceState::RES_LOADING);

    _resourceName = name;

    loadDefaultCamera();

    TaskPool& pool = _context.taskPool(TaskPoolType::HIGH_PRIORITY);
    Task* loadTask = CreateTask(pool, DELEGATE_CBK<void, Task&>());
    while (!_xmlSceneGraph.empty()) {
        XML::SceneNode node = _xmlSceneGraph.top();
        _xmlSceneGraph.pop();

        Start(
        *CreateTask(pool, loadTask, [this, node](Task & parentTask) -> void {
            loadAsset(parentTask, node, &_sceneGraph->getRoot(), false);
        }));
    }
    Wait(Start(*loadTask));

    U32 totalLoadingTasks = _loadingTasks.load();
    Console::d_printfn(Locale::get(_ID("SCENE_LOAD_TASKS")), totalLoadingTasks);

    while (totalLoadingTasks > 0) {
        U32 actualTasks = _loadingTasks.load();
        if (totalLoadingTasks != actualTasks) {
            totalLoadingTasks = actualTasks;
            Console::d_printfn(Locale::get(_ID("SCENE_LOAD_TASKS")), totalLoadingTasks);
        }
        idle();
        std::this_thread::yield();
    }

    // We always add a sky
    auto skies = sceneGraph().getNodesByType(SceneNodeType::TYPE_SKY);
    assert(!skies.empty());
    _currentSky = skies[0];

    if (_sun == nullptr) {
        auto dirLights = _lightPool->getLights(LightType::DIRECTIONAL);
        if (!dirLights.empty()) {
            _sun = &dirLights.front()->getSGN();
        }
    }

    // We always add at least one light
    _sun->get<DirectionalLightComponent>()->castsShadows(true);
    _loadComplete = true;

    return _loadComplete;
}

bool Scene::unload() {
    _aiManager->stop();
    WAIT_FOR_CONDITION(!_aiManager->running());

    U32 totalLoadingTasks = _loadingTasks.load();
    while (totalLoadingTasks > 0) {
        U32 actualTasks = _loadingTasks.load();
        if (totalLoadingTasks != actualTasks) {
            totalLoadingTasks = actualTasks;
            Console::d_printfn(Locale::get(_ID("SCENE_LOAD_TASKS")), totalLoadingTasks);
        }
        std::this_thread::yield();
    }

    clearTasks();
    if (_pxScene != nullptr) {
        /// Destroy physics (:D)
        _pxScene->release();
        MemoryManager::DELETE(_pxScene);
    }

    _context.pfx().setPhysicsScene(nullptr);
    clearObjects();
    _loadComplete = false;
    assert(_scenePlayers.empty());

    return true;
}

void Scene::postLoad() {
    _sceneGraph->postLoad();

    if (_pxScene == nullptr) {
        _pxScene = _context.pfx().NewSceneInterface(*this);
        _pxScene->init();
    }

    // Cook geometry
    if (_paramHandler.getParam<bool>(_ID((resourceName() + ".options.autoCookPhysicsAssets").c_str()), true)) {
        _cookCollisionMeshesScheduled = true;
    }
}

void Scene::postLoadMainThread(const Rect<U16>& targetRenderViewport) {
    assert(Runtime::isMainThread());
    ACKNOWLEDGE_UNUSED(targetRenderViewport);
    setState(ResourceState::RES_LOADED);
}

void Scene::rebuildShaders() {
    bool rebuilt = false;
    for (auto iter : _currentSelection) {
        for (I64 selection : iter.second) {
            SceneGraphNode* node = sceneGraph().findNode(selection);
            if (node != nullptr) {
                node->get<RenderingComponent>()->rebuildMaterial();
                rebuilt = true;
            }
        }
    }

    if (!rebuilt) {
        ShaderProgram::rebuildAllShaders();
    }
}

stringImpl Scene::getPlayerSGNName(PlayerIndex idx) {
    return Util::StringFormat(g_defaultPlayerName, idx + 1);
}

void Scene::currentPlayerPass(PlayerIndex idx) {
    //ToDo: These don't necessarily need to match -Ionut
    renderState().renderPass(idx);
    state().playerPass(idx);

    if (state().playerState().cameraUnderwater()) {
        _context.gfx().postFX().pushFilter(FilterType::FILTER_UNDERWATER);
    } else {
        _context.gfx().postFX().popFilter(FilterType::FILTER_UNDERWATER);
    }
}

void Scene::onSetActive() {
    _context.pfx().setPhysicsScene(_pxScene);
    _aiManager->pauseUpdate(false);

    input().onSetActive();
    _context.sfx().stopMusic();
    _context.sfx().dumpPlaylists();

    for (U32 i = 0; i < to_base(MusicType::COUNT); ++i) {
        const SceneState::MusicPlaylist& playlist = state().music(static_cast<MusicType>(i));
        if (!playlist.empty()) {
            for (const SceneState::MusicPlaylist::value_type& song : playlist) {
                _context.sfx().addMusic(i, song.second);
            }
        }
    }
    _context.sfx().playMusic(0);

    assert(_parent.getActivePlayerCount() == 0);
    addPlayerInternal(false);

    static stringImpl originalTitle = _context.activeWindow().title();
    _context.activeWindow().title(originalTitle + " - " + resourceName());
}

void Scene::onRemoveActive() {
    _aiManager->pauseUpdate(true);

    while(!_scenePlayers.empty()) {
        Attorney::SceneManagerScene::removePlayer(_parent, *this, _scenePlayers.back(), false);
    }

    input().onRemoveActive();
}

void Scene::addPlayerInternal(bool queue) {
    // Limit max player count
    if (_parent.getActivePlayerCount() == Config::MAX_LOCAL_PLAYER_COUNT) {
        return;
    }

    stringImpl playerName = getPlayerSGNName(static_cast<PlayerIndex>(_parent.getActivePlayerCount()));
    
    SceneGraphNode* playerSGN(_sceneGraph->findNode(playerName));
    if (!playerSGN) {
        SceneGraphNode& root = _sceneGraph->getRoot();

        SceneGraphNodeDescriptor playerNodeDescriptor;
        playerNodeDescriptor._serialize = false;
        playerNodeDescriptor._node = std::make_shared<SceneNode>(_resCache, static_cast<size_t>(GUIDWrapper::generateGUID() + _parent.getActivePlayerCount()), playerName);
        playerNodeDescriptor._name = playerName;
        playerNodeDescriptor._usageContext = NodeUsageContext::NODE_DYNAMIC;
        playerNodeDescriptor._componentMask = to_base(ComponentType::UNIT) |
                                              to_base(ComponentType::NAVIGATION) |
                                              to_base(ComponentType::TRANSFORM) |
                                              to_base(ComponentType::BOUNDS) |
                                              to_base(ComponentType::RENDERING) |
                                              to_base(ComponentType::NETWORKING);

        playerSGN = root.addNode(playerNodeDescriptor);
    }

    Attorney::SceneManagerScene::addPlayer(_parent, *this, playerSGN, queue);
    assert(playerSGN->get<UnitComponent>()->getUnit() != nullptr);
}

void Scene::removePlayerInternal(PlayerIndex idx) {
    assert(idx < _scenePlayers.size());
    
    Attorney::SceneManagerScene::removePlayer(_parent, *this, _scenePlayers[getSceneIndexForPlayer(idx)], true);
}

void Scene::onPlayerAdd(const Player_ptr& player) {
    _scenePlayers.push_back(player);
    state().onPlayerAdd(player->index());
    input().onPlayerAdd(player->index());
}

void Scene::onPlayerRemove(const Player_ptr& player) {
    PlayerIndex idx = player->index();

    input().onPlayerRemove(idx);
    state().onPlayerRemove(idx);
    _cameraUpdateMap[idx] = 0u;
    if (_flashLight.size() > idx) {
        SceneGraphNode* flashLight = _flashLight[idx];
        if (flashLight) {
            _sceneGraph->getRoot().removeNode(*flashLight);
            _flashLight[idx] = nullptr;
        }
    }
    _sceneGraph->getRoot().removeNode(*player->getBoundNode());

    _scenePlayers.erase(std::cbegin(_scenePlayers) + getSceneIndexForPlayer(idx));
}

U8 Scene::getSceneIndexForPlayer(PlayerIndex idx) const {
    for (U8 i = 0; i < to_U8(_scenePlayers.size()); ++i) {
        if (_scenePlayers[i]->index() == idx) {
            return i;
        }
    }

    DIVIDE_UNEXPECTED_CALL("Player not found!");
    return 0;
}

const Player_ptr& Scene::getPlayerForIndex(PlayerIndex idx) const {
    return _scenePlayers[getSceneIndexForPlayer(idx)];
}

U8 Scene::getPlayerIndexForDevice(U8 deviceIndex) const {
    return input().getPlayerIndexForDevice(deviceIndex);
}

void Scene::clearObjects() {
    while (!_xmlSceneGraph.empty()) {
        _xmlSceneGraph.pop();
    }

    _flashLight.clear();
    _sceneGraph->unload();
}

bool Scene::mouseMoved(const Input::MouseMoveEvent& arg) {
    if (!arg.wheelEvent()) {
        PlayerIndex idx = getPlayerIndexForDevice(arg._deviceIndex);
        if (!state().playerState(idx).cameraLockedToMouse()) {
            findHoverTarget(idx, arg.absolutePos());
        } else if (Config::Build::ENABLE_EDITOR) {
            Editor& editor = _context.editor();
            if (editor.running() && !editor.scenePreviewHovered()) {
                lockCameraToPlayerMouse(idx, false);
            }
        }
    }
    return false;
}

bool Scene::updateCameraControls(PlayerIndex idx) {
    Camera& cam = getPlayerForIndex(idx)->getCamera();
    
    SceneStatePerPlayer& playerState = state().playerState(idx);

    bool updated = false;
    updated = cam.moveRelative(vec3<I32>(to_I32(playerState.moveFB()),
                                         to_I32(playerState.moveLR()),
                                         to_I32(playerState.moveUD()))) || updated;

    updated = cam.rotateRelative(vec3<I32>(to_I32(playerState.angleUD()), //pitch
                                           to_I32(playerState.angleLR()), //yaw
                                           to_I32(playerState.roll()))) || updated; //roll
    updated = cam.zoom(to_I32(playerState.zoom())) || updated;

    playerState.cameraUpdated(updated);
    playerState.cameraUnderwater(checkCameraUnderwater(idx));

    return playerState.cameraUpdated();
}

void Scene::updateSceneState(const U64 deltaTimeUS) {
    _sceneTimerUS += deltaTimeUS;
    updateSceneStateInternal(deltaTimeUS);
    _sceneGraph->sceneUpdate(deltaTimeUS, *_sceneState);
    _aiManager->update(deltaTimeUS);
}

void Scene::onStartUpdateLoop(const U8 loopNumber) {
    _sceneGraph->onStartUpdateLoop(loopNumber);
}

void Scene::onLostFocus() {
    for (U8 i = 0; i < to_U8(_scenePlayers.size()); ++i) {
        state().playerState(_scenePlayers[i]->index()).resetMovement();
    }

    //_paramHandler.setParam(_ID("freezeLoopTime"), true);
}

void Scene::registerTask(Task& taskItem, bool start, TaskPriority priority) {
    UniqueLockShared w_lock(_tasksMutex);
    _tasks.push_back(&taskItem);
    if (start) {
        Start(taskItem, priority);
    }
}

void Scene::clearTasks() {
    Console::printfn(Locale::get(_ID("STOP_SCENE_TASKS")));
    // Performance shouldn't be an issue here
    UniqueLockShared w_lock(_tasksMutex);
    for (Task* task : _tasks) {
        Wait(Stop(*task));
    }

    _tasks.clear();
}

void Scene::removeTask(Task& task) {
    UniqueLockShared w_lock(_tasksMutex);
    vectorEASTL<Task*>::iterator it;
    for (it = eastl::begin(_tasks); it != eastl::end(_tasks); ++it) {
        if ((*it)->_id == task._id) {
            Wait(Stop(*(*it)));
            _tasks.erase(it);
            return;
        }
    }
}

void Scene::processInput(PlayerIndex idx, const U64 deltaTimeUS) {
}

void Scene::processGUI(const U64 deltaTimeUS) {
    D64 delta = Time::MicrosecondsToMilliseconds<D64>(deltaTimeUS);

    std::transform(std::begin(_guiTimersMS), std::end(_guiTimersMS), std::begin(_guiTimersMS),
                   std::bind1st(std::plus<D64>(), delta));
}

void Scene::processTasks(const U64 deltaTimeUS) {
    D64 delta = Time::MicrosecondsToMilliseconds<D64>(deltaTimeUS);

    std::transform(std::begin(_taskTimers), std::end(_taskTimers), std::begin(_taskTimers),
                   std::bind1st(std::plus<D64>(), delta));
}

void Scene::debugDraw(const Camera& activeCamera, RenderStagePass stagePass, GFX::CommandBuffer& bufferInOut) {
    if (Config::Build::IS_DEBUG_BUILD) {
        const SceneRenderState::GizmoState& currentGizmoState = renderState().gizmoState();

        if (currentGizmoState == SceneRenderState::GizmoState::SELECTED_GIZMO) {
            for (auto iter : _currentSelection) {
                for (I64 selection : iter.second) {
                    SceneGraphNode* node = sceneGraph().findNode(selection);
                    if (node != nullptr) {
                        node->get<RenderingComponent>()->drawDebugAxis();
                    }
                }
            }
        }
        if (renderState().isEnabledOption(SceneRenderState::RenderOptions::RENDER_DEBUG_LINES)) {
            bufferInOut.add(_linesPrimitive->toCommandBuffer());
        }
        if (renderState().isEnabledOption(SceneRenderState::RenderOptions::RENDER_OCTREE_REGIONS)) {
            _octreeBoundingBoxes.resize(0);
            sceneGraph().getOctree().getAllRegions(_octreeBoundingBoxes);

            size_t primitiveCount = _octreePrimitives.size();
            size_t regionCount = _octreeBoundingBoxes.size();
            if (regionCount > primitiveCount) {
                size_t diff = regionCount - primitiveCount;
                for (size_t i = 0; i < diff; ++i) {
                    _octreePrimitives.push_back(_context.gfx().newIMP());
                }
            }

            assert(_octreePrimitives.size() >= _octreeBoundingBoxes.size());

            for (size_t i = 0; i < regionCount; ++i) {
                const BoundingBox& box = _octreeBoundingBoxes[i];
                _octreePrimitives[i]->fromBox(box.getMin(), box.getMax(), UColour(255, 0, 255, 255));
                bufferInOut.add(_octreePrimitives[i]->toCommandBuffer());
            }
        }
    }
    if (Config::Build::IS_DEBUG_BUILD) {
        bufferInOut.add(_linesPrimitive->toCommandBuffer());
    }
    // Show NavMeshes
    _aiManager->debugDraw(bufferInOut, false);
    _lightPool->drawLightImpostors(stagePass._stage, bufferInOut);
    _envProbePool->debugDraw(bufferInOut);
}

bool Scene::checkCameraUnderwater(PlayerIndex idx) const {
    const vectorEASTL<SceneGraphNode*>& waterBodies = _sceneGraph->getNodesByType(SceneNodeType::TYPE_WATER);

    const Camera& crtCamera = getPlayerForIndex(idx)->getCamera();
    const vec3<F32>& eyePos = crtCamera.getEye();

    if (!waterBodies.empty()) {
        for (SceneGraphNode* node : waterBodies) {
            if (node->getNode<WaterPlane>().pointUnderwater(*node, eyePos)) {
                return true;
            }
        }
    }

    for (const WaterDetails& water : state().globalWaterBodies()) {
        if (eyePos.y < water._heightOffset) {
            return true;
        }
    }

    return false;
}

void Scene::findHoverTarget(PlayerIndex idx, const vec2<I32>& aimPosIn) {
    const Camera& crtCamera = getPlayerForIndex(idx)->getCamera();

    vec2<I32> aimPos(aimPosIn);
    const Rect<I32>& viewport = _context.gfx().getCurrentViewport();
    const vec2<U16>& displaySize = _context.activeWindow().getDimensions();
    if (Config::Build::ENABLE_EDITOR) {
        if (_context.editor().running() && _context.editor().scenePreviewFocused()) {
            const Rect<I32>& sceneRect = _context.editor().scenePreviewRect(false);
            if (sceneRect.contains(aimPos)) {
                aimPos = COORD_REMAP(aimPos, sceneRect, viewport);
            }
        }
    }
    const vec2<F32>& zPlanes = crtCamera.getZPlanes();

    F32 aimX = to_F32(aimPos.x);
    F32 aimY = displaySize.height - to_F32(aimPos.y) - 1;

    vec3<F32> startRay = crtCamera.unProject(aimX, aimY, 0.0f, viewport);
    vec3<F32> endRay = crtCamera.unProject(aimX, aimY, 1.0f, viewport);
    // see if we select another one
    _sceneSelectionCandidates.clear();

    // Cast the picking ray and find items between the nearPlane and far Plane
    Ray mouseRay(startRay, startRay.direction(endRay));
    sceneGraph().intersect(mouseRay, zPlanes.x, zPlanes.y, _sceneSelectionCandidates);

    _currentHoverTarget[idx] = -1;
    if (!_sceneSelectionCandidates.empty()) {
        // If we don't force selections, remove all of the nodes that lack a SelectionComponent
        std::sort(std::begin(_sceneSelectionCandidates),
                  std::end(_sceneSelectionCandidates),
                  [](const SGNRayResult& A, const SGNRayResult& B) -> bool {
                      return std::get<1>(A) < std::get<1>(B);
                  });

        SceneGraphNode* target = nullptr;
        for (const SGNRayResult& result : _sceneSelectionCandidates) {
            I64 crtCandidate = std::get<0>(result);
            SceneGraphNode* crtNode = _sceneGraph->findNode(crtCandidate);
            if (crtNode && crtNode->get<SelectionComponent>() && crtNode->get<SelectionComponent>()->enabled()) {
                target = crtNode;
                break;
            }
        }

        // Well ... this happened somehow ...
        if (target == nullptr) {
            return;
        }

        _currentHoverTarget[idx] = target->getGUID();
        if (target->getSelectionFlag() != SceneGraphNode::SelectionFlag::SELECTION_SELECTED) {
            target->setSelectionFlag(SceneGraphNode::SelectionFlag::SELECTION_HOVER);
        }
    } else {
        SceneGraphNode* target = _sceneGraph->findNode(_currentHoverTarget[idx]);
        if (target == nullptr) {
            return;
        }

        if (target->getSelectionFlag() != SceneGraphNode::SelectionFlag::SELECTION_SELECTED) {
            target->setSelectionFlag(SceneGraphNode::SelectionFlag::SELECTION_NONE);
        }
    }
}

void Scene::onNodeDestroy(SceneGraphNode& node) {
    I64 guid = node.getGUID();
    for (auto iter : _currentHoverTarget) {
        if (iter.second == guid) {
            iter.second = -1;
        }
    }

    for (auto iter : _currentSelection) {
        iter.second.erase(
            std::remove_if(std::begin(iter.second), std::end(iter.second),
                           [guid](I64 crtGUID) -> bool { return guid == crtGUID; }),
            std::end(iter.second));
    }
}

void Scene::resetSelection(PlayerIndex idx) {
    for (I64 selectionGUID : _currentSelection[idx]) {
        SceneGraphNode* node = sceneGraph().findNode(selectionGUID);
        if (node != nullptr) {
            node->setSelectionFlag(SceneGraphNode::SelectionFlag::SELECTION_NONE);
        }
    }

    _currentSelection[idx].clear();

    for (DELEGATE_CBK<void, U8, SceneGraphNode*>& cbk : _selectionChangeCallbacks) {
        cbk(idx, nullptr);
    }
}

void Scene::setSelected(PlayerIndex idx, SceneGraphNode& sgn) {
    _currentSelection[idx].push_back(sgn.getGUID());
    sgn.setSelectionFlag(SceneGraphNode::SelectionFlag::SELECTION_SELECTED);
    for (DELEGATE_CBK<void, U8, SceneGraphNode*>& cbk : _selectionChangeCallbacks) {
        cbk(idx, &sgn);
    }
}

void Scene::findSelection(PlayerIndex idx, bool clearOld) {
    // Clear old selection
    if (clearOld) {
        _parent.resetSelection(idx);
    }

    I64 hoverGUID = _currentHoverTarget[idx];
    // No hover target
    if (hoverGUID == -1) {
        return;
    }

    vector<I64>& selections = _currentSelection[idx];
    if (!selections.empty()) {
        if (std::find(std::cbegin(selections), std::cend(selections), hoverGUID) != std::cend(selections)) {
            //Already selected
            return;
        }
    }

    SceneGraphNode* selectedNode = _sceneGraph->findNode(hoverGUID);
    if (selectedNode != nullptr) {
        _parent.setSelected(idx, *selectedNode);
    }
}

bool Scene::save(ByteBuffer& outputBuffer) const {
    U8 playerCount = to_U8(_scenePlayers.size());
    outputBuffer << playerCount;
    for (U8 i = 0; i < playerCount; ++i) {
        const Camera& cam = _scenePlayers[i]->getCamera();
        outputBuffer << _scenePlayers[i]->index() << cam.getEye() << cam.getOrientation();
    }

    return _sceneGraph->saveCache(outputBuffer);
}

bool Scene::load(ByteBuffer& inputBuffer) {

    if (!inputBuffer.empty()) {
        vec3<F32> camPos;
        Quaternion<F32> camOrientation;

        U8 currentPlayerIndex = 0;
        U8 currentPlayerCount = to_U8(_scenePlayers.size());

        U8 previousPlayerCount = 0;
        inputBuffer >> previousPlayerCount;
        for (U8 i = 0; i < previousPlayerCount; ++i) {
            inputBuffer >> currentPlayerIndex >> camPos >> camOrientation;
            if (currentPlayerIndex < currentPlayerCount) {
                Camera& cam = _scenePlayers[currentPlayerIndex]->getCamera();
                cam.setEye(camPos);
                cam.setRotation(camOrientation);
            }
        }
    }

    return _sceneGraph->loadCache(inputBuffer);
}

Camera* Scene::playerCamera() const {
    return Attorney::SceneManagerCameraAccessor::playerCamera(_parent);
}

Camera* Scene::playerCamera(U8 index) const {
    return Attorney::SceneManagerCameraAccessor::playerCamera(_parent, index);
}

};