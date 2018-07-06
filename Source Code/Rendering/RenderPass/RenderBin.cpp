#include "Headers/RenderBin.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Managers/Headers/LightManager.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Material/Headers/Material.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"

RenderBinItem::RenderBinItem(I32 sortKeyA, I32 sortKeyB, F32 distToCamSq, SceneGraphNode* const node) : _node(node),
                                                                                                        _sortKeyA( sortKeyA ),
                                                                                                        _sortKeyB( sortKeyB ),
                                                                                                        _distanceToCameraSq(distToCamSq)
{
    Material* mat = _node->getNode()->getMaterial();
    // If we do not have a material, no need to continue
    if(!mat) return;

    // Sort by state hash depending on the current rendering stage
    // Save the render state hash value for sorting
    _stateHash = GFX_DEVICE.getStateBlockDescriptor(mat->getRenderStateBlock(GFX_DEVICE.getRenderStage())).getHash();
}

struct RenderQueueDistanceBacktoFront{
    bool operator()( const RenderBinItem &a, const RenderBinItem &b) const {
        return a._distanceToCameraSq < b._distanceToCameraSq;
    }
};

struct RenderQueueDistanceFrontToBack{
    bool operator()( const RenderBinItem &a, const RenderBinItem &b) const {
        return a._distanceToCameraSq > b._distanceToCameraSq;
    }
};

/// Sorting opaque items is a 3 step process:
/// 1: sort by shaders
/// 2: if the shader is identical, sort by state hash
/// 3: if shader is identical and state hash is identical, sort by albedo ID
struct RenderQueueKeyCompare{
    //Sort
    bool operator()(const RenderBinItem &a, const RenderBinItem &b) const{
        //Sort by shader in all states The sort key is the shader id (for now)
        if (a._sortKeyA < b._sortKeyA)
            return true;
        if (a._sortKeyA > b._sortKeyA)
            return false;

            // If the shader values are the same, we use the state hash for sorting
            // The _stateHash is a CRC value created based on the RenderState.
        if (a._stateHash < b._stateHash)
            return true;
        if (a._stateHash > b._stateHash)
            return false;

        // If both the shader are the same and the state hashes match, we sort by the secondary key (usually the texture id)
        return (a._sortKeyB < b._sortKeyB);
    }
};

RenderBin::RenderBin(const RenderBinType& rbType,const RenderingOrder::List& renderOrder,D32 drawKey) : _rbType(rbType),
                                                                                                        _renderOrder(renderOrder),
                                                                                                        _drawKey(drawKey)
{
    _renderBinStack.reserve(125);
    renderBinTypeToNameMap[RBT_PLACEHOLDER] = "Invalid Bin";
    renderBinTypeToNameMap[RBT_MESH]        = "Mesh Bin";
    renderBinTypeToNameMap[RBT_IMPOSTOR]    = "Impostor Bin";
    renderBinTypeToNameMap[RBT_DELEGATE]    = "Delegate Bin";
    renderBinTypeToNameMap[RBT_TRANSLUCENT] = "Translucent Bin";
    renderBinTypeToNameMap[RBT_SKY]         = "Sky Bin";
    renderBinTypeToNameMap[RBT_WATER]       = "Water Bin";
    renderBinTypeToNameMap[RBT_TERRAIN]     = "Terrain Bin";
    renderBinTypeToNameMap[RBT_PARTICLES]   = "Particle Bin";
    renderBinTypeToNameMap[RBT_VEGETATION_GRASS]   = "Grass Bin";
    renderBinTypeToNameMap[RBT_VEGETATION_TREES]   = "Trees Bin";
    renderBinTypeToNameMap[RBT_DECALS]      = "Decals Bin";
    renderBinTypeToNameMap[RBT_SHADOWS]     = "Shadow Bin";
}

void RenderBin::sort(const RenderStage& currentRenderStage){
    //WriteLock w_lock(_renderBinGetMutex);
    switch(_renderOrder){
        default:
        case RenderingOrder::BY_STATE:{
            if(bitCompare(DEPTH_STAGE, currentRenderStage))
                std::sort(_renderBinStack.begin(), _renderBinStack.end(), RenderQueueDistanceFrontToBack());
            else
                std::sort(_renderBinStack.begin(), _renderBinStack.end(), RenderQueueKeyCompare());
            }break;
        case RenderingOrder::BACK_TO_FRONT:
            std::sort(_renderBinStack.begin(), _renderBinStack.end(), RenderQueueDistanceBacktoFront());
            break;
        case RenderingOrder::FRONT_TO_BACK:
            std::sort(_renderBinStack.begin(), _renderBinStack.end(), RenderQueueDistanceFrontToBack());
            break;
        case RenderingOrder::NONE:
            //no need to sort
            break;
        case RenderingOrder::ORDER_PLACEHOLDER:
            ERROR_FN(Locale::get("ERROR_INVALID_RENDER_BIN_SORT_ORDER"),renderBinTypeToNameMap[_rbType]);
            break;
    };
}

void RenderBin::refresh(){
    //WriteLock w_lock(_renderBinGetMutex);
    _renderBinStack.resize(0);
    _renderBinStack.reserve(125);
}

void RenderBin::addNodeToBin(SceneGraphNode* const sgn, const vec3<F32>& eyePos){
    SceneNode* sn = sgn->getNode();
    I32 keyA = (U32)_renderBinStack.size() + 1;
    I32 keyB = keyA;
    Material* nodeMaterial = sn->getMaterial();
    F32 distToCam = sgn->getBoundingBoxConst().nearestDistanceFromPointSquared(eyePos);
    if(nodeMaterial){
        nodeMaterial->getSortKeys(keyA, keyB);
    }
    _renderBinStack.push_back(RenderBinItem(keyA, keyB, distToCam, sgn));
}

void RenderBin::preRender(const RenderStage& currentRenderStage){
}

void RenderBin::render(const SceneRenderState& renderState, const RenderStage& currentRenderStage){
    SceneNode* sn = nullptr;
    SceneGraphNode* sgn = nullptr;
    LightManager& lightMgr = LightManager::getInstance();
    //if needed, add more stages to which lighting is applied
    U32 lightValidStages = DISPLAY_STAGE | REFLECTION_STAGE;
    bool isDepthPass = bitCompare(DEPTH_STAGE, currentRenderStage);
    bool isLightValidStage = bitCompare(lightValidStages, currentRenderStage);
    
    for(U16 j = 0; j < getBinSize(); j++){
        //Get the current scene node and validate it
        sgn = getItem(j)._node;
        assert(sgn);
        //Call any pre-draw operations on the SceneGraphNode (e.g. tick animations)
        if(!sgn->onDraw(currentRenderStage))
            continue; //< If the SGN isn't ready for rendering, skip it this frame
        //And get it's attached SceneNode and validate it
        sn = sgn->getNode();
        if (!sn) continue;
        //Check if we should draw the node. (only after onDraw as it may contain exclusion mask changes before draw)
        if (sn->getDrawState(currentRenderStage) && sn->isReadyForDraw(currentRenderStage)) {
            U8 lightCount = 0;
            if(!isDepthPass && sn->getType() != TYPE_SKY){
                //Find the most influential lights for this node.
                //Use MAX_LIGHTS_PER_SCENE_NODE to allow more lights to affect this node
                lightCount = lightMgr.findLightsForSceneNode(sgn);
            }
            //Only 2 sets of shadow maps for every node
            CLAMP<U8>(lightCount, 0, Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE);
            //Apply shadows only from the most important MAX_SHADOW_CASTING_LIGHTS_PER_NODE lights
            for (U8 lightIndex = 0; lightIndex < lightCount && isLightValidStage; lightIndex++)
                lightMgr.bindDepthMaps(lightIndex);

            //setup materials and render the node
            //As nodes are sorted, this should be very fast
            //We need to apply different materials for each stage
            if (isDepthPass ? sn->prepareDepthMaterial(sgn) : sn->prepareMaterial(sgn)){
                //Call render and the stage exclusion mask should do the rest
                sn->render(sgn, renderState);
            }

        }

        sgn->postDraw(currentRenderStage);
    }
}

void RenderBin::postRender(const RenderStage& currentRenderStage){
    SceneGraphNode* sgn = nullptr;

    for(U16 j = 0; j < getBinSize(); j++){
        sgn = getItem(j)._node;
        //Perform any last updates before the preFrameDrawEnd
        sgn->getNode()->preFrameDrawEnd(sgn);
    }
}