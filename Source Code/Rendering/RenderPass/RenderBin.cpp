#include "Headers/RenderBin.h"
#include "Rendering/Headers/Frustum.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"

RenderBinItem::RenderBinItem(P32 sortKey, SceneGraphNode* const node ) : _node( node ),
															    		 _sortKey( sortKey ) {
	/// Defaulting to a null state hash
	_stateHash = 0;
	Material* mat = _node->getNode<SceneNode>()->getMaterial();
	/// If we have a material
	if(mat){
		/// Sort by state hash depending on the current rendering stage
		if(GFX_DEVICE.isCurrentRenderStage(REFLECTION_STAGE)){
			/// Check if we have a reflection state
			RenderStateBlock* reflectionState = mat->getRenderState(REFLECTION_STAGE);
			if(reflectionState != NULL){
				/// Save the render state hash value for sorting
				_stateHash = reflectionState->getHash();
			}else{ /// else, use the final rendering state as that will be used to render in reflection
				/// all materials should have at least one final render state
				RenderStateBlock* finalState = mat->getRenderState(FINAL_STAGE);
				assert(finalState != NULL);
				_stateHash = finalState->getHash();
			}
		}else if(GFX_DEVICE.isCurrentRenderStage(DEPTH_STAGE)){
			/// Check if we have a shadow state
			RenderStateBlock* depthState = mat->getRenderState(DEPTH_STAGE);
			/// If we do not have a special shadow state, don't worry, just use 0 for all similar nodes
			/// the SceneGraph will use a global state on them, so using 0 is still sort-correct
			if(depthState != NULL){
				/// Save the render state hash value for sorting
				_stateHash = depthState->getHash();
			}
		}else{
			/// all materials should have at least one final render state
			RenderStateBlock* finalState = mat->getRenderState(FINAL_STAGE);
			assert(finalState != NULL);
			/// Save the render state has value for sorting
			_stateHash = finalState->getHash();
		}
	}
}

/// Sorting opaque items is a 2 step process:
/// 1: sort by shaders
/// 2: if the shader is identical, sort by state hash
struct RenderQueueKeyCompare{
	///Sort 
	bool operator()( const RenderBinItem &a, const RenderBinItem &b ) const{
		/// No need to sort by shaders in shadow stage -Update: Since animation update, shadow stage uses shaders as well
		//if(GFX_DEVICE.getRenderStage() != DEPTH_STAGE){ ///Sort by shader in all states

			/// The sort key is the shader id (for now)
			if(  a._sortKey.i < b._sortKey.i ) 
				return true;
	        /// The _stateHash is a CRC value created based on the RenderState.
			/// Might wanna generate a hash based on mose important values instead, but it's not important at this stage
			if( a._sortKey.i == b._sortKey.i ) 
				return a._stateHash < b._stateHash;
			/// different shaders
			return false;

		//}
		/// In shadow stage, sort by state!
		//return a._stateHash < b._stateHash;
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
}

void RenderBin::sort(){
	WriteLock w_lock(_renderBinGetMutex);
	switch(_renderOrder){
		default:
		case RenderingOrder::BY_STATE : 
			std::sort( _renderBinStack.begin(), _renderBinStack.end(), RenderQueueKeyCompare());
			break;
		case RenderingOrder::BACK_TO_FRONT:
			std::sort( _renderBinStack.begin(), _renderBinStack.end(),RenderQueueDistanceBacktoFront());
			break;
		case RenderingOrder::FRONT_TO_BACK:
			std::sort( _renderBinStack.begin(), _renderBinStack.end(),RenderQueueDistanceFrontToBack());
			break;
		case RenderingOrder::NONE:
			///no need to sort
			break;
		case RenderingOrder::ORDER_PLACEHOLDER:
			ERROR_FN(Locale::get("ERROR_INVALID_RENDER_BIN_SORT_ORDER"),rBinTypeToString(_rbType));
			break;
	};
	
}

void RenderBin::refresh(){
	WriteLock w_lock(_renderBinGetMutex);
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

const RenderBinItem& RenderBin::getItem(U16 index){
	assert(index < _renderBinStack.size());
	return _renderBinStack[index];
	
}

std::string RenderBin::rBinTypeToString(const RenderBinType& rbt) {
	std::string name;
	switch(rbt){
		default:
		case RBT_PLACEHOLDER:
			name = "Invalid Bin";
			break;
		case RBT_MESH:
			name = "Mesh Bin";
			break;
		case RBT_DELEGATE:
			name = "Delegate Bin";
			break;
		case RBT_TRANSLUCENT:
			name = "Translucent Bin";
			break;
		case RBT_SKY:
			name = "Sky Bin";
			break;
		case RBT_WATER:
			name = "Water Bin";
			break;
		case RBT_TERRAIN:
			name = "Terrain Bin";
			break;
		case RBT_FOLIAGE:
			name = "Folliage Bin";
			break;
		case RBT_PARTICLES:
			name = "Particle Bin";
			break;
		case RBT_DECALS:
			name = "Decals Bin";
			break;
		case RBT_SHADOWS:
			name = "Shadow Bin";
			break;
	};
	return name;
}

void RenderBin::render(){
}