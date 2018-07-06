#include "Headers/RenderQueue.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Rendering/Headers/Frustum.h"
#include "Hardware/Video/RenderStateBlock.h"

RenderQueueItem::RenderQueueItem(P32 sortKey, SceneGraphNode *node ) : _node( node ),
																	   _sortKey( sortKey ) {
	/// Defaulting to a null state hash
	_stateHash = 0;
	Material* mat = _node->getNode()->getMaterial();
	/// If we have a material
	if(mat){
		/// Sort by state hash depending on the current rendering stage
		if(GFX_DEVICE.getRenderStage() == REFLECTION_STAGE){
			/// Check if we have a reflection state
			RenderStateBlock* reflectionState = mat->getRenderState(REFLECTION_STAGE);
			if(reflectionState != NULL){
				/// Save the render state hash value for sorting
				_stateHash = reflectionState->getHash();
			}else{ /// else, use the final rendering state as that will be used to render in reflection
				/// all materials should have at least one final render state
				assert(mat->getRenderState(FINAL_STAGE) != NULL);
				_stateHash = mat->getRenderState(FINAL_STAGE)->getHash();
			}
		}else if(GFX_DEVICE.getRenderStage() == SHADOW_STAGE){
			/// Check if we have a reflection state
			RenderStateBlock* shadowState = mat->getRenderState(SHADOW_STAGE);
			/// If we do not have a special shadow state, don't worry, just use 0 for all similar nodes
			/// the SceneGraph will use a global state on them, so using 0 is still sort-correct
			if(shadowState != NULL){
				/// Save the render state hash value for sorting
				_stateHash = shadowState->getHash();
			}
		}else{
			/// all materials should have at least one final render state
			assert(mat->getRenderState(FINAL_STAGE) != NULL);
			/// Save the render state has value for sorting
			_stateHash = mat->getRenderState(FINAL_STAGE)->getHash();
		}
	}

}

/// Sorting opaque items is a 2 step process:
/// 1: sort by shaders
/// 2: if the shader is identical, sort by state hash
struct RenderQueueKeyCompare{
	///Sort 
	bool operator()( const RenderQueueItem &a, const RenderQueueItem &b ) const{
		/// No need to sort by shaders in shadow stage
		if(GFX_DEVICE.getRenderStage() != SHADOW_STAGE){
			/// The sort key is the shader id (for now)
			if(  a._sortKey.i < b._sortKey.i ) 
				return true;
	        /// The _stateHash is a CRC value created based on the RenderState.
			/// Might wanna generate a hash based on mose important values instead, but it's not important at this stage
			if( a._sortKey.i == b._sortKey.i ) 
				return a._stateHash < b._stateHash;
			/// different shaders
			return false;
		}
		/// In shadow stage, sort by state!
		return a._stateHash < b._stateHash;
	}
};



struct RenderQueueDistanceBacktoFront{
	bool operator()( const RenderQueueItem &a, const RenderQueueItem &b) const {
		const vec3& eye = Frustum::getInstance().getEyePos();
		F32 dist_a = a._node->getBoundingBox().nearestDistanceFromPoint(eye);
		F32 dist_b = b._node->getBoundingBox().nearestDistanceFromPoint(eye);
		///ToDo: REMOVE WATER DISTANCE CHECK HACK!!!!!!
		if(a._node->getNode()->getType() == TYPE_WATER) return true;
		if(b._node->getNode()->getType() == TYPE_WATER) return false;
		return dist_a < dist_b;
	}
};

struct RenderQueueDistanceFrontToBack{
	bool operator()( const RenderQueueItem &a, const RenderQueueItem &b) const {
		const vec3& eye = Frustum::getInstance().getEyePos();
		F32 dist_a = a._node->getBoundingBox().nearestDistanceFromPoint(eye);
		F32 dist_b = b._node->getBoundingBox().nearestDistanceFromPoint(eye);
		return dist_a > dist_b;
	}
};

void RenderQueue::sort(){
	const vec3& eye = Frustum::getInstance().getEyePos();
	std::sort( _translucentStack.begin(), _translucentStack.end(), RenderQueueDistanceBacktoFront());
	std::sort( _opaqueStack.begin(), _opaqueStack.end(), RenderQueueKeyCompare()  );
	//Render opaque items first
	for_each(RenderQueueStack::value_type& s, _opaqueStack){
		_renderQueue.push_back(s);
	}
	//Then translucent
	for_each(RenderQueueStack::value_type& s, _translucentStack){
		_renderQueue.push_back(s);
	}
}

void RenderQueue::addNodeToQueue(SceneGraphNode* const sgn){
	assert(sgn != NULL);
	boost::unique_lock< boost::mutex > lock_access_here(_renderQueueMutex);
	if(sgn->getNode()){
		P32 key;
		Material* nodeMaterial = sgn->getNode()->getMaterial();
		if(nodeMaterial){
			key = nodeMaterial->getMaterialId();
			if(sgn->getNode()->getMaterial()->isTranslucent() || sgn->getNode()->getType() == TYPE_WATER){
				_translucentStack.push_back(RenderQueueItem(key,sgn));
			}else{
				_opaqueStack.push_back(RenderQueueItem(key,sgn));
			}
		}else{
			key.i = _renderQueue.size()+1;
			_renderQueue.push_back(RenderQueueItem(key,sgn));
		}
	}
}

const RenderQueueItem& RenderQueue::getItem(I32 index){
	boost::unique_lock< boost::mutex > lock_access_here(_renderQueueMutex);
	return _renderQueue[index];
}
 
void RenderQueue::refresh(){
	boost::unique_lock< boost::mutex > lock_access_here(_renderQueueMutex);
	_renderQueue.resize(0);
	_translucentStack.resize(0);
	_opaqueStack.resize(0);
}

U32 RenderQueue::getRenderQueueStackSize(){
	boost::unique_lock< boost::mutex > lock_access_here(_renderQueueMutex);
	return _renderQueue.size();
}
