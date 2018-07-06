#include "Headers/RenderQueue.h"

#include "Headers/RenderBinMesh.h"
#include "Headers/RenderBinDelegate.h"
#include "Headers/RenderBinTranslucent.h"
#include "Headers/RenderBinParticles.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

RenderQueue::RenderQueue() : _renderQueueLocked(false), _isSorted(false)
{
    //_renderBins.reserve(RenderBin::RBT_PLACEHOLDER);
}

RenderQueue::~RenderQueue()
{
    MemoryManager::DELETE_HASHMAP( _renderBins );
}

U16 RenderQueue::getRenderQueueStackSize() const {
    U16 temp = 0;
	for (const RenderBinMap::value_type& renderBins : _renderBins) {
        temp += ( renderBins.second )->getBinSize();
    }
    return temp;
}

RenderBin* RenderQueue::getBin(RenderBin::RenderBinType rbType) {
    RenderBin* temp = nullptr;
    RenderBinMap::const_iterator binMapiter = _renderBins.find(rbType);
    if(binMapiter != _renderBins.end()){
        temp = binMapiter->second;
    }
    return temp;
}

SceneGraphNode* RenderQueue::getItem(U16 renderBin, U16 index){
    SceneGraphNode* temp = nullptr;
    if (renderBin < _renderBins.size()) {
        RenderBin* tempBin = _renderBins[_renderBinId[renderBin]];
        if(index < tempBin->getBinSize()){
            temp = tempBin->getItem(index)._node;
        }
    }
    return temp;
}

RenderBin* RenderQueue::getOrCreateBin(const RenderBin::RenderBinType& rbType) {
    RenderBinMap::const_iterator binMapiter = _renderBins.find(rbType);
    if(binMapiter != _renderBins.end()){
        return binMapiter->second;
    }

    RenderBin* temp = nullptr;

    switch(rbType) {
        case RenderBin::RBT_MESH : {
            // By state varies based on the current rendering stage
            temp = MemoryManager_NEW RenderBinMesh(RenderBin::RBT_MESH, RenderingOrder::BY_STATE, 0.0f);
        } break;
        case RenderBin::RBT_TERRAIN : {
            temp = MemoryManager_NEW RenderBinDelegate(RenderBin::RBT_TERRAIN, RenderingOrder::FRONT_TO_BACK, 1.0f);
        } break;
        case RenderBin::RBT_DELEGATE : {
            temp = MemoryManager_NEW RenderBinDelegate(RenderBin::RBT_DELEGATE, RenderingOrder::FRONT_TO_BACK, 2.0f);
        } break;
        case RenderBin::RBT_SHADOWS : {
            temp = MemoryManager_NEW RenderBinDelegate(RenderBin::RBT_SHADOWS, RenderingOrder::NONE, 3.0f);
        } break;
        case RenderBin::RBT_DECALS : {
            temp = MemoryManager_NEW RenderBinMesh(RenderBin::RBT_DECALS, RenderingOrder::FRONT_TO_BACK, 4.0f);
        } break;
        case RenderBin::RBT_SKY : {
            // Draw sky after opaque but before translucent to prevent overdraw
            temp = MemoryManager_NEW RenderBinDelegate(RenderBin::RBT_SKY, RenderingOrder::NONE, 5.0f);
        } break;
        case RenderBin::RBT_WATER : {
            // Water does not count as translucency, because rendering is very specific
            temp = MemoryManager_NEW RenderBinDelegate(RenderBin::RBT_WATER, RenderingOrder::BACK_TO_FRONT, 6.0f);
        } break;
        case RenderBin::RBT_VEGETATION_GRASS : {
            temp = MemoryManager_NEW RenderBinDelegate(RenderBin::RBT_VEGETATION_GRASS, RenderingOrder::BACK_TO_FRONT, 7.0f);
        } break;
        case RenderBin::RBT_VEGETATION_TREES : {
            temp = MemoryManager_NEW RenderBinDelegate(RenderBin::RBT_VEGETATION_TREES, RenderingOrder::BACK_TO_FRONT, 7.5f);
        } break;
        case RenderBin::RBT_PARTICLES : {
            temp = MemoryManager_NEW RenderBinParticles(RenderBin::RBT_PARTICLES, RenderingOrder::BACK_TO_FRONT, 8.0f);
        } break;
        case RenderBin::RBT_TRANSLUCENT : {
            // When rendering translucent objects we should sort each object's polygons depending on it's distance to the camera
            temp = MemoryManager_NEW RenderBinTranslucent(RenderBin::RBT_TRANSLUCENT, RenderingOrder::BACK_TO_FRONT, 9.0f);
        } break;
        case RenderBin::RBT_IMPOSTOR : {
            temp = MemoryManager_NEW RenderBinDelegate(RenderBin::RBT_IMPOSTOR, RenderingOrder::FRONT_TO_BACK, 9.9f);
        } break;
        default:
        case RenderBin::RBT_PLACEHOLDER : {
            ERROR_FN(Locale::get("ERROR_INVALID_RENDER_BIN_CREATION"));
        } break;
    };

    hashAlg::emplace(_renderBins, rbType, temp);
    hashAlg::emplace(_renderBinId, static_cast<U16>(_renderBins.size() - 1),  rbType);

    _sortedRenderBins.insert(std::lower_bound(_sortedRenderBins.begin(), 
                                              _sortedRenderBins.end(), 
                                              temp,
                                              [](RenderBin* const a, const RenderBin* const b) -> bool {
                                                  return a->getSortOrder() < b->getSortOrder();
                                              }), temp);
    _isSorted = false;
    
    return temp;
}

RenderBin* RenderQueue::getBinForNode(SceneNode* const node, Material* const matInstance){
    assert(node != nullptr);
    switch(node->getType()){
        case TYPE_LIGHT: {
            return getOrCreateBin(RenderBin::RBT_IMPOSTOR);
        }
        case TYPE_WATER: {
            return getOrCreateBin(RenderBin::RBT_WATER);
        }
        case TYPE_PARTICLE_EMITTER: {
            return getOrCreateBin(RenderBin::RBT_PARTICLES);
        }
        case TYPE_VEGETATION_GRASS: {
            return getOrCreateBin(RenderBin::RBT_VEGETATION_GRASS);
        }
        case TYPE_VEGETATION_TREES: {
            return getOrCreateBin(RenderBin::RBT_VEGETATION_TREES);
        }
        case TYPE_SKY: {
            return getOrCreateBin(RenderBin::RBT_SKY);
        }
        case TYPE_OBJECT3D         : { 
            if(static_cast<Object3D*>(node)->getType() == Object3D::TERRAIN){
                return getOrCreateBin(RenderBin::RBT_TERRAIN);
            }
            //Check if the object has a material with translucency
            if (matInstance && matInstance->isTranslucent()){
                //Add it to the appropriate bin if so ...
                return getOrCreateBin(RenderBin::RBT_TRANSLUCENT);
            }
            //... else add it to the general geometry bin
            return getOrCreateBin(RenderBin::RBT_MESH);
        }
    }
    return nullptr;
}

void RenderQueue::addNodeToQueue(SceneGraphNode* const sgn, const vec3<F32>& eyePos){
    assert(sgn != nullptr);
    RenderingComponent* renderingCmp = sgn->getComponent<RenderingComponent>();
    RenderBin* rb = getBinForNode(sgn->getNode(), 
                                  renderingCmp ? renderingCmp->getMaterialInstance() :
                                                 nullptr);
    if (rb) {
        rb->addNodeToBin(sgn, eyePos);
    }
    _isSorted = false;
}

void RenderQueue::sort(const RenderStage& currentRenderStage){
    /*if(_renderQueueLocked && _isSorted) {
        return;
    }*/

	for (RenderBinMap::value_type& renderBin : _renderBins) {
        assert( renderBin.second );
        renderBin.second->sort( currentRenderStage );
    }

    _isSorted = true;
}

void RenderQueue::refresh(bool force){
    if ( _renderQueueLocked && !force ) {
        return;
    }
	for (RenderBinMap::value_type& renderBin : _renderBins) {
        assert( renderBin.second );
        renderBin.second->refresh();
    }
    _isSorted = false;
}

void RenderQueue::lock(){
    _renderQueueLocked = true;
}

void RenderQueue::unlock(bool resetNodes){
    _renderQueueLocked = false;

    if ( resetNodes ) {
        refresh();
    }
}

};