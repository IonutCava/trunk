#include "Headers/RenderPassManager.h"
#include "Rendering/RenderPass/Headers/RenderPass.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"

struct RenderPassCallOrder{
	bool operator()(const RenderPassItem& a, const RenderPassItem& b) const {
		return a._sortKey < b._sortKey;
	}
};

RenderPassManager::RenderPassManager()
{
	RenderQueue::createInstance();
}

RenderPassManager::~RenderPassManager()
{
	for_each(RenderPassItem& rpi, _renderPasses){
		SAFE_DELETE(rpi._rp);
	}
	_renderPasses.clear();
	RenderQueue::destroyInstance();
}

void RenderPassManager::render(const SceneRenderState& sceneRenderState, SceneGraph* activeSceneGraph) {
	for_each(RenderPassItem& rpi, _renderPasses){
		rpi._rp->render(sceneRenderState, activeSceneGraph);
	}
}

void RenderPassManager::addRenderPass(RenderPass* const renderPass, U8 orderKey) {
	assert(renderPass != NULL);
	_renderPasses.push_back(RenderPassItem(orderKey,renderPass));
	std::sort(_renderPasses.begin(), _renderPasses.end(), RenderPassCallOrder());
}

void RenderPassManager::removeRenderPass(RenderPass* const renderPass,bool deleteRP) {
	for(vectorImpl<RenderPassItem >::iterator it = _renderPasses.begin(); it != _renderPasses.end(); it++){
		if(it->_rp->getName().compare(renderPass->getName()) == 0){
			if(deleteRP){
				SAFE_DELETE(it->_rp);
			}
			_renderPasses.erase(it);
			break;
		}
	}
}

void RenderPassManager::removeRenderPass(const std::string& name,bool deleteRP) {
	for(vectorImpl<RenderPassItem >::iterator it = _renderPasses.begin(); it != _renderPasses.end(); it++){
		if(it->_rp->getName().compare(name) == 0){
			if(deleteRP){
				SAFE_DELETE(it->_rp);
			}
			_renderPasses.erase(it);
			break;
		}
	}
}

U16 RenderPassManager::getLastTotalBinSize(U8 renderPassId) const {
	if(renderPassId < _renderPasses.size()){
		return _renderPasses[renderPassId]._rp->getLasTotalBinSize();
	}
	return 0;
}