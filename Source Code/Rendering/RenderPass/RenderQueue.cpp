#include "Headers/RenderQueue.h"

#include "Headers/RenderBinMesh.h"
#include "Headers/RenderBinDelegate.h"
#include "Headers/RenderBinTranslucent.h"
#include "Headers/RenderBinParticles.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Rendering/Headers/Frustum.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Material/Headers/Material.h"

struct RenderBinCallOrder{
    bool operator()(RenderBin* a, RenderBin* b) const {
        return a->getSortOrder() < b->getSortOrder();
    }
};

RenderQueue::RenderQueue() : _renderQueueLocked(false), _isSorted(false)
{
    //_renderBins.reserve(RenderBin::RBT_PLACEHOLDER);
}

RenderQueue::~RenderQueue()
{
    FOR_EACH(RenderBinMap::value_type& renderBins, _renderBins){
        SAFE_DELETE(renderBins.second);
    }
    _renderBins.clear();
}

U16 RenderQueue::getRenderQueueStackSize() {
    U16 temp = 0;
    FOR_EACH(RenderBinMap::value_type& renderBins, _renderBins){
        temp += (renderBins.second)->getBinSize();
    }
    return temp;
}

RenderBin* RenderQueue::getBin(RenderBin::RenderBinType rbType){
    RenderBin* temp = nullptr;
    RenderBinMap::iterator binMapiter = _renderBins.find(rbType);
    if(binMapiter != _renderBins.end()){
        temp = binMapiter->second;
    }
    return temp;
}

SceneGraphNode* RenderQueue::getItem(U16 renderBin, U16 index){
    SceneGraphNode* temp = nullptr;
    if(renderBin < _renderBins.size()){
        RenderBin* tempBin = _renderBins[_renderBinId[renderBin]];
        if(index < tempBin->getBinSize()){
            temp = tempBin->getItem(index)._node;
        }
    }
    return temp;
}

RenderBin* RenderQueue::getOrCreateBin(const RenderBin::RenderBinType& rbType){
    RenderBin* temp = nullptr;
    RenderBinMap::iterator binMapiter = _renderBins.find(rbType);
    if(binMapiter != _renderBins.end()){
        temp = binMapiter->second;
    }else{
        switch(rbType){
            case RenderBin::RBT_MESH :
                temp = New RenderBinMesh(RenderBin::RBT_MESH,RenderingOrder::BY_STATE, 0.0f); //< "By state varies based on the current rendering stage"
                break;
            case RenderBin::RBT_TERRAIN:
                temp = New RenderBinDelegate(RenderBin::RBT_TERRAIN,RenderingOrder::FRONT_TO_BACK, 1.0f);
                break;
            case RenderBin::RBT_DELEGATE:
                temp = New RenderBinDelegate(RenderBin::RBT_DELEGATE,RenderingOrder::FRONT_TO_BACK, 2.0f);
                break;
            case RenderBin::RBT_SHADOWS:
                temp = New RenderBinDelegate(RenderBin::RBT_SHADOWS,RenderingOrder::NONE, 3.0f);
                break;
            case RenderBin::RBT_DECALS:
                temp = New RenderBinMesh(RenderBin::RBT_DECALS,RenderingOrder::FRONT_TO_BACK, 4.0f);
                break;
            case RenderBin::RBT_SKY:
                //Draw sky after opaque but before translucent to prevent overdraw
                temp = New RenderBinDelegate(RenderBin::RBT_SKY,RenderingOrder::NONE, 5.0f);
                break;
            case RenderBin::RBT_WATER:
                //Water does not count as translucency, because rendering is very specific
                temp = New RenderBinDelegate(RenderBin::RBT_WATER,RenderingOrder::BACK_TO_FRONT, 6.0f);
                break;
            case RenderBin::RBT_VEGETATION_GRASS:
                temp = New RenderBinDelegate(RenderBin::RBT_VEGETATION_GRASS,RenderingOrder::BACK_TO_FRONT, 7.0f);
                break;
            case RenderBin::RBT_VEGETATION_TREES:
                temp = New RenderBinDelegate(RenderBin::RBT_VEGETATION_TREES,RenderingOrder::BACK_TO_FRONT, 7.5f);
                break;
            case RenderBin::RBT_PARTICLES:
                temp = New RenderBinParticles(RenderBin::RBT_PARTICLES,RenderingOrder::BACK_TO_FRONT, 8.0f);
                break;
            case RenderBin::RBT_TRANSLUCENT:
                ///When rendering translucent objects, we should also sort each object's polygons depending on it's distance from the camera
                temp = New RenderBinTranslucent(RenderBin::RBT_TRANSLUCENT,RenderingOrder::BACK_TO_FRONT, 9.0f);
                break;
            case RenderBin::RBT_IMPOSTOR:
                temp = New RenderBinDelegate(RenderBin::RBT_IMPOSTOR,RenderingOrder::FRONT_TO_BACK, 9.9f);
                break;
            default:
            case RenderBin::RBT_PLACEHOLDER:
                ERROR_FN(Locale::get("ERROR_INVALID_RENDER_BIN_CREATION"));
                break;
        };
        _renderBins.insert(std::make_pair(rbType,temp));
        _renderBinId.insert(std::make_pair((U16)(_renderBins.size() - 1),  rbType));
        _sortedRenderBins.push_back(temp);
        std::sort(_sortedRenderBins.begin(), _sortedRenderBins.end(), RenderBinCallOrder());
        _isSorted = false;
    }
    return temp;
}

RenderBin* RenderQueue::getBinForNode(SceneNode* const node){
    switch(node->getType()){
        case TYPE_TERRAIN          : return getOrCreateBin(RenderBin::RBT_TERRAIN);
        case TYPE_LIGHT            : return getOrCreateBin(RenderBin::RBT_IMPOSTOR);
        case TYPE_WATER            : return getOrCreateBin(RenderBin::RBT_WATER);
        case TYPE_PARTICLE_EMITTER : return getOrCreateBin(RenderBin::RBT_PARTICLES);
        case TYPE_VEGETATION_GRASS : return getOrCreateBin(RenderBin::RBT_VEGETATION_GRASS);
        case TYPE_VEGETATION_TREES : return getOrCreateBin(RenderBin::RBT_VEGETATION_TREES);
        case TYPE_SKY              : return getOrCreateBin(RenderBin::RBT_SKY);
        case TYPE_OBJECT3D         : { 
            //Check if the object has a material with translucency
            Material* nodeMaterial = node->getMaterial();
            if(nodeMaterial && nodeMaterial->isTranslucent()){
                //Add it to the appropriate bin if so ...
                return getOrCreateBin(RenderBin::RBT_TRANSLUCENT);
            }
            //... else add it to the general geometry bin
            return getOrCreateBin(RenderBin::RBT_MESH);
        }
    }
    return nullptr;
}

void RenderQueue::addNodeToQueue(SceneGraphNode* const sgn){
    assert(sgn != nullptr);
    RenderBin* rb = getBinForNode(sgn->getNode());
    if(rb) rb->addNodeToBin(sgn);
    _isSorted = false;
}

void RenderQueue::sort(const RenderStage& currentRenderStage){
    /*if(_renderQueueLocked && _isSorted)
        return;*/

    FOR_EACH(RenderBinMap::value_type& renderBin, _renderBins){
        assert(renderBin.second);
        renderBin.second->sort(currentRenderStage);
    }
    _isSorted = true;
}

void RenderQueue::refresh(bool force){
    if(_renderQueueLocked && !force)
        return;

    FOR_EACH(RenderBinMap::value_type& renderBin, _renderBins){
        assert(renderBin.second);
        renderBin.second->refresh();
    }
    _isSorted = false;
}

void RenderQueue::lock(){
    _renderQueueLocked = true;
}

void RenderQueue::unlock(bool resetNodes){
    _renderQueueLocked = false;

    if (resetNodes) refresh();
}