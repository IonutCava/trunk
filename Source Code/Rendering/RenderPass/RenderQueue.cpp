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

RenderQueue::~RenderQueue()
{
	for_each(RenderBinMap::value_type& renderBins, _renderBins){
		SAFE_DELETE(renderBins.second);
	}
	_renderBins.clear();
}

U16 RenderQueue::getRenderQueueStackSize() {
	U16 temp = 0;
	for_each(RenderBinMap::value_type& renderBins, _renderBins){
		temp += (renderBins.second)->getBinSize();
	}
	return temp;
}

RenderBin* RenderQueue::getBinSorted(U16 renderBin){
	return _sortedRenderBins[renderBin];
}

RenderBin* RenderQueue::getBin(U16 renderBin){
	return getBin(_renderBinId[renderBin]);
}

RenderBin* RenderQueue::getBin(RenderBin::RenderBinType rbType){
	RenderBin* temp = NULL;
	RenderBinMap::iterator binMapiter = _renderBins.find(rbType);
	if(binMapiter != _renderBins.end()){
		temp = binMapiter->second;
	}
	return temp;
}

SceneGraphNode* RenderQueue::getItem(U16 renderBin, U16 index){
	SceneGraphNode* temp = NULL;
	if(renderBin < _renderBins.size()){
		RenderBin* tempBin = _renderBins[_renderBinId[renderBin]];
		if(index < tempBin->getBinSize()){
			temp = tempBin->getItem(index)._node;
		}
	}
	return temp;
}

RenderBin* RenderQueue::getOrCreateBin(const RenderBin::RenderBinType& rbType){
	RenderBin* temp = NULL;
	RenderBinMap::iterator binMapiter = _renderBins.find(rbType);
	if(binMapiter != _renderBins.end()){
		temp = binMapiter->second;
	}else{
		switch(rbType){
			case RenderBin::RBT_SKY:
				temp = New RenderBinDelegate(RenderBin::RBT_SKY,RenderingOrder::NONE,0.01);
				break;
			case RenderBin::RBT_MESH :
				temp = New RenderBinMesh(RenderBin::RBT_MESH,RenderingOrder::FRONT_TO_BACK,0.1);
				break;
            case RenderBin::RBT_IMPOSTOR:
                temp = New RenderBinDelegate(RenderBin::RBT_IMPOSTOR,RenderingOrder::NONE,0.95);
                break;
			case RenderBin::RBT_TERRAIN:
				temp = New RenderBinDelegate(RenderBin::RBT_TERRAIN,RenderingOrder::NONE,0.2);
				break;
			case RenderBin::RBT_DELEGATE:
				temp = New RenderBinDelegate(RenderBin::RBT_DELEGATE,RenderingOrder::NONE,0.3);
				break;
			case RenderBin::RBT_SHADOWS:
				temp = New RenderBinDelegate(RenderBin::RBT_SHADOWS,RenderingOrder::NONE,0.4);
				break;
			case RenderBin::RBT_DECALS:
				temp = New RenderBinMesh(RenderBin::RBT_DECALS,RenderingOrder::FRONT_TO_BACK,0.5);
				break;
			case RenderBin::RBT_WATER:
				///Water does not count as translucency, because rendering is very specific
				temp = New RenderBinDelegate(RenderBin::RBT_WATER,RenderingOrder::BACK_TO_FRONT,0.6);
				break;
			case RenderBin::RBT_FOLIAGE:
				temp = New RenderBinDelegate(RenderBin::RBT_FOLIAGE,RenderingOrder::BACK_TO_FRONT,0.7);
				break;
			case RenderBin::RBT_PARTICLES:
				temp = New RenderBinParticles(RenderBin::RBT_PARTICLES,RenderingOrder::BACK_TO_FRONT,0.8);
				break;
			case RenderBin::RBT_TRANSLUCENT:
				temp = New RenderBinTranslucent(RenderBin::RBT_TRANSLUCENT,RenderingOrder::BACK_TO_FRONT,0.9);
				break;
			default:
			case RenderBin::RBT_PLACEHOLDER:
				ERROR_FN(Locale::get("ERROR_INVALID_RENDER_BIN_CREATION"));
				break;
		};
		_renderBins.insert(std::make_pair(rbType,temp));
		_renderBinId.insert(std::make_pair(_renderBins.size() - 1,  rbType));
		_sortedRenderBins.push_back(temp);
		std::sort(_sortedRenderBins.begin(), _sortedRenderBins.end(), RenderBinCallOrder());
	}
	return temp;
}

RenderBin* RenderQueue::getBinForNode(SceneNode* const node){
	RenderBin* rb = NULL;
	switch(node->getType()){
		default:
			//skip
			break;
		case TYPE_TERRAIN :
			rb = getOrCreateBin(RenderBin::RBT_TERRAIN);
			break;
        case TYPE_LIGHT :
            rb = getOrCreateBin(RenderBin::RBT_IMPOSTOR);
            break;
		case TYPE_WATER :
			rb = getOrCreateBin(RenderBin::RBT_WATER);
			break;
		case TYPE_PARTICLE_EMITTER :
			rb = getOrCreateBin(RenderBin::RBT_PARTICLES);
			break;
		case TYPE_SKY:
			rb = getOrCreateBin(RenderBin::RBT_SKY);
			break;
		case TYPE_OBJECT3D : {
			///Check if the object has a material with translucency
			Material* nodeMaterial = node->getMaterial();
			if(nodeMaterial){
				if(nodeMaterial->isTranslucent()){
					///Add it to the appropriate bin if so ...
					rb = getOrCreateBin(RenderBin::RBT_TRANSLUCENT);
					break;
				}
			}
			///... else add it to the general geoemtry bin
			rb = getOrCreateBin(RenderBin::RBT_MESH);
			}break;
		};
	return rb;
}

void RenderQueue::addNodeToQueue(SceneGraphNode* const sgn){
	assert(sgn != NULL);
	SceneNode* sn = sgn->getNode<SceneNode>();
	if(sn != NULL){
		RenderBin* rb = getBinForNode(sn);
		if(rb){
			rb->addNodeToBin(sgn);
		}
	}
}

void RenderQueue::sort(){
	for_each(RenderBinMap::value_type& renderBin, _renderBins){
      assert(renderBin.second);
      renderBin.second->sort();
   }
}

void RenderQueue::refresh(){
	for_each(RenderBinMap::value_type& renderBin, _renderBins){
      assert(renderBin.second);
      renderBin.second->refresh();
   }
}