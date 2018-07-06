#include "Headers/Terrain.h"
#include "Headers/TerrainChunk.h"
#include "Headers/TerrainDescriptor.h"

#include "Quadtree/Headers/QuadtreeNode.h"

#include "Core/Headers/PlatformContext.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Predefined/Patch3D.h"

#include "Geometry/Material/Headers/Material.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

namespace {
    size_t g_PlaneCommandIndex = 0;
};

namespace Test {
    bool treeDirty = true;

    struct terrainNode_t {
        vec3<F32> origin;
        vec2<F32> dimensions;
        U8 type; // 1, 2, 3, 4 -- the child # relative to its parent. (0 == root)

        // Tessellation scale
        F32 tscale_negx; // negative x edge
        F32 tscale_posx; // Positive x edge
        F32 tscale_negz; // Negative z edge
        F32 tscale_posz; // Positive z edge

        terrainNode_t *p;  // Parent
        terrainNode_t *c1; // Children
        terrainNode_t *c2;
        terrainNode_t *c3;
        terrainNode_t *c4;

        terrainNode_t *n; // Neighbor to north
        terrainNode_t *s; // Neighbor to south
        terrainNode_t *e; // Neighbor to east
        terrainNode_t *w; // Neighbor to west
    };

    // The size of a patch in meters at which point to
    // stop subdividing a terrain patch once it's width is
    // less than the cutoff
    #define VMB_TERRAIN_REC_CUTOFF 10

    terrainNode_t *terrainTree;
    terrainNode_t *terrainTreeTail;
    I32 numTerrainNodes = 0;

    /**
    * Allocates a new node in the terrain quadtree with the specified parameters.
    */
    terrainNode_t* createNode(terrainNode_t *parent, U8 type, F32 x, F32 y, F32 z, F32 width, F32 height)
    {
        if (numTerrainNodes >= Terrain::MAX_RENDER_NODES) {
            return nullptr;
        }

        numTerrainNodes++;

        terrainTreeTail++;
        terrainTreeTail->type = type;
        terrainTreeTail->origin.set(x, y, z);
        terrainTreeTail->dimensions.set(width, height);
        terrainTreeTail->tscale_negx = 1.0;
        terrainTreeTail->tscale_negz = 1.0;
        terrainTreeTail->tscale_posx = 1.0;
        terrainTreeTail->tscale_posz = 1.0;
        terrainTreeTail->p = parent;
        terrainTreeTail->n = nullptr;
        terrainTreeTail->s = nullptr;
        terrainTreeTail->e = nullptr;
        terrainTreeTail->w = nullptr;
        return terrainTreeTail;
    }

    /**
    * Resets the terrain quadtree.
    */
    void terrain_clearTree()
    {
        terrainTreeTail = terrainTree;
        memset(terrainTree, 0, Terrain::MAX_RENDER_NODES * sizeof(terrainNode_t));
        numTerrainNodes = 0;
    }

    /**
    * Determines whether a node should be subdivided based on its distance to the camera.
    * Returns true if the node should be subdivided.
    */
    bool checkDivide(const vec3<F32>& camPos, terrainNode_t *node)
    {
        // Distance from current origin to camera
        F32 d = camPos.xz().distance(node->origin.xz());

        // Check base case:
        // Distance to camera is greater than twice the length of the diagonal
        // from current origin to corner of current square.
        // OR
        // Max recursion level has been hit
        if (d > 2.5f * std::sqrtf(std::pow(0.5f * node->dimensions.width, 2.0f) + 
                                  std::pow(0.5f * node->dimensions.height, 2.0f)) ||
            node->dimensions.width < VMB_TERRAIN_REC_CUTOFF)
        {
            return false;
        }

        return true;
    }

    /**
    * Returns true if node is sub-divided. False otherwise.
    */
    bool terrain_divideNode(const vec3<F32>& camPos, terrainNode_t *node)
    {
        // Subdivide
        F32 w_new = 0.5f * node->dimensions.width;
        F32 h_new = 0.5f * node->dimensions.height;

        // Create the child nodes
        node->c1 = createNode(node, 1u, node->origin.x - 0.5f * w_new, node->origin.y, node->origin.z - 0.5f * h_new, w_new, h_new);
        node->c2 = createNode(node, 2u, node->origin.x + 0.5f * w_new, node->origin.y, node->origin.z - 0.5f * h_new, w_new, h_new);
        node->c3 = createNode(node, 3u, node->origin.x + 0.5f * w_new, node->origin.y, node->origin.z + 0.5f * h_new, w_new, h_new);
        node->c4 = createNode(node, 4u, node->origin.x - 0.5f * w_new, node->origin.y, node->origin.z + 0.5f * h_new, w_new, h_new);

        // Assign neighbors
        switch (node->type) {
            case 1: {
                node->e = node->p->c2;
                node->n = node->p->c4;
            } break;
            case 2: {
                node->w = node->p->c1;
                node->n = node->p->c3;
            } break;
            case 3: {
                node->s = node->p->c2;
                node->w = node->p->c4;
            } break;
            case 4: {
                node->s = node->p->c1;
                node->e = node->p->c3;
            }break;
            default:
                break;
        };

        // Check if each of these four child nodes will be subdivided.
        bool div1 = checkDivide(camPos, node->c1);
        bool div2 = checkDivide(camPos, node->c2);
        bool div3 = checkDivide(camPos, node->c3);
        bool div4 = checkDivide(camPos, node->c4);

        if (div1) {
            terrain_divideNode(camPos, node->c1);
        }
        if (div2) {
            terrain_divideNode(camPos, node->c2);
        }
        if (div3) {
            terrain_divideNode(camPos, node->c3);
        }
        if (div4) {
            terrain_divideNode(camPos, node->c4);
        }

        return div1 || div2 || div3 || div4;
    }

    /**
    * Builds a terrain quadtree based on specified parameters and current camera position.
    */
    void terrain_createTree(const vec3<F32>& camPos, const vec3<F32>& origin, const vec2<U16>& terrainDimensions)
    {
        terrain_clearTree();

        terrainTree->type = 0u; // Root node
        terrainTree->origin.set(origin);
        terrainTree->dimensions.set(terrainDimensions.width, terrainDimensions.height);
        terrainTree->tscale_negx = 1.0;
        terrainTree->tscale_negz = 1.0;
        terrainTree->tscale_posx = 1.0;
        terrainTree->tscale_posz = 1.0;
        terrainTree->p = nullptr;
        terrainTree->n = nullptr;
        terrainTree->s = nullptr;
        terrainTree->e = nullptr;
        terrainTree->w = nullptr;

        // Recursively subdivide the terrain
        terrain_divideNode(camPos, terrainTree);
    }

    /**
    * Reserves memory for the terrrain quadtree and initializes the data structure.
    */
    void terrain_init()
    {
        terrainTree = MemoryManager_NEW terrainNode_t[Terrain::MAX_RENDER_NODES];
        terrain_clearTree();
    }

    /**
    * Frees memory for the terrain quadtree.
    */
    void terrain_shutdown()
    {
        MemoryManager::DELETE_ARRAY(terrainTree);
        terrainTreeTail = nullptr;
    }

    /**
    * Search for a node in the tree.
    * x, z == the point we are searching for (trying to find the node with an origin closest to that point)
    * n = the current node we are testing
    */
    terrainNode_t *find(terrainNode_t* n, F32 x, F32 z)
    {
        if (COMPARE(n->origin.x, x) && COMPARE(n->origin.z, z)) {
            return n;
        }

        if (!n->c1 && !n->c2 && !n->c3 && !n->c4) {
            return n;
        }
        
        if (IS_GEQUAL(n->origin.x, x) && IS_GEQUAL(n->origin.z, z) && n->c1) {
            return find(n->c1, x, z);
        } else if (IS_LEQUAL(n->origin.x, x) && IS_GEQUAL(n->origin.z, z) && n->c2) {
            return find(n->c2, x, z);
        } else if (IS_LEQUAL(n->origin.x, x) && IS_LEQUAL(n->origin.z, z) && n->c3) {
            return find(n->c3, x, z);
        } else if (IS_GEQUAL(n->origin.x, x) && IS_LEQUAL(n->origin.z, z) && n->c4) {
            return find(n->c4, x, z);
        }

        return n;
    }

    /**
    * Calculate the tessellation scale factor for a node depending on the neighboring patches.
    */
    void calcTessScale(terrainNode_t *node)
    {
        terrainNode_t *t;

        // Positive Z (north)
        t = find(terrainTree, node->origin.x, node->origin.z + 1 + node->dimensions.width * 0.5f);
        if (t->dimensions.width > node->dimensions.width) {
            node->tscale_posz = 2.0;
        }

        // Positive X (east)
        t = find(terrainTree, node->origin.x + 1 + node->dimensions.width * 0.5f, node->origin.z);
        if (t->dimensions.width > node->dimensions.width) {
            node->tscale_posx = 2.0;
        }

        // Negative Z (south)
        t = find(terrainTree, node->origin.x, node->origin.z - 1 - node->dimensions.width * 0.5f);
        if (t->dimensions.width > node->dimensions.width) {
            node->tscale_negz = 2.0;
        }

        // Negative X (west)
        t = find(terrainTree, node->origin.x - 1 - node->dimensions.width * 0.5f, node->origin.z);
        if (t->dimensions.width > node->dimensions.width) {
            node->tscale_negx = 2.0;
        }
    }

    struct NodeData {
        inline void set(const vec3<F32>& nodeOrigin,
                        F32 tileScale,
                        F32 tileScaleNegX,
                        F32 tileScaleNegZ,
                        F32 tileScalePosX,
                        F32 tileScalePosZ)
        {
            _positionAndTileScale.set(nodeOrigin, tileScale);
            _tScale.set(tileScaleNegX, tileScaleNegZ, tileScalePosX, tileScalePosZ);
        }

        vec4<F32> _positionAndTileScale;
        vec4<F32> _tScale;
    };

    std::array<Test::NodeData, Terrain::MAX_RENDER_NODES> _terrainRenderData;

    I32 maxRenderDepth = 1;
    I32 renderDepth = 0;

    /**
    * Pushes a node (patch) to the GPU to be drawn.
    * note: height parameter is here but not used. currently only dealing with square terrains (width is used only)
    */
    void terrain_renderNode(terrainNode_t *node)
    {
        // Calculate the tess scale factor
        calcTessScale(node);

        _terrainRenderData[renderDepth].set(node->origin,
                                            0.5f * node->dimensions.width,
                                            node->tscale_negx,
                                            node->tscale_negz,
                                            node->tscale_posx,
                                            node->tscale_posz);
    }

    /**
    * Traverses the terrain quadtree to draw nodes with no children.
    */
    void terrain_renderRecursive(terrainNode_t *node)
    {
        //if (renderDepth >= maxRenderDepth) {
        //    return;
        //}

        // If all children are null, render this node
        if (!node->c1 && !node->c2 && !node->c3 && !node->c4)
        {
            terrain_renderNode(node);
            renderDepth++;
            return;
        }

        // Otherwise, recruse to the children.
        if (node->c1) {
            terrain_renderRecursive(node->c1);
            terrain_renderRecursive(node->c2);
            terrain_renderRecursive(node->c3);
            terrain_renderRecursive(node->c4);
        }
    }

    /**
    * Draw the terrrain.
    */
    void terrain_render()
    {
        renderDepth = 0;
        terrain_renderRecursive(terrainTree);
    }

};

Terrain::Terrain(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name)
    : Object3D(context, parentCache, descriptorHash, name, ObjectType::TERRAIN, ObjectFlag::OBJECT_FLAG_NONE),
      _plane(nullptr),
      _shaderData(nullptr),
      _drawBBoxes(false),
      _underwaterDiffuseScale(100.0f),
      _chunkSize(1),
      _waterHeight(0.0f),
      _altitudeRange(0.0f, 1.0f)
{
}

Terrain::~Terrain()
{
}

bool Terrain::unload() {
    MemoryManager::DELETE_VECTOR(_terrainTextures);
    Test::terrain_shutdown();
    return Object3D::unload();
}

void Terrain::postLoad(SceneGraphNode& sgn) {
    static const U32 normalMask = to_base(SGNComponent::ComponentType::NAVIGATION) |
                                  to_base(SGNComponent::ComponentType::PHYSICS) |
                                  to_base(SGNComponent::ComponentType::BOUNDS) |
                                  to_base(SGNComponent::ComponentType::RENDERING) |
                                  to_base(SGNComponent::ComponentType::NETWORKING);

    SceneGraphNode_ptr planeSGN(sgn.addNode(_plane, normalMask, PhysicsGroup::GROUP_STATIC));
    planeSGN->setActive(false);
    //for (TerrainChunk* chunk : _terrainChunks) {
        //SceneGraphNode_ptr vegetation = sgn.addNode(Attorney::TerrainChunkTerrain::getVegetation(*chunk), normalMask);
        //vegetation->lockVisibility(true);
    //}
    // Skip Object3D::load() to avoid triangle list computation (extremely expensive)!!!

    ShaderBufferParams params;
    params._primitiveCount = Terrain::MAX_RENDER_NODES;
    params._primitiveSizeInBytes = sizeof(Test::NodeData);
    params._ringBufferLength = 1;
    params._unbound = true;
    params._updateFrequency = BufferUpdateFrequency::OFTEN;

    _shaderData = _context.newSB(params);
    sgn.get<RenderingComponent>()->registerShaderBuffer(ShaderBufferLocation::TERRAIN_DATA,
                                                        vec2<U32>(0, Terrain::MAX_RENDER_NODES),
                                                        *_shaderData);

    sgn.get<PhysicsComponent>()->setPosition(_offsetPosition);

    Test::terrain_init();
    SceneNode::postLoad(sgn);
}

void Terrain::buildQuadtree() {
    reserveTriangleCount((_terrainDimensions.x - 1) *
                         (_terrainDimensions.y - 1) * 2);
    _terrainQuadtree.Build(
        _context,
        _boundingBox,
        vec2<U32>(_terrainDimensions.x, _terrainDimensions.y),
        _chunkSize, this);

    // The terrain's final bounding box is the QuadTree's root bounding box
    _boundingBox.set(_terrainQuadtree.computeBoundingBox());

    const Material_ptr& mat = getMaterialTpl();

    const vec3<F32>& bbMin = _boundingBox.getMin();
    const vec3<F32>& bbExtent = _boundingBox.getExtent();

    for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
        for (U32 i = 0; i < to_base(RenderStage::COUNT); ++i) {
            RenderStage stage = static_cast<RenderStage>(i);

            const ShaderProgram_ptr& drawShader = mat->getShaderInfo(RenderStagePass(stage, static_cast<RenderPassType>(pass))).getProgram();

            drawShader->Uniform("bbox_min", bbMin);
            drawShader->Uniform("bbox_extent", bbExtent);
            drawShader->Uniform("underwaterDiffuseScale", _underwaterDiffuseScale);

            U8 textureOffset = to_U8(ShaderProgram::TextureUsage::COUNT);
            U8 layerOffset = 0;
            stringImpl layerIndex;
            for (U8 k = 0; k < _terrainTextures.size(); ++k) {
                layerOffset = k * 3 + textureOffset;
                layerIndex = to_stringImpl(k);
                TerrainTextureLayer* textureLayer = _terrainTextures[k];

                drawShader->Uniform(("texBlend[" + layerIndex + "]").c_str(), layerOffset);
                drawShader->Uniform(("texTileMaps[" + layerIndex + "]").c_str(), layerOffset + 1);
                drawShader->Uniform(("texNormalMaps[" + layerIndex + "]").c_str(), layerOffset + 2);
                drawShader->Uniform(("diffuseScale[" + layerIndex + "]").c_str(), textureLayer->getDiffuseScales());
                drawShader->Uniform(("detailScale[" + layerIndex + "]").c_str(), textureLayer->getDetailScales());

                if (pass == 0 && i == 0 ) {
                    getMaterialTpl()->addCustomTexture(textureLayer->blendMap(), layerOffset);
                    getMaterialTpl()->addCustomTexture(textureLayer->tileMaps(), layerOffset + 1);
                    getMaterialTpl()->addCustomTexture(textureLayer->normalMaps(), layerOffset + 2);
                }
            }
        }
    }
}

void Terrain::sceneUpdate(const U64 deltaTime,
                          SceneGraphNode& sgn,
                          SceneState& sceneState) {
    _waterHeight = sceneState.waterLevel();
    _terrainQuadtree.sceneUpdate(deltaTime, sgn, sceneState);
    Object3D::sceneUpdate(deltaTime, sgn, sceneState);
}

void Terrain::onCameraUpdate(SceneGraphNode& sgn,
                             const I64 cameraGUID,
                             const vec3<F32>& posOffset,
                             const mat4<F32>& rotationOffset) {
    ACKNOWLEDGE_UNUSED(sgn);
    ACKNOWLEDGE_UNUSED(cameraGUID);
    ACKNOWLEDGE_UNUSED(posOffset);
    ACKNOWLEDGE_UNUSED(rotationOffset);

    Test::treeDirty = true;
}

void Terrain::onCameraChange(SceneGraphNode& sgn,
                             const Camera& cam) {
    ACKNOWLEDGE_UNUSED(sgn);
    ACKNOWLEDGE_UNUSED(cam);

    Test::treeDirty = true;
}

void Terrain::initialiseDrawCommands(SceneGraphNode& sgn,
                                     const RenderStagePass& renderStagePass,
                                     GenericDrawCommands& drawCommandsInOut) {
   
    GenericDrawCommand cmd;
    cmd.primitiveType(PrimitiveType::PATCH);
    cmd.enableOption(GenericDrawCommand::RenderOptions::RENDER_TESSELLATED);
    cmd.sourceBuffer(getGeometryVB());
    cmd.patchVertexCount(4);
    cmd.cmd().indexCount = getGeometryVB()->getIndexCount();
    drawCommandsInOut.push_back(cmd);

    g_PlaneCommandIndex = drawCommandsInOut.size();
    if (renderStagePass._stage == RenderStage::DISPLAY) {
        //infinite plane
        GenericDrawCommand planeCmd;
        planeCmd.primitiveType(PrimitiveType::TRIANGLE_STRIP);
        planeCmd.cmd().firstIndex = 0;
        planeCmd.cmd().indexCount = _plane->getGeometryVB()->getIndexCount();
        planeCmd.LoD(0);
        planeCmd.sourceBuffer(_plane->getGeometryVB());
        planeCmd.shaderProgram(renderStagePass._passType == RenderPassType::DEPTH_PASS
                                                          ? _planeDepthShader
                                                          : _planeShader);
        drawCommandsInOut.push_back(planeCmd);

        //BoundingBoxes
        GenericDrawCommands commands;
        commands.reserve(getQuadtree().getChunkCount());

        _terrainQuadtree.drawBBox(_context, commands);
        for (const GenericDrawCommand& crtCmd : commands) {
            drawCommandsInOut.push_back(crtCmd);
        }
    }

    Object3D::initialiseDrawCommands(sgn, renderStagePass, drawCommandsInOut);
}

void Terrain::updateDrawCommands(SceneGraphNode& sgn,
                                 const RenderStagePass& renderStagePass,
                                 const SceneRenderState& sceneRenderState,
                                 GenericDrawCommands& drawCommandsInOut) {

    F32 minAltitude = _altitudeRange.x;
    F32 altitudeRange = _altitudeRange.y - _altitudeRange.x;

    //_context.setClipPlane(ClipPlaneIndex::CLIP_PLANE_0, Plane<F32>(WORLD_Y_AXIS, _waterHeight));
    //drawCommandsInOut.front().shaderProgram()->Uniform("dvd_waterHeight", _waterHeight);
    drawCommandsInOut.front().shaderProgram()->Uniform("TerrainOrigin", vec3<F32>(-(_terrainDimensions.width * 0.5f), 0.0f, -(_terrainDimensions.height * 0.5f)));
    drawCommandsInOut.front().shaderProgram()->Uniform("MinHeight", minAltitude);
    drawCommandsInOut.front().shaderProgram()->Uniform("HeightRange", altitudeRange);
    drawCommandsInOut.front().shaderProgram()->Uniform("TerrainLength", to_F32(_terrainDimensions.height));
    drawCommandsInOut.front().shaderProgram()->Uniform("TerrainWidth", to_F32(_terrainDimensions.width));
    drawCommandsInOut.front().enableOption(GenericDrawCommand::RenderOptions::RENDER_TESSELLATED);

    if (Test::treeDirty) {
        Test::terrain_createTree(Camera::activeCamera()->getEye(),
                                 vec3<F32>(0),
                                 _terrainDimensions);
        Test::treeDirty = false;
    }

    Test::terrain_render();
    drawCommandsInOut.front().drawCount(to_U16(Test::renderDepth));
    STUBBED("This may cause stalls. Profile! -Ionut");
#if 0
    _shaderData->updateData(0, Test::renderDepth, Test::_terrainRenderData.data());
#else
    _shaderData->setData(Test::_terrainRenderData.data());
#endif

    if (renderStagePass._stage == RenderStage::DISPLAY) {
        // draw infinite plane
       assert(drawCommandsInOut[g_PlaneCommandIndex].drawCount() == 1);

        _planeShader->Uniform("underwaterDiffuseScale", _underwaterDiffuseScale);
        _planeDepthShader->Uniform("underwaterDiffuseScale", _underwaterDiffuseScale);
        drawCommandsInOut[g_PlaneCommandIndex].shaderProgram(
            renderStagePass._passType == RenderPassType::DEPTH_PASS
                                       ? _planeDepthShader
                                       : _planeShader);
        size_t i = g_PlaneCommandIndex + 1;

        if (_drawBBoxes) {
            GenericDrawCommands commands;
            commands.reserve(getQuadtree().getChunkCount());
            _terrainQuadtree.drawBBox(_context, commands);

            for (const GenericDrawCommand& bbCmd : commands) {
                drawCommandsInOut[i++] = bbCmd;
            }

        } else {
            std::for_each(std::begin(drawCommandsInOut) + i,
                          std::end(drawCommandsInOut),
                          [](GenericDrawCommand& bbCmd) {
                                bbCmd.drawCount(0);
                          });
        }
    }

    Object3D::updateDrawCommands(sgn, renderStagePass, sceneRenderState, drawCommandsInOut);
}

vec3<F32> Terrain::getPositionFromGlobal(F32 x, F32 z) const {
    x -= _boundingBox.getCenter().x;
    z -= _boundingBox.getCenter().z;
    F32 xClamp = (0.5f * _terrainDimensions.x) + x;
    F32 zClamp = (0.5f * _terrainDimensions.y) - z;
    xClamp /= _terrainDimensions.x;
    zClamp /= _terrainDimensions.y;
    zClamp = 1 - zClamp;
    vec3<F32> temp = getPosition(xClamp, zClamp);

    return temp;
}

vec3<F32> Terrain::getPosition(F32 x_clampf, F32 z_clampf) const {
    assert(!(x_clampf < .0f || z_clampf < .0f || x_clampf > 1.0f ||
             z_clampf > 1.0f));

    vec2<F32> posF(x_clampf * _terrainDimensions.x,
                   z_clampf * _terrainDimensions.y);
    vec2<I32> posI(to_I32(posF.x), to_I32(posF.y));
    vec2<F32> posD(posF.x - posI.x, posF.y - posI.y);

    if (posI.x >= (I32)_terrainDimensions.x - 1)
        posI.x = _terrainDimensions.x - 2;
    if (posI.y >= (I32)_terrainDimensions.y - 1)
        posI.y = _terrainDimensions.y - 2;

    assert(posI.x >= 0 && posI.x < to_I32(_terrainDimensions.x) - 1 &&
           posI.y >= 0 && posI.y < to_I32(_terrainDimensions.y) - 1);

    vec3<F32> pos(
        _boundingBox.getMin().x +
            x_clampf * (_boundingBox.getMax().x - _boundingBox.getMin().x),
        0.0f,
        _boundingBox.getMin().z +
            z_clampf * (_boundingBox.getMax().z - _boundingBox.getMin().z));

    auto height = [&](U32 index) -> F32 {
        return _physicsVerts[index]._position.y;
    };

    pos.y = height(TER_COORD(posI.x, posI.y, to_I32(_terrainDimensions.x))) *
            (1.0f - posD.x) * (1.0f - posD.y) + 
            height(TER_COORD(posI.x + 1, posI.y, to_I32(_terrainDimensions.x))) *
            posD.x * (1.0f - posD.y) +
            height(TER_COORD(posI.x, posI.y + 1, to_I32(_terrainDimensions.x))) *
            (1.0f - posD.x) * posD.y +
            height(TER_COORD(posI.x + 1, posI.y + 1,to_I32(_terrainDimensions.x))) *
            posD.x * posD.y;

    return pos;
}

vec3<F32> Terrain::getNormal(F32 x_clampf, F32 z_clampf) const {
    assert(!(x_clampf < .0f || z_clampf < .0f || x_clampf > 1.0f ||
             z_clampf > 1.0f));

    vec2<F32> posF(x_clampf * _terrainDimensions.x,
                   z_clampf * _terrainDimensions.y);
    vec2<I32> posI(to_I32(x_clampf * _terrainDimensions.x),
                   to_I32(z_clampf * _terrainDimensions.y));
    vec2<F32> posD(posF.x - posI.x, posF.y - posI.y);

    if (posI.x >= to_I32(_terrainDimensions.x) - 1) {
        posI.x = to_I32(_terrainDimensions.x) - 2;
    }
    if (posI.y >= to_I32(_terrainDimensions.y) - 1) {
        posI.y = to_I32(_terrainDimensions.y) - 2;
    }
    assert(posI.x >= 0 && posI.x < to_I32(_terrainDimensions.x) - 1 &&
           posI.y >= 0 && posI.y < to_I32(_terrainDimensions.y) - 1);

    vec3<F32> normals[4];

    Util::UNPACK_VEC3(_physicsVerts[TER_COORD(posI.x, posI.y, to_I32(_terrainDimensions.x))]._normal, normals[0]);
    Util::UNPACK_VEC3(_physicsVerts[TER_COORD(posI.x + 1, posI.y, to_I32(_terrainDimensions.x))]._normal, normals[1]);
    Util::UNPACK_VEC3(_physicsVerts[TER_COORD(posI.x, posI.y + 1, to_I32(_terrainDimensions.x))]._normal, normals[2]);
    Util::UNPACK_VEC3(_physicsVerts[TER_COORD(posI.x + 1, posI.y + 1, to_I32(_terrainDimensions.x))]._normal, normals[3]);

    return normals[0] * (1.0f - posD.x) * (1.0f - posD.y) +
           normals[1] * posD.x * (1.0f - posD.y) +
           normals[2] * (1.0f - posD.x) * posD.y + 
           normals[3] * posD.x * posD.y;
}

vec3<F32> Terrain::getTangent(F32 x_clampf, F32 z_clampf) const {
    assert(!(x_clampf < .0f || z_clampf < .0f || x_clampf > 1.0f ||
             z_clampf > 1.0f));

    vec2<F32> posF(x_clampf * _terrainDimensions.x,
                   z_clampf * _terrainDimensions.y);
    vec2<I32> posI(to_I32(x_clampf * _terrainDimensions.x),
                   to_I32(z_clampf * _terrainDimensions.y));
    vec2<F32> posD(posF.x - posI.x, posF.y - posI.y);

    if (posI.x >= to_I32(_terrainDimensions.x) - 1) {
        posI.x = to_I32(_terrainDimensions.x) - 2;
    }
    if (posI.y >= to_I32(_terrainDimensions.y) - 1) {
        posI.y = to_I32(_terrainDimensions.y) - 2;
    }
    assert(posI.x >= 0 && posI.x < to_I32(_terrainDimensions.x) - 1 &&
           posI.y >= 0 && posI.y < to_I32(_terrainDimensions.y) - 1);

    vec3<F32> tangents[4];

    Util::UNPACK_VEC3(_physicsVerts[TER_COORD(posI.x, posI.y, to_I32(_terrainDimensions.x))]._tangent, tangents[0]);
    Util::UNPACK_VEC3(_physicsVerts[TER_COORD(posI.x + 1, posI.y, to_I32(_terrainDimensions.x))]._tangent, tangents[1]);
    Util::UNPACK_VEC3(_physicsVerts[TER_COORD(posI.x, posI.y + 1, to_I32(_terrainDimensions.x))]._tangent, tangents[2]);
    Util::UNPACK_VEC3(_physicsVerts[TER_COORD(posI.x + 1, posI.y + 1, to_I32(_terrainDimensions.x))]._tangent, tangents[3]);

    return tangents[0] * (1.0f - posD.x) * (1.0f - posD.y) +
           tangents[1] * posD.x * (1.0f - posD.y) +
           tangents[2] * (1.0f - posD.x) * posD.y +
           tangents[3] * posD.x * posD.y;
}

TerrainTextureLayer::~TerrainTextureLayer()
{
}
};