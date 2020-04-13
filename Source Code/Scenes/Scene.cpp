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

constexpr U32 g_NodesPerLoadPartition = 10u;
constexpr const char* const g_defaultPlayerName = "Player_%d";
};

Scene::Scene(PlatformContext& context, ResourceCache* cache, SceneManager& parent, const Str128& name)
    : Resource(ResourceType::DEFAULT, name),
      PlatformContextComponent(context),
      _parent(parent),
      _resCache(cache),
      _currentSky(nullptr),
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

    RenderStateBlock primitiveDescriptor;
    _linesPrimitive = _context.gfx().newIMP();
    _linesPrimitive->name("GenericLinePrimitive");

    PipelineDescriptor pipeDesc;
    pipeDesc._stateHash = primitiveDescriptor.getHash();
    pipeDesc._shaderProgramHandle = ShaderProgram::defaultShader()->getGUID();
    _linesPrimitive->pipeline(*_context.gfx().newPipeline(pipeDesc));
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
        _context.gfx().destroyIMP(prim);
    }
    if (_linesPrimitive) {
        _context.gfx().destroyIMP(_linesPrimitive);
    }
}

bool Scene::onStartup() {
    return true;
}

bool Scene::onShutdown() {
    return true;
}

bool Scene::frameStarted() {
    //UniqueLock<Mutex> lk(_perFrameArenaMutex);
    //_perFrameArena.clear();
    return true;
}

bool Scene::frameEnded() {
    return true;
}

bool Scene::idle() {  // Called when application is idle
    _sceneGraph->idle();

    Attorney::SceneRenderStateScene::playAnimations(renderState(), _context.config().debug.mesh.playAnimations);

    if (_cookCollisionMeshesScheduled && checkLoadFlag()) {
        if (_context.gfx().getFrameCount() > 1) {
            _sceneGraph->getRoot().get<RigidBodyComponent>()->cookCollisionMesh(resourceName().c_str());
            _cookCollisionMeshesScheduled = false;
        }
    }

    _lightPool->idle();

    {
        SharedLock<SharedMutex> r_lock(_tasksMutex);
        if (_tasks.empty()) {
            return true;
        }
    }

    UniqueLock<SharedMutex> r_lock(_tasksMutex);
    _tasks.erase(std::remove_if(eastl::begin(_tasks),
                                eastl::end(_tasks),
                                [](Task* handle) -> bool { 
                                    return handle != nullptr && Finished(*handle);
                                }),
                eastl::end(_tasks));

    return true;
}

void Scene::addMusic(MusicType type, const Str64& name, const Str256& srcFile) {

    FileWithPath fileResult = splitPathToNameAndLocation(srcFile.c_str());
    const stringImpl& musicFile = fileResult._fileName;
    const stringImpl& musicFilePath = fileResult._path;

    ResourceDescriptor music(name);
    music.assetName(musicFile);
    music.assetLocation(musicFilePath);
    music.flag(true);
    hashAlg::insert(state().music(type),
                    _ID(name.c_str()),
                    CreateResource<AudioDescriptor>(_resCache, music));
}


bool Scene::saveXML() const {
    using boost::property_tree::ptree;

    Console::printfn(Locale::get(_ID("XML_SAVE_SCENE_START")), resourceName().c_str());

    const Str256& scenePath = Paths::g_xmlDataLocation + Paths::g_scenesLocation;
    const boost::property_tree::xml_writer_settings<std::string> settings(' ', 4);

    const Str256 sceneLocation(scenePath + "/" + resourceName().c_str());
    const Str256 sceneDataFile(sceneLocation + ".xml");

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
        pt.put("options.cameraSpeed.<xmlattr>.move", par.getParam<F32>(_ID_32((resourceName() + ".options.cameraSpeed.move").c_str())));
        pt.put("options.cameraSpeed.<xmlattr>.turn", par.getParam<F32>(_ID_32((resourceName() + ".options.cameraSpeed.turn").c_str())));
        pt.put("options.autoCookPhysicsAssets", true);

        pt.put("fog.fogDensity", state().renderState().fogDescriptor().density());
        pt.put("fog.fogColour.<xmlattr>.r", state().renderState().fogDescriptor().colour().r);
        pt.put("fog.fogColour.<xmlattr>.g", state().renderState().fogDescriptor().colour().g);
        pt.put("fog.fogColour.<xmlattr>.b", state().renderState().fogDescriptor().colour().b);

        pt.put("lod.lodThresholds.<xmlattr>.x", state().renderState().lodThresholds().x);
        pt.put("lod.lodThresholds.<xmlattr>.y", state().renderState().lodThresholds().y);
        pt.put("lod.lodThresholds.<xmlattr>.z", state().renderState().lodThresholds().z);
        pt.put("lod.lodThresholds.<xmlattr>.w", state().renderState().lodThresholds().w);

        pt.put("shadowing.<xmlattr>.lightBleedBias", state().lightBleedBias());
        pt.put("shadowing.<xmlattr>.minShadowVariance", state().minShadowVariance());
        pt.put("shadowing.<xmlattr>.shadowFadeDistance", state().shadowFadeDistance());
        pt.put("shadowing.<xmlattr>.shadowDistance", state().shadowDistance());

        copyFile(scenePath.c_str(), (resourceName() + ".xml").c_str(), scenePath.c_str(), (resourceName() + ".xml.bak").c_str(), true);
        write_xml(sceneDataFile.c_str(), pt, std::locale(), settings);
    }
    sceneGraph().saveToXML();

    //save music
    {
        ptree pt;
        copyFile((sceneLocation + "/").c_str(), "musicPlaylist.xml", (sceneLocation + "/").c_str(), "musicPlaylist.xml.bak", true);
        write_xml((sceneLocation + "/" + "musicPlaylist.xml.dev").c_str(), pt, std::locale(), settings);
    }

    Console::printfn(Locale::get(_ID("XML_SAVE_SCENE_END")), resourceName().c_str());

    return true;
}

bool Scene::loadXML(const Str128& name) {
    const Str256& scenePath = Paths::g_xmlDataLocation + Paths::g_scenesLocation;
    Configuration& config = _context.config();

    ParamHandler& par = ParamHandler::instance();

    boost::property_tree::ptree pt;

    Console::printfn(Locale::get(_ID("XML_LOAD_SCENE")), name.c_str());
    const Str256 sceneLocation(scenePath + "/" + name.c_str());
    const Str256 sceneDataFile(sceneLocation + ".xml");

    // A scene does not necessarily need external data files
    // Data can be added in code for simple scenes
    if (!fileExists(sceneDataFile.c_str())) {
        XML::loadSceneGraph(sceneLocation, "assets.xml", this);
        XML::loadMusicPlaylist(sceneLocation, "musicPlaylist.xml", this, config);
        return true;
    }

    try {
        read_xml(sceneDataFile.c_str(), pt);
    } catch (const boost::property_tree::xml_parser_error& e) {
        Console::errorfn(Locale::get(_ID("ERROR_XML_INVALID_SCENE")), name.c_str());
        stringImpl error(e.what());
        error += " [check error log!]";
        throw error.c_str();
    }

    state().renderState().grassVisibility(pt.get("vegetation.grassVisibility", 1000.0f));
    state().renderState().treeVisibility(pt.get("vegetation.treeVisibility", 1000.0f));
    state().renderState().generalVisibility(pt.get("options.visibility", 1000.0f));

    state().windDirX(pt.get("wind.windDirX", 1.0f));
    state().windDirZ(pt.get("wind.windDirZ", 1.0f));
    state().windSpeed(pt.get("wind.windSpeed", 1.0f));

    state().lightBleedBias(pt.get("shadowing.<xmlattr>.lightBleedBias", 0.2f));
    state().minShadowVariance(pt.get("shadowing.<xmlattr>.minShadowVariance", 0.001f));
    state().shadowFadeDistance(pt.get("shadowing.<xmlattr>.shadowFadeDistance", to_U16(900u)));
    state().shadowDistance(pt.get("shadowing.<xmlattr>.shadowDistance", to_U16(1000u)));

    if (boost::optional<boost::property_tree::ptree&> waterOverride = pt.get_child_optional("water")) {
        WaterDetails waterDetails = {};
        waterDetails._heightOffset = pt.get("water.waterLevel", 0.0f);
        waterDetails._depth = pt.get("water.waterDepth", -75.0f);
        state().globalWaterBodies().push_back(waterDetails);
    }

    if (boost::optional<boost::property_tree::ptree&> cameraPositionOverride = pt.get_child_optional("options.cameraStartPosition")) {
        par.setParam(_ID_32((name + ".options.cameraStartPosition.x").c_str()), pt.get("options.cameraStartPosition.<xmlattr>.x", 0.0f));
        par.setParam(_ID_32((name + ".options.cameraStartPosition.y").c_str()), pt.get("options.cameraStartPosition.<xmlattr>.y", 0.0f));
        par.setParam(_ID_32((name + ".options.cameraStartPosition.z").c_str()), pt.get("options.cameraStartPosition.<xmlattr>.z", 0.0f));
        par.setParam(_ID_32((name + ".options.cameraStartOrientation.xOffsetDegrees").c_str()), pt.get("options.cameraStartPosition.<xmlattr>.xOffsetDegrees", 0.0f));
        par.setParam(_ID_32((name + ".options.cameraStartOrientation.yOffsetDegrees").c_str()), pt.get("options.cameraStartPosition.<xmlattr>.yOffsetDegrees", 0.0f));
        par.setParam(_ID_32((name + ".options.cameraStartPositionOverride").c_str()), true);
    } else {
        par.setParam(_ID_32((name + ".options.cameraStartPositionOverride").c_str()), false);
    }

    if (boost::optional<boost::property_tree::ptree&> physicsCook = pt.get_child_optional("options.autoCookPhysicsAssets")) {
        par.setParam(_ID_32((name + ".options.autoCookPhysicsAssets").c_str()), pt.get<bool>("options.autoCookPhysicsAssets", false));
    } else {
        par.setParam(_ID_32((name + ".options.autoCookPhysicsAssets").c_str()), false);
    }

    if (boost::optional<boost::property_tree::ptree&> cameraPositionOverride = pt.get_child_optional("options.cameraSpeed")) {
        par.setParam(_ID_32((name + ".options.cameraSpeed.move").c_str()), pt.get("options.cameraSpeed.<xmlattr>.move", 35.0f));
        par.setParam(_ID_32((name + ".options.cameraSpeed.turn").c_str()), pt.get("options.cameraSpeed.<xmlattr>.turn", 35.0f));
    } else {
        par.setParam(_ID_32((name + ".options.cameraSpeed.move").c_str()), 35.0f);
        par.setParam(_ID_32((name + ".options.cameraSpeed.turn").c_str()), 35.0f);
    }

    vec3<F32> fogColour(config.rendering.fogColour);
    F32 fogDensity = config.rendering.fogDensity;

    if (boost::optional<boost::property_tree::ptree&> fog = pt.get_child_optional("fog")) {
        fogDensity = pt.get("fog.fogDensity", fogDensity);
        fogColour.set(pt.get<F32>("fog.fogColour.<xmlattr>.r", fogColour.r),
                      pt.get<F32>("fog.fogColour.<xmlattr>.g", fogColour.g),
                      pt.get<F32>("fog.fogColour.<xmlattr>.b", fogColour.b));
    }
    state().renderState().fogDescriptor().set(fogColour, fogDensity);

    vec4<U16> lodThresholds(config.rendering.lodThresholds);

    if (boost::optional<boost::property_tree::ptree&> fog = pt.get_child_optional("lod")) {
        lodThresholds.set(pt.get<U16>("lod.lodThresholds.<xmlattr>.x", lodThresholds.x),
                          pt.get<U16>("lod.lodThresholds.<xmlattr>.y", lodThresholds.y),
                          pt.get<U16>("lod.lodThresholds.<xmlattr>.z", lodThresholds.z),
                          pt.get<U16>("lod.lodThresholds.<xmlattr>.w", lodThresholds.w));
    }
    state().renderState().lodThresholds().set(lodThresholds);

    XML::loadSceneGraph(sceneLocation, pt.get("assets", ""), this);
    XML::loadMusicPlaylist(sceneLocation, pt.get("musicPlaylist", ""), this, config);

    return true;
}

namespace {
    bool IsPrimitive(const char* modelName) {
        constexpr std::array<const char*, 3> pritimiveNames = {
            "BOX_3D",
            "QUAD_3D",
            "SPHERE_3D"
        };

        for (const char* it : pritimiveNames) {
            if (Util::CompareIgnoreCase(modelName, it)) {
                return true;
            }
        }

        return false;
    }
};


void Scene::loadAsset(Task* parentTask, const XML::SceneNode& sceneNode, SceneGraphNode* parent, bool waitForReady) {
    assert(parent != nullptr);

    auto waitForReasoureTask = [&parentTask](const CachedResource_wptr& res) {
        ACKNOWLEDGE_UNUSED(res);
        TaskYield(*parentTask);
    };

    const Str256& scenePath = Paths::g_xmlDataLocation + Paths::g_scenesLocation;
    Str256 sceneLocation(scenePath + "/" + resourceName().c_str());
    stringImpl nodePath = sceneLocation + "/nodes/" + parent->name() + "_" + sceneNode.name + ".xml";

    SceneGraphNode* crtNode = parent;
    if (fileExists(nodePath.c_str())) {

        U32 normalMask = to_base(ComponentType::TRANSFORM) |
                         to_base(ComponentType::BOUNDS) |
                         to_base(ComponentType::NETWORKING);


        boost::property_tree::ptree nodeTree = {};
        read_xml(nodePath, nodeTree);

        const auto loadModelComplete = [this, &nodeTree/*, &parentTask*/](CachedResource_wptr res) {
            while (res.lock()->getState() != ResourceState::RES_LOADED) {
                //TaskYield(*parentTask);
            }

            eastl::static_pointer_cast<SceneNode>(res.lock())->loadFromXML(nodeTree);
            _loadingTasks.fetch_sub(1);
        };

        stringImpl modelName = nodeTree.get("model", "");

        SceneNode_ptr ret = nullptr;

        bool skipAdd = true;
        if (sceneNode.type == "ROOT") {
            // Nothing to do with the root. It's good that it exists
        } else if (IsPrimitive(sceneNode.type.c_str())) {// Primitive types (only top level)
            normalMask |= to_base(ComponentType::RENDERING);

            if (!modelName.empty()) {
                _loadingTasks.fetch_add(1);
                ResourceDescriptor item(sceneNode.name);
                item.onLoadCallback(loadModelComplete);
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
                    item.mask(quadMask);
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
                tempMaterial->setShadingMode(ShadingMode::BLINN_PHONG);
                ret->setMaterialTpl(tempMaterial);
            }
            skipAdd = false;
        }
        // Terrain types
        else if (sceneNode.type == "TERRAIN") {
            _loadingTasks.fetch_add(1);
            normalMask |= to_base(ComponentType::RENDERING);
            addTerrain(*parent, nodeTree, sceneNode.name);
        }
        else if (sceneNode.type == "VEGETATION_GRASS") {
            normalMask |= to_base(ComponentType::RENDERING);
            NOP(); //we rebuild grass everytime
        }
        else if (sceneNode.type == "INFINITE_PLANE") {
            _loadingTasks.fetch_add(1);
            normalMask |= to_base(ComponentType::RENDERING);
            addInfPlane(*parent, nodeTree, sceneNode.name);
        } 
        else if (sceneNode.type == "WATER") {
            _loadingTasks.fetch_add(1);
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
                model.flag(true);
                model.threaded(false);
                model.waitForReady(waitForReady);
                model.waitForReadyCbk(waitForReasoureTask);
                model.onLoadCallback(loadModelComplete);
                ret = CreateResource<Mesh>(_resCache, model);
            }

            skipAdd = false;
        }
        // Submesh (change component properties, as the meshes should already be loaded)
        else if (Util::CompareIgnoreCase(sceneNode.type, "SUBMESH")) {
            while (parent->getNode().getState() != ResourceState::RES_LOADED) {
                TaskYield(*parentTask);
            }

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

            crtNode = parent->addChildNode(nodeDescriptor);
            crtNode->loadFromXML(nodeTree);
        }
    }

    ParallelForDescriptor descriptor = {};
    descriptor._iterCount = to_U32(sceneNode.children.size());
    descriptor._partitionSize = g_NodesPerLoadPartition;
    descriptor._priority = TaskPriority::DONT_CARE;
    descriptor._useCurrentThread = true;

    parallel_for(_context,
        [this, &sceneNode, &crtNode, &waitForReady](Task* parentTask, U32 start, U32 end) {
            for (U32 i = start; i < end; ++i) {
                loadAsset(parentTask, sceneNode.children[i], crtNode, waitForReady);
            }
        },
        descriptor);
}

SceneGraphNode* Scene::addParticleEmitter(const Str64& name,
                                          std::shared_ptr<ParticleData> data,
                                          SceneGraphNode& parentNode) {
    DIVIDE_ASSERT(!name.empty(),
                  "Scene::addParticleEmitter error: invalid name specified!");

    ResourceDescriptor particleEmitter(name);
    eastl::shared_ptr<ParticleEmitter> emitter = CreateResource<ParticleEmitter>(_resCache, particleEmitter);

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

    return parentNode.addChildNode(particleNodeDescriptor);
}

void Scene::addTerrain(SceneGraphNode& parentNode, boost::property_tree::ptree pt, const Str64& name) {
    Console::printfn(Locale::get(_ID("XML_LOAD_TERRAIN")), name.c_str());

    // Load the rest of the terrain
    std::shared_ptr<TerrainDescriptor> ter(new TerrainDescriptor((name + "_descriptor").c_str()));
    if (!ter->loadFromXML(pt, name.c_str())) {
        return;
    }

    auto registerTerrain = [this, name, &parentNode, pt](CachedResource_wptr res) {
        SceneGraphNodeDescriptor terrainNodeDescriptor;
        terrainNodeDescriptor._name = name;
        terrainNodeDescriptor._node = eastl::static_pointer_cast<Terrain>(res.lock());
        terrainNodeDescriptor._usageContext = NodeUsageContext::NODE_STATIC;
        terrainNodeDescriptor._componentMask = to_base(ComponentType::NAVIGATION) |
                                               to_base(ComponentType::TRANSFORM) |
                                               to_base(ComponentType::RIGID_BODY) |
                                               to_base(ComponentType::BOUNDS) |
                                               to_base(ComponentType::RENDERING) |
                                               to_base(ComponentType::NETWORKING);
        terrainNodeDescriptor._node->addStateCallback(ResourceState::RES_LOADED,
            [this](Resource_wptr res) {
                ACKNOWLEDGE_UNUSED(res);
                _loadingTasks.fetch_sub(1);
            }
        );
        terrainNodeDescriptor._node->loadFromXML(pt);

        SceneGraphNode* terrainTemp = parentNode.addChildNode(terrainNodeDescriptor);


        NavigationComponent* nComp = terrainTemp->get<NavigationComponent>();
        nComp->navigationContext(NavigationComponent::NavigationContext::NODE_OBSTACLE);

        terrainTemp->get<RigidBodyComponent>()->physicsGroup(PhysicsGroup::GROUP_STATIC);
        terrainTemp->loadFromXML(pt);
    };

    ResourceDescriptor descriptor(ter->getVariable("terrainName"));
    descriptor.propertyDescriptor(*ter);
    descriptor.threaded(true);
    descriptor.onLoadCallback(registerTerrain);
    descriptor.flag(ter->active());
    descriptor.waitForReady(false);
    auto ret = CreateResource<Terrain>(_resCache, descriptor);
    ACKNOWLEDGE_UNUSED(ret);
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
        flashLight = _sceneGraph->getRoot().addChildNode(lightNodeDescriptor);
        SpotLightComponent* spotLight = flashLight->get<SpotLightComponent>();
        spotLight->castsShadows(true);
        spotLight->setDiffuseColour(DefaultColours::WHITE.rgb());

        hashAlg::insert(_flashLight, idx, flashLight);

        _cameraUpdateListeners[idx] = playerCamera(idx)->addUpdateListener([this, idx](const Camera& cam) {
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

SceneGraphNode* Scene::addSky(SceneGraphNode& parentNode, boost::property_tree::ptree pt, const Str64& nodeName) {
    ResourceDescriptor skyDescriptor("DefaultSky_"+ nodeName);
    skyDescriptor.ID(to_U32(std::floor(Camera::utilityCamera(Camera::UtilityCamera::DEFAULT)->getZPlanes().y * 2)));

    eastl::shared_ptr<Sky> skyItem = CreateResource<Sky>(_resCache, skyDescriptor);
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

    SceneGraphNode* skyNode = parentNode.addChildNode(skyNodeDescriptor);
    skyNode->setFlag(SceneGraphNode::Flags::VISIBILITY_LOCKED);
    skyNode->loadFromXML(pt);

    return skyNode;
}

void Scene::addWater(SceneGraphNode& parentNode, boost::property_tree::ptree pt, const Str64& nodeName) {
    auto registerWater = [this, nodeName, &parentNode, pt](CachedResource_wptr res) {
        SceneGraphNodeDescriptor waterNodeDescriptor;
        waterNodeDescriptor._name = nodeName;
        waterNodeDescriptor._node = eastl::static_pointer_cast<WaterPlane>(res.lock());
        waterNodeDescriptor._usageContext = NodeUsageContext::NODE_STATIC;
        waterNodeDescriptor._componentMask = to_base(ComponentType::NAVIGATION) |
                                            to_base(ComponentType::TRANSFORM) |
                                            to_base(ComponentType::RIGID_BODY) |
                                            to_base(ComponentType::BOUNDS) |
                                            to_base(ComponentType::RENDERING) |
                                            to_base(ComponentType::NETWORKING);

        waterNodeDescriptor._node->loadFromXML(pt);

        SceneGraphNode* waterNode = parentNode.addChildNode(waterNodeDescriptor);


        NavigationComponent* nComp = waterNode->get<NavigationComponent>();
        nComp->navigationContext(NavigationComponent::NavigationContext::NODE_OBSTACLE);

        waterNode->get<RigidBodyComponent>()->physicsGroup(PhysicsGroup::GROUP_STATIC);
        waterNode->loadFromXML(pt);
        _loadingTasks.fetch_sub(1);
    };

    ResourceDescriptor waterDescriptor("Water_" + nodeName);
    waterDescriptor.threaded(true);
    waterDescriptor.onLoadCallback(registerWater);
    waterDescriptor.waitForReady(false);
    auto ret = CreateResource<WaterPlane>(_resCache, waterDescriptor);
    ACKNOWLEDGE_UNUSED(ret);
}

SceneGraphNode* Scene::addInfPlane(SceneGraphNode& parentNode, boost::property_tree::ptree pt, const Str64& nodeName) {
    auto registerPlane = [this](CachedResource_wptr res) {
        ACKNOWLEDGE_UNUSED(res);
        _loadingTasks.fetch_sub(1);
    };

    ResourceDescriptor planeDescriptor("InfPlane_" + nodeName);

    Camera* baseCamera = Camera::utilityCamera(Camera::UtilityCamera::DEFAULT);

    planeDescriptor.ID(to_U32(baseCamera->getZPlanes().max));
    planeDescriptor.onLoadCallback(registerPlane);

    eastl::shared_ptr<InfinitePlane> planeItem = CreateResource<InfinitePlane>(_resCache, planeDescriptor);
    DIVIDE_ASSERT(planeItem != nullptr, "Scene::addInfPlane error: Could not create infinite plane resource!");
    planeItem->loadFromXML(pt);

    SceneGraphNodeDescriptor planeNodeDescriptor;
    planeNodeDescriptor._node = planeItem;
    planeNodeDescriptor._name = nodeName;
    planeNodeDescriptor._usageContext = NodeUsageContext::NODE_STATIC;
    planeNodeDescriptor._componentMask = to_base(ComponentType::TRANSFORM) |
                                         to_base(ComponentType::BOUNDS) |
                                         to_base(ComponentType::RENDERING);

    SceneGraphNode* ret = parentNode.addChildNode(planeNodeDescriptor);
    ret->loadFromXML(pt);
    return ret;
}

U16 Scene::registerInputActions() {
    _input->flushCache();

    auto none = [](InputParams param) {};
    auto deleteSelection = [this](InputParams param) { 
        PlayerIndex idx = getPlayerIndexForDevice(param._deviceIndex);
        Selections& playerSelections = _currentSelection[idx];
        for (U8 i = 0u; i < playerSelections._selectionCount; ++i) {
            _sceneGraph->removeNode(playerSelections._selections[i]);
        }
        playerSelections._selectionCount = 0u;
        playerSelections._selections.fill(-1);
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
        par.setParam(_ID_32("freezeLoopTime"), !par.getParam(_ID_32("freezeLoopTime"), false));
    };
    auto toggleDepthOfField = [this](InputParams param) {
        PostFX& postFX = _context.gfx().getRenderer().postFX();
        if (postFX.getFilterState(FilterType::FILTER_DEPTH_OF_FIELD)) {
            postFX.popFilter(FilterType::FILTER_DEPTH_OF_FIELD);
        } else {
            postFX.pushFilter(FilterType::FILTER_DEPTH_OF_FIELD);
        }
    };
    auto toggleBloom = [this](InputParams param) {
        PostFX& postFX = _context.gfx().getRenderer().postFX();
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
    auto toggleBoundingBoxRendering = [this](InputParams param) {
        bool showAABB = renderState().isEnabledOption(SceneRenderState::RenderOptions::RENDER_AABB);
        bool showBS = renderState().isEnabledOption(SceneRenderState::RenderOptions::RENDER_BSPHERES);
        if (!showAABB && !showBS) {
            renderState().enableOption(SceneRenderState::RenderOptions::RENDER_AABB);
        } else if (showAABB && !showBS) {
            renderState().enableOption(SceneRenderState::RenderOptions::RENDER_BSPHERES);
        } else if (showAABB && showBS) {
            renderState().disableOption(SceneRenderState::RenderOptions::RENDER_AABB);
        } else /*if (!showAABB && showBS) */ {
            renderState().disableOption(SceneRenderState::RenderOptions::RENDER_BSPHERES);
        }
    };
    auto placeholder = [this](InputParams param) {
        
    };
    auto takeScreenshot = [this](InputParams param) { _context.gfx().Screenshot("screenshot_"); };
    auto toggleFullScreen = [this](InputParams param) { _context.gfx().toggleFullScreen(); };
    auto toggleFlashLight = [this](InputParams param) { toggleFlashlight(getPlayerIndexForDevice(param._deviceIndex)); };
    auto toggleOctreeRegionRendering = [this](InputParams param) {renderState().toggleOption(SceneRenderState::RenderOptions::RENDER_OCTREE_REGIONS);};
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
        if_constexpr(Config::Build::ENABLE_EDITOR) {
            _context.editor().toggle(!_context.editor().running());
        }
    };

    auto toggleConsole = [this](InputParams param) {
        if_constexpr(Config::Build::ENABLE_EDITOR) {
            _context.gui().getConsole().setVisible(!_context.gui().getConsole().isVisible());
        }
    };

    auto dragSelectBegin = [this](InputParams param) {
        beginDragSelection(getPlayerIndexForDevice(param._deviceIndex), _context.editor().running(), vec2<I32>(param._var[2], param._var[3]));
    };

    auto dragSelectEnd = [this](InputParams param) {
        endDragSelection(getPlayerIndexForDevice(param._deviceIndex), _context.editor().running(), vec2<I32>(param._var[2], param._var[3]));
    };

    InputActionList& actions = _input->actionList();
    actions.registerInputAction(0,  none);
    actions.registerInputAction(1,  deleteSelection);
    actions.registerInputAction(2,  increaseCameraSpeed);
    actions.registerInputAction(3,  decreaseCameraSpeed);
    actions.registerInputAction(4,  increaseResolution);
    actions.registerInputAction(5,  decreaseResolution);
    actions.registerInputAction(6,  moveForward);
    actions.registerInputAction(7,  moveBackwards);
    actions.registerInputAction(8,  stopMoveFWDBCK);
    actions.registerInputAction(9,  strafeLeft);
    actions.registerInputAction(10, strafeRight);
    actions.registerInputAction(11, stopStrafeLeftRight);
    actions.registerInputAction(12, rollCCW);
    actions.registerInputAction(13, rollCW);
    actions.registerInputAction(14, stopRollCCWCW);
    actions.registerInputAction(15, turnLeft);
    actions.registerInputAction(16, turnRight);
    actions.registerInputAction(17, stopTurnLeftRight);
    actions.registerInputAction(18, turnUp);
    actions.registerInputAction(19, turnDown);
    actions.registerInputAction(20, stopTurnUpDown);
    actions.registerInputAction(21, togglePauseState);
    actions.registerInputAction(22, toggleDepthOfField);
    actions.registerInputAction(23, toggleBloom);
    actions.registerInputAction(24, toggleSkeletonRendering);
    actions.registerInputAction(25, toggleAxisLineRendering);
    actions.registerInputAction(26, toggleWireframeRendering);
    actions.registerInputAction(27, toggleGeometryRendering);
    actions.registerInputAction(28, toggleDebugLines);
    actions.registerInputAction(29, toggleBoundingBoxRendering);
    actions.registerInputAction(30, placeholder);
    actions.registerInputAction(31, takeScreenshot);
    actions.registerInputAction(32, toggleFullScreen);
    actions.registerInputAction(33, toggleFlashLight);
    actions.registerInputAction(34, toggleOctreeRegionRendering);
    actions.registerInputAction(35, lockCameraToMouse);
    actions.registerInputAction(36, releaseCameraFromMouse);
    actions.registerInputAction(37, rendererDebugView);
    actions.registerInputAction(38, shutdown);
    actions.registerInputAction(39, povNavigation);
    actions.registerInputAction(40, axisNavigation);
    actions.registerInputAction(41, toggleDebugInterface);
    actions.registerInputAction(42, toggleEditor);
    actions.registerInputAction(43, toggleConsole);
    actions.registerInputAction(44, dragSelectBegin);
    actions.registerInputAction(45, dragSelectEnd);

    return 46;
}

void Scene::loadKeyBindings() {
    XML::loadDefaultKeyBindings(Paths::g_xmlDataLocation + "keyBindings.xml", this);
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
        _context.app().windowManager().setCursorPosition(lastMousePosition.x, lastMousePosition.y);
    }

    WindowManager::ToggleRelativeMouseMode(lockState);

    return true;
}

void Scene::loadDefaultCamera() {
    Camera* baseCamera = Camera::utilityCamera(Camera::UtilityCamera::DEFAULT);
    
    
    // Camera position is overridden in the scene's XML configuration file
    if (!_paramHandler.isParam<bool>(_ID_32((resourceName() + ".options.cameraStartPositionOverride").c_str()))) {
        return;
    }

    if (_paramHandler.getParam<bool>(_ID_32((resourceName() + ".options.cameraStartPositionOverride").c_str()))) {
        baseCamera->setEye(vec3<F32>(
            _paramHandler.getParam<F32>(_ID_32((resourceName() + ".options.cameraStartPosition.x").c_str())),
            _paramHandler.getParam<F32>(_ID_32((resourceName() + ".options.cameraStartPosition.y").c_str())),
            _paramHandler.getParam<F32>(_ID_32((resourceName() + ".options.cameraStartPosition.z").c_str()))));
        vec2<F32> camOrientation(_paramHandler.getParam<F32>(_ID_32((resourceName() + ".options.cameraStartOrientation.xOffsetDegrees").c_str())),
            _paramHandler.getParam<F32>(_ID_32((resourceName() + ".options.cameraStartOrientation.yOffsetDegrees").c_str())));
        baseCamera->setGlobalRotation(camOrientation.y /*yaw*/, camOrientation.x /*pitch*/);
    } else {
        baseCamera->setEye(vec3<F32>(0, 50, 0));
    }

    baseCamera->setMoveSpeedFactor(_paramHandler.getParam<F32>(_ID_32((resourceName() + ".options.cameraSpeed.move").c_str()), 1.0f));
    baseCamera->setTurnSpeedFactor(_paramHandler.getParam<F32>(_ID_32((resourceName() + ".options.cameraSpeed.turn").c_str()), 1.0f));
    baseCamera->setProjection(_context.gfx().renderingData().aspectRatio(),
                              _context.config().runtime.verticalFOV,
                              vec2<F32>(_context.config().runtime.zNear, _context.config().runtime.zFar));

}

bool Scene::load(const Str128& name) {
    setState(ResourceState::RES_LOADING);
    std::atomic_init(&_loadingTasks, 0u);

    _resourceName = name;

    loadDefaultCamera();

    SceneGraphNode& rootNode = _sceneGraph->getRoot();

    ParallelForDescriptor descriptor = {};
    descriptor._iterCount = to_U32(_xmlSceneGraph.size());
    descriptor._partitionSize = g_NodesPerLoadPartition;
    descriptor._priority = TaskPriority::DONT_CARE;
    descriptor._useCurrentThread = true;

    parallel_for(_context,
        [this, &rootNode](Task* parentTask, U32 start, U32 end) {
            for (U32 i = start; i < end; ++i) {
                loadAsset(parentTask, _xmlSceneGraph[i], &rootNode, false);
            }
        },
        descriptor);

    WAIT_FOR_CONDITION(_loadingTasks.load() == 0u);

    // We always add a sky
    const auto& skies = sceneGraph().getNodesByType(SceneNodeType::TYPE_SKY);
    assert(!skies.empty());
    _currentSky = skies[0];

    if (_sun == nullptr) {
        auto dirLights = _lightPool->getLights(LightType::DIRECTIONAL);
        if (!dirLights.empty()) {
            _sun = &dirLights.front()->getSGN();
        }
    }
    if (_sun != nullptr) {
        // We always add at least one light
        _sun->get<DirectionalLightComponent>()->castsShadows(true);
    }
    _loadComplete = true;

    return _loadComplete;
}

bool Scene::unload() {
    _aiManager->stop();
    WAIT_FOR_CONDITION(!_aiManager->running());

    U32 totalLoadingTasks = _loadingTasks.load();
    while (totalLoadingTasks > 0) {
        const U32 actualTasks = _loadingTasks.load();
        if (totalLoadingTasks != actualTasks) {
            totalLoadingTasks = actualTasks;
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
    if (_paramHandler.getParam<bool>(_ID_32((resourceName() + ".options.autoCookPhysicsAssets").c_str()), true)) {
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
    for (auto& [playerIdx, selections] : _currentSelection) {
        for (U8 i = 0u; i < selections._selectionCount; ++i) {
            SceneGraphNode* node = sceneGraph().findNode(selections._selections[i]);
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
        _context.gfx().getRenderer().postFX().pushFilter(FilterType::FILTER_UNDERWATER);
    } else {
        _context.gfx().getRenderer().postFX().popFilter(FilterType::FILTER_UNDERWATER);
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
    _context.activeWindow().title("%s - %s", originalTitle.c_str(), resourceName().c_str());
}

void Scene::onRemoveActive() {
    _aiManager->pauseUpdate(true);

    while(!_scenePlayers.empty()) {
        Attorney::SceneManagerScene::removePlayer(_parent, *this, _scenePlayers.back()->getBoundNode(), false);
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
        playerNodeDescriptor._node = eastl::make_shared<SceneNode>(_resCache, to_size(GUIDWrapper::generateGUID() + _parent.getActivePlayerCount()), playerName, playerName, "", SceneNodeType::TYPE_EMPTY, 0u);
        playerNodeDescriptor._name = playerName;
        playerNodeDescriptor._usageContext = NodeUsageContext::NODE_DYNAMIC;
        playerNodeDescriptor._componentMask = to_base(ComponentType::UNIT) |
                                              to_base(ComponentType::TRANSFORM) |
                                              to_base(ComponentType::BOUNDS) |
                                              to_base(ComponentType::NETWORKING);

        playerSGN = root.addChildNode(playerNodeDescriptor);
    }

    Attorney::SceneManagerScene::addPlayer(_parent, *this, playerSGN, queue);
    assert(playerSGN->get<UnitComponent>()->getUnit() != nullptr);
}

void Scene::removePlayerInternal(PlayerIndex idx) {
    assert(idx < _scenePlayers.size());
    
    Attorney::SceneManagerScene::removePlayer(_parent, *this, _scenePlayers[getSceneIndexForPlayer(idx)]->getBoundNode(), true);
}

void Scene::onPlayerAdd(const Player_ptr& player) {
    _scenePlayers.push_back(player.get());
    state().onPlayerAdd(player->index());
    input().onPlayerAdd(player->index());
}

void Scene::onPlayerRemove(const Player_ptr& player) {
    PlayerIndex idx = player->index();

    input().onPlayerRemove(idx);
    state().onPlayerRemove(idx);
    _cameraUpdateListeners[idx] = 0u;
    if (_flashLight.size() > idx) {
        SceneGraphNode* flashLight = _flashLight[idx];
        if (flashLight) {
            _sceneGraph->getRoot().removeChildNode(*flashLight);
            _flashLight[idx] = nullptr;
        }
    }
    _sceneGraph->getRoot().removeChildNode(*player->getBoundNode());

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

Player* Scene::getPlayerForIndex(PlayerIndex idx) const {
    return _scenePlayers[getSceneIndexForPlayer(idx)];
}

U8 Scene::getPlayerIndexForDevice(U8 deviceIndex) const {
    return input().getPlayerIndexForDevice(deviceIndex);
}

void Scene::clearObjects() {
    _xmlSceneGraph.clear();
    _flashLight.clear();
    _sceneGraph->unload();
}

bool Scene::mouseMoved(const Input::MouseMoveEvent& arg) {
    if (!arg.wheelEvent()) {
        const PlayerIndex idx = getPlayerIndexForDevice(arg._deviceIndex);
        DragSelectData& data = _dragSelectData[idx];
        if (data._isDragging) {
            data._endDragPos = arg.absolutePos();
            updateSelectionData(idx, data);
        } else {
            if (!state().playerState(idx).cameraLockedToMouse()) {
                findHoverTarget(idx, arg.absolutePos());
            } else if (Config::Build::ENABLE_EDITOR) {
                Editor& editor = _context.editor();
                if (editor.running() && !editor.scenePreviewHovered()) {
                    lockCameraToPlayerMouse(idx, false);
                }
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
    if (updated) {
        playerState.cameraUnderwater(checkCameraUnderwater(cam));
        return true;
    }

    return false;
}

void Scene::updateSceneState(const U64 deltaTimeUS) {
    OPTICK_EVENT();

    _sceneTimerUS += deltaTimeUS;
    updateSceneStateInternal(deltaTimeUS);
    _sceneGraph->sceneUpdate(deltaTimeUS, *_sceneState);
    _aiManager->update(deltaTimeUS);
}

void Scene::onStartUpdateLoop(const U8 loopNumber) {
    OPTICK_EVENT();

    _sceneGraph->onStartUpdateLoop(loopNumber);
}

void Scene::onLostFocus() {
    for (const Player* player : _scenePlayers) {
        state().playerState(player->index()).resetMovement();
    }

    //_paramHandler.setParam(_ID_32("freezeLoopTime"), true);
}

void Scene::registerTask(Task& taskItem, bool start, TaskPriority priority) {
    UniqueLock<SharedMutex> w_lock(_tasksMutex);
    _tasks.push_back(&taskItem);
    if (start) {
        Start(taskItem, priority);
    }
}

void Scene::clearTasks() {
    Console::printfn(Locale::get(_ID("STOP_SCENE_TASKS")));
    // Performance shouldn't be an issue here
    UniqueLock<SharedMutex> w_lock(_tasksMutex);
    for (Task* task : _tasks) {
        Wait(Stop(*task));
    }

    _tasks.clear();
}

void Scene::removeTask(Task& task) {
    UniqueLock<SharedMutex> w_lock(_tasksMutex);
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
                    [delta](D64 timer) { return timer + delta; });
}

void Scene::processTasks(const U64 deltaTimeUS) {
    D64 delta = Time::MicrosecondsToMilliseconds<D64>(deltaTimeUS);

    std::transform(std::begin(_taskTimers), std::end(_taskTimers), std::begin(_taskTimers),
                   [delta](D64 timer) { return timer + delta; });
}

void Scene::drawCustomUI(const Rect<I32>& targetViewport, GFX::CommandBuffer& bufferInOut) {
    ACKNOWLEDGE_UNUSED(targetViewport);

    if (_linesPrimitive->hasBatch()) {
        bufferInOut.add(_linesPrimitive->toCommandBuffer());
    }
}

void Scene::debugDraw(const Camera& activeCamera, RenderStagePass stagePass, GFX::CommandBuffer& bufferInOut) {
    if (!Config::Build::IS_SHIPPING_BUILD) {
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
                _octreePrimitives[i]->fromBox(box.getMin(), box.getMax(), UColour4(255, 0, 255, 255));
                bufferInOut.add(_octreePrimitives[i]->toCommandBuffer());
            }
        }
    }

    // Show NavMeshes
    _aiManager->debugDraw(bufferInOut, false);
    _lightPool->drawLightImpostors(stagePass._stage, bufferInOut);
    _envProbePool->debugDraw(bufferInOut);
}

bool Scene::checkCameraUnderwater(PlayerIndex idx) const {
    const Camera* crtCamera = Attorney::SceneManagerCameraAccessor::playerCamera(_parent, idx);
    return checkCameraUnderwater(*crtCamera);
}

bool Scene::checkCameraUnderwater(const Camera& camera) const {
    const vec3<F32>& eyePos = camera.getEye();
    {
        const auto& waterBodies = state().globalWaterBodies();
        for (const WaterDetails& water : waterBodies) {
            if (IS_IN_RANGE_INCLUSIVE(eyePos.y, water._heightOffset - water._depth, water._heightOffset)) {
                return true;
            }
        }
    }

    const auto& waterBodies = _sceneGraph->getNodesByType(SceneNodeType::TYPE_WATER);
    for (SceneGraphNode* node : waterBodies) {
        if (node && node->getNode<WaterPlane>().pointUnderwater(*node, eyePos)) {
            return true;
        }
    }

    return false;
}

void Scene::findHoverTarget(PlayerIndex idx, const vec2<I32>& aimPosIn) {
    const Camera& crtCamera = getPlayerForIndex(idx)->getCamera();

    const Rect<I32>& viewport = _context.gfx().getCurrentViewport();
    const vec2<U16>& displaySize = _context.activeWindow().getDimensions();
    vec2<I32> aimPos = COORD_REMAP(aimPosIn, Rect<I32>(0, 0, displaySize.width, displaySize.height), viewport);

    bool editorRunning = false;
    if_constexpr(Config::Build::ENABLE_EDITOR) {
        if (_context.editor().running() && _context.editor().scenePreviewFocused()) {
            editorRunning = true;
            const Rect<I32>& sceneRect = _context.editor().scenePreviewRect(false);
            if (sceneRect.contains(aimPos)) {
                aimPos = COORD_REMAP(aimPos, sceneRect, viewport);
            }
        }
    }
    const vec2<F32>& zPlanes = crtCamera.getZPlanes();

    F32 aimX = to_F32(aimPos.x);
    F32 aimY = viewport.w - to_F32(aimPos.y) - 1;

    const vec3<F32> startRay = crtCamera.unProject(aimX, aimY, 0.0f, viewport);
    const vec3<F32> endRay = crtCamera.unProject(aimX, aimY, 1.0f, viewport);
    // see if we select another one
    _sceneSelectionCandidates.resize(0);

    // Cast the picking ray and find items between the nearPlane and far Plane
    Ray mouseRay(startRay, startRay.direction(endRay));
    sceneGraph().intersect(mouseRay, zPlanes.x, zPlanes.y, _sceneSelectionCandidates);

    if (_currentHoverTarget[idx] != -1) {
        SceneGraphNode* target = _sceneGraph->findNode(_currentHoverTarget[idx]);
        if (target != nullptr) {
            target->clearFlag(SceneGraphNode::Flags::HOVERED);
        }
    }

    _currentHoverTarget[idx] = -1;
    if (!_sceneSelectionCandidates.empty()) {
        // If we don't force selections, remove all of the nodes that lack a SelectionComponent
        std::sort(std::begin(_sceneSelectionCandidates),
                  std::end(_sceneSelectionCandidates),
                  [](const SGNRayResult& A, const SGNRayResult& B) -> bool {
                      return A.dist < B.dist;
                  });

        SceneGraphNode* target = nullptr;
        for (const SGNRayResult& result : _sceneSelectionCandidates) {
            if (result.dist < 0.0f) {
                continue;
            }

            SceneGraphNode* crtNode = _sceneGraph->findNode(result.sgnGUI);
            if (crtNode && 
                (editorRunning || 
                (crtNode->get<SelectionComponent>() && crtNode->get<SelectionComponent>()->enabled())))
            {
                target = crtNode;
                break;
            }
        }

        // Well ... this happened somehow ...
        if (target == nullptr) {
            return;
        }

        _currentHoverTarget[idx] = target->getGUID();
        if (!target->hasFlag(SceneGraphNode::Flags::SELECTED)) {
            target->setFlag(SceneGraphNode::Flags::HOVERED);
        }
    }
}

void Scene::onNodeDestroy(SceneGraphNode& node) {
    const I64 guid = node.getGUID();
    for (auto iter : _currentHoverTarget) {
        if (iter.second == guid) {
            iter.second = -1;
        }
    }

    for (auto& [playerIdx, playerSelections] : _currentSelection) {
        for (I8 i = playerSelections._selectionCount; i > 0; --i) {
            I64 crtGUID = playerSelections._selections[i];
            if (crtGUID == guid) {
                std::swap(playerSelections._selections[i], playerSelections._selections[playerSelections._selectionCount--]);
            }
        }
    }
}

void Scene::resetSelection(PlayerIndex idx) {
    Selections& playerSelections = _currentSelection[idx];
    for (I8 i = 0; i < playerSelections._selectionCount; ++i) {
        SceneGraphNode* node = sceneGraph().findNode(playerSelections._selections[i]);
        if (node != nullptr) {
            node->clearFlag(SceneGraphNode::Flags::HOVERED);
            node->clearFlag(SceneGraphNode::Flags::SELECTED);
        }
    }
    playerSelections._selections.fill(-1);
    playerSelections._selectionCount = 0u;

    for (auto& cbk : _selectionChangeCallbacks) {
        cbk(idx, {});
    }
}

void Scene::setSelected(PlayerIndex idx, const vectorEASTL<SceneGraphNode*>& sgns) {
    Selections& playerSelections = _currentSelection[idx];

    for (SceneGraphNode* sgn : sgns) {
        playerSelections._selections[playerSelections._selectionCount++] = sgn->getGUID();
        sgn->setFlag(SceneGraphNode::Flags::SELECTED);
    }
    for (auto& cbk : _selectionChangeCallbacks) {
        cbk(idx, sgns);
    }
}

bool Scene::findSelection(PlayerIndex idx, bool clearOld) {
    // Clear old selection
    if (clearOld) {
        _parent.resetSelection(idx);
    }

    I64 hoverGUID = _currentHoverTarget[idx];
    // No hover target
    if (hoverGUID == -1) {
        return false;
    }

    Selections& playerSelections = _currentSelection[idx];
    for (U8 i = 0; i < playerSelections._selectionCount; ++i) {
        if (playerSelections._selections[i] == hoverGUID) {
            //Already selected
            return true;
        }
    }

    SceneGraphNode* selectedNode = _sceneGraph->findNode(hoverGUID);
    if (selectedNode != nullptr) {
        _parent.setSelected(idx, { selectedNode });
        return true;
    }
    _parent.resetSelection(idx);
    return false;
}


void Scene::updateSelectionData(PlayerIndex idx, DragSelectData& data) {
    static vectorSTD<Line> s_lines(4);
    static bool s_linesSet = false;
    if (!s_linesSet) {
        for (Line& line : s_lines) {
            line.widthStart(2.0f);
            line.widthEnd(1.0f);
            line.colourStart({ 0, 255, 0, 255 });
            line.colourEnd({ 0, 255, 0, 255 });
        }
        s_linesSet = true;
    }

    const vec2<U16>& displaySize = _context.activeWindow().getDimensions();
   
    vec2<I32> endPos = COORD_REMAP(data._endDragPos, Rect<I32>(0, 0, displaySize.width, displaySize.height), data._targetViewport);
    if (data._inEditor) {
        const Rect<I32>& sceneRect = _context.editor().scenePreviewRect(false);
        endPos = COORD_REMAP(sceneRect.clamp(endPos), sceneRect, data._targetViewport);
    } 

    I32 startX = data._startDragPos.x;
    I32 startY = data._targetViewport.w - data._startDragPos.y - 1;
    I32 endX = endPos.x;
    I32 endY = data._targetViewport.w - endPos.y - 1;

    Rect<I32> selectionRect = {
        std::min(startX, endX),
        std::min(startY, endY),
        std::max(startX, endX),
        std::max(startY, endY)
    };

    if (data._inEditor) {
        const Rect<I32>& targetViewport = _context.editor().getTargetViewport();
        const vec2<I32> tempStart = COORD_REMAP(selectionRect.xy(), data._targetViewport, targetViewport);
        const vec2<I32> tempEnd = COORD_REMAP(selectionRect.zw(), data._targetViewport, targetViewport);

        startX = tempStart.x;
        startY = tempStart.y;
        endX = tempEnd.x;
        endY = tempEnd.y;
    } else {
        startX = selectionRect.x;
        startY = selectionRect.y;
        endX = selectionRect.z;
        endY = selectionRect.w;
    }

    {
        { //X0, Y0 -> X1, Y0
            s_lines[0].pointStart({ startX, startY, 0 });
            s_lines[0].pointEnd({ endX, startY, 0 });
        }
        { //X1 , Y0 -> X1, Y1
            s_lines[1].pointStart({ endX, startY, 0 });
            s_lines[1].pointEnd({ endX, endY, 0 });
        }
        { //X1, Y1 -> X0, Y1
            s_lines[2].pointStart(s_lines[1].pointEnd());
            s_lines[2].pointEnd({ startX, endY, 0 });
        }
        { //X0, Y1 -> X0, Y0
            s_lines[3].pointStart(s_lines[2].pointEnd());
            s_lines[3].pointEnd(s_lines[0].pointStart());
        }
    }

    _linesPrimitive->fromLines(s_lines);

    _currentHoverTarget[idx] = -1;
    _parent.resetSelection(idx);

    const Camera& crtCamera = getPlayerForIndex(idx)->getCamera();
    vectorEASTL<SceneGraphNode*> nodes = Attorney::SceneManagerScene::getNodesInScreenRect(_parent, selectionRect, crtCamera, data._targetViewport, data._inEditor);
    _parent.setSelected(idx, nodes);
}

void Scene::beginDragSelection(PlayerIndex idx, bool editorRunning, vec2<I32> mousePos) {
    bool inEditor = false;
    Rect<I32> targetViewport = {};
    if_constexpr(Config::Build::ENABLE_EDITOR) {
        const Editor& editor = _context.editor();
        inEditor = editorRunning && editor.scenePreviewFocused();
        if (inEditor) {
            targetViewport = _context.gfx().getCurrentViewport();
        } else {
            const vec2<U16>& resolution = _context.gfx().renderingResolution();
            targetViewport = Rect<I32>(0, 0, resolution.width, resolution.height);
        }

        const vec2<U16>& displaySize = _context.activeWindow().getDimensions();
        mousePos = COORD_REMAP(mousePos, Rect<I32>(0, 0, displaySize.width, displaySize.height), targetViewport);
        if (inEditor) {
            const Rect<I32>& sceneRect = editor.scenePreviewRect(false);
            if (!sceneRect.contains(mousePos)) {
                return;
            }
            mousePos = COORD_REMAP(mousePos, sceneRect, targetViewport);
        }
    }

    if (!findSelection(idx, true)) {
        DragSelectData& data = _dragSelectData[idx];
        data._inEditor = inEditor;
        data._startDragPos = mousePos;
        data._endDragPos = mousePos;
        data._isDragging = true;
        data._targetViewport = targetViewport;
    }
}

void Scene::endDragSelection(PlayerIndex idx, bool editorRunning, vec2<I32> mousePos) {
    ACKNOWLEDGE_UNUSED(editorRunning);

    DragSelectData& data =_dragSelectData[idx];
    data._isDragging = false;
    data._endDragPos = mousePos;
    _linesPrimitive->clearBatch();
}

bool Scene::isDragSelecting(PlayerIndex idx) const {
    const auto it = _dragSelectData.find(idx);
    if (it != std::cend(_dragSelectData)) {
        return it->second._isDragging;
    }
    return false;
}

bool Scene::save(ByteBuffer& outputBuffer) const {
    const U8 playerCount = to_U8(_scenePlayers.size());
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