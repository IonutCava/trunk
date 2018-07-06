#include "Headers/RenderBin.h"
#include "Rendering/Headers/Frustum.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Managers/Headers/LightManager.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Material/Headers/Material.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"

RenderBinItem::RenderBinItem(P32 sortKey, SceneGraphNode* const node ) : _node( node ),
                                                                         _sortKey( sortKey ),
                                                                         _stateHash(0)//< Defaulting to a null state hash
{
    RenderStateBlock* currentStateBlock = NULL;
    Material* mat = _node->getNode<SceneNode>()->getMaterial();
    // If we do not have a material, no need to continue
    if(!mat) return;

    // Sort by state hash depending on the current rendering stage
    if(GFX_DEVICE.isCurrentRenderStage(REFLECTION_STAGE)){
        // Check if we have a reflection state
        currentStateBlock = mat->getRenderState(REFLECTION_STAGE);
         // else, use the final rendering state as that will be used to render in reflection
        if(!currentStateBlock){
            // all materials should have at least one final render state
            currentStateBlock = mat->getRenderState(FINAL_STAGE);
            assert(currentStateBlock != NULL);
        }
    }else if(GFX_DEVICE.isCurrentRenderStage(DEPTH_STAGE)){
        // Check if we have a shadow state
        currentStateBlock = mat->getRenderState(DEPTH_STAGE);
        // If we do not have a special shadow state, don't worry, just use 0 for all similar nodes
        // the SceneGraph will use a global state on them, so using 0 is still sort-correct
        if(!currentStateBlock)	return;
    }else{
        // all materials should have at least one final render state
        currentStateBlock = mat->getRenderState(FINAL_STAGE);
        assert(currentStateBlock != NULL);
    }
    // Save the render state hash value for sorting
    _stateHash = currentStateBlock->getGUID();
}

/// Sorting opaque items is a 2 step process:
/// 1: sort by shaders
/// 2: if the shader is identical, sort by state hash
struct RenderQueueKeyCompare{
    //Sort
    bool operator()( const RenderBinItem &a, const RenderBinItem &b ) const{
        //Sort by shader in all states
        // The sort key is the shader id (for now)
        if(  a._sortKey.i < b._sortKey.i )
            return true;
        // The _stateHash is a CRC value created based on the RenderState.
        // Might wanna generate a hash based on mose important values instead, but it's not important at this stage
        if( a._sortKey.i == b._sortKey.i )
            return a._stateHash < b._stateHash;
        // different shaders
        return false;
    }
};

struct RenderQueueDistanceBacktoFront{
    bool operator()( const RenderBinItem &a, const RenderBinItem &b) const {
        const vec3<F32>& eye = Frustum::getInstance().getEyePos();
        F32 dist_a = a._node->getBoundingBox().nearestDistanceFromPoint(eye);
        F32 dist_b = b._node->getBoundingBox().nearestDistanceFromPoint(eye);
        return dist_a < dist_b;
    }
};

struct RenderQueueDistanceFrontToBack{
    bool operator()( const RenderBinItem &a, const RenderBinItem &b) const {
        const vec3<F32>& eye = Frustum::getInstance().getEyePos();
        F32 dist_a = a._node->getBoundingBox().nearestDistanceFromPoint(eye);
        F32 dist_b = b._node->getBoundingBox().nearestDistanceFromPoint(eye);
        return dist_a > dist_b;
    }
};

RenderBin::RenderBin(const RenderBinType& rbType,const RenderingOrder::List& renderOrder,D32 drawKey) : _rbType(rbType),
                                                                                                        _renderOrder(renderOrder),
                                                                                                        _drawKey(drawKey)
{
    _renderBinStack.reserve(250);
    renderBinTypeToNameMap[RBT_PLACEHOLDER] = "Invalid Bin";
    renderBinTypeToNameMap[RBT_MESH]        = "Mesh Bin";
    renderBinTypeToNameMap[RBT_IMPOSTOR]    = "Impostor Bin";
    renderBinTypeToNameMap[RBT_DELEGATE]    = "Delegate Bin";
    renderBinTypeToNameMap[RBT_TRANSLUCENT] = "Translucent Bin";
    renderBinTypeToNameMap[RBT_SKY]         = "Sky Bin";
    renderBinTypeToNameMap[RBT_WATER]       = "Water Bin";
    renderBinTypeToNameMap[RBT_TERRAIN]     = "Terrain Bin";
    renderBinTypeToNameMap[RBT_FOLIAGE]     = "Folliage Bin";
    renderBinTypeToNameMap[RBT_PARTICLES]   = "Particle Bin";
    renderBinTypeToNameMap[RBT_DECALS]      = "Decals Bin";
    renderBinTypeToNameMap[RBT_SHADOWS]     = "Shadow Bin";
}

void RenderBin::sort(){
    //WriteLock w_lock(_renderBinGetMutex);
    switch(_renderOrder){
        default:
        case RenderingOrder::BY_STATE :
            std::sort(_renderBinStack.begin(), _renderBinStack.end(), RenderQueueKeyCompare());
            break;
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
}

void RenderBin::addNodeToBin(SceneGraphNode* const sgn){
    SceneNode* sn = sgn->getNode<SceneNode>();
    P32 key;
    key.i = _renderBinStack.size() + 1;
    Material* nodeMaterial = sn->getMaterial();
    if(nodeMaterial){
        key = nodeMaterial->getMaterialId();
    }
    _renderBinStack.push_back(RenderBinItem(key,sgn));
}

void RenderBin::preRender(){
}

void RenderBin::render(const RenderStage& currentRenderStage){
    SceneNode* sn = NULL;
    SceneGraphNode* sgn = NULL;
    LightManager& lightMgr = LightManager::getInstance();
    //if needed, add more stages to which lighting is applied
    U32 lightValidStages = DISPLAY_STAGE | REFLECTION_STAGE;
    bool isDepthPass = bitCompare(DEPTH_STAGE, currentRenderStage);
    bool isLightValidStage = bitCompare(lightValidStages, currentRenderStage);

    for(U16 j = 0; j < getBinSize(); j++){
        //Get the current scene node and validate it
        sgn = getItem(j)._node;
        assert(sgn);
        //And get it's attached SceneNode and validate it
        sn = sgn->getNode<SceneNode>();
        assert(sn);

        //Call any pre-draw operations on the SceneNode (refresh VBO, update materials, etc)
        sn->onDraw(currentRenderStage);

        //Check if we should draw the node. (only after onDraw as it may contain exclusion mask changes before draw)
        if(sn->getDrawState(currentRenderStage)) {
            U8 lightCount = 0;
            if(!isDepthPass){
                //Find the most influental lights for this node.
                //Use MAX_LIGHTS_PER_SCENE_NODE to allow more lights to affect this node
                lightCount = lightMgr.findLightsForSceneNode(sgn);
                //Update lights for this node
                lightMgr.update();
                //Only 2 sets of shadow maps for every node
            }

            CLAMP<U8>(lightCount, 0, Config::MAX_SHADOW_CASTING_LIGHTS_PER_NODE);
            //Apply shadows only from the most important MAX_SHADOW_CASTING_LIGHTS_PER_NODE lights
            if(isLightValidStage){
                U8 offset = 9;
                for(U8 n = 0; n < lightCount; n++, offset++){
                    Light* l = lightMgr.getLightForCurrentNode(n);
                    lightMgr.bindDepthMaps(l, n, offset);
                }
            }

            //setup materials and render the node
            //As nodes are sorted, this should be very fast
            //We need to apply different materials for each stage
            isDepthPass ?  sn->prepareDepthMaterial(sgn) : sn->prepareMaterial(sgn);

            //Call render and the stage exclusion mask should do the rest
            sn->render(sgn);

            //Unbind current material properties
            isDepthPass ?  sn->releaseDepthMaterial() : sn->releaseMaterial();

            //Apply shadows only from the most important MAX_SHADOW_CASTING_LIGHTS_PER_NODE lights
            if(isLightValidStage){
                U8 offset = (lightCount - 1) + 9;
                for(I32 n = lightCount - 1; n >= 0; n--,offset--){
                    Light* l = lightMgr.getLightForCurrentNode(n);
                    lightMgr.unbindDepthMaps(l, offset);
                }
            }
        }

        // Perform any post draw operations regardless of the draw state
        sn->postDraw(currentRenderStage);
    }
}

void RenderBin::postRender(){
    SceneGraphNode* sgn = NULL;

    for(U16 j = 0; j < getBinSize(); j++){
        sgn = getItem(j)._node;
        //Perform any last updates before the preFrameDrawEnd
        sgn->getNode<SceneNode>()->preFrameDrawEnd(sgn);
    }
}