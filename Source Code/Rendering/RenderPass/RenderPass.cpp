#include "Headers/RenderPass.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/ShaderManager.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"

RenderPass::RenderPass(const std::string& name) : _name(name)
{
	_lastTotalBinSize = 0;
	_updateMaterials = false;
}

RenderPass::~RenderPass()
{
}
#pragma message("ToDo: Optimize material bind/unbind for each node -Ionut")
void RenderPass::render(SceneRenderState* const sceneRenderState){

	///Sort the render queue by the specified key
	///This only affects the final rendering stage
	RenderQueue::getInstance().sort();
	SceneNode* sn = NULL;
	SceneGraphNode* sgn = NULL;
	Transform* t = NULL;
	RenderStage currentStage = GFX_DEVICE.getRenderStage();
	U16 renderBinCount = RenderQueue::getInstance().getRenderQueueBinSize();
	_lastTotalBinSize = RenderQueue::getInstance().getRenderQueueStackSize();
	///Draw the entire queue;
	///Limited to 65536 (2^16) items per queue pass!
	if(sceneRenderState->drawObjects()){
		for(U16 i = 0; i < renderBinCount; i++){
			RenderBin* currentBin = RenderQueue::getInstance().getBinSorted(i);
			for(U16 j = 0; j < currentBin->getBinSize(); j++){
				//Get the current scene node
				sgn = currentBin->getItem(j)._node;
				///And validate it
				assert(sgn);
				///Get it's transform
				t = sgn->getTransform();
				///And it's attached SceneNode
				sn = sgn->getNode<SceneNode>();
				///Validate the SceneNode
				assert(sn);
				///Call any pre-draw operations on the SceneNode (refresh VBO, update materials, etc)
				sn->onDraw();
				///Try to see if we need a shader transform or a fixed pipeline one
				ShaderProgram* s = NULL;
				Material* m = sn->getMaterial();
				if(m){
					//m->computeShader(_updateMaterials,currentStage == DEPTH_STAGE ? DEPTH_STAGE : FINAL_STAGE);
					s = m->getShaderProgram(currentStage == DEPTH_STAGE ? DEPTH_STAGE : FINAL_STAGE);
				}
				///Check if we should draw the node. (only after onDraw as it may contain exclusion mask changes before draw)
				if(sn->getDrawState(currentStage)){
					///Transform the Object (Rot, Trans, Scale)
					if(!GFX_DEVICE.excludeFromStateChange(sn->getType())){ ///< only if the node is not in the exclusion mask
						GFX_DEVICE.setObjectState(t,false,s);
					}
					U8 lightCount = 0;
					if(!bitCompare(DEPTH_STAGE,currentStage)){
						///Find the most influental lights for this node. 
						///Use MAX_LIGHTS_PER_SCENE_NODE to allow more lights to affect this node
						lightCount = LightManager::getInstance().findLightsForSceneNode(sgn);
						///Update lights for this node
						LightManager::getInstance().update();
						///Only 2 sets of shadow maps for every node
                    }
					CLAMP<U8>(lightCount, 0, MAX_SHADOW_CASTING_LIGHTS_PER_NODE);
					///Apply shadows only from the most important 2 lights
					if(GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE | REFLECTION_STAGE)){
						U8 offset = 9;
						for(U8 n = 0; n < lightCount; n++, offset++){
							Light* l = LightManager::getInstance().getLightForCurrentNode(n);
							LightManager::getInstance().bindDepthMaps(l, n, offset);
						}
					}
					///setup materials and render the node
					///As nodes are sorted, this should be very fast
					///We need to apply different materials for each stage
					switch(currentStage){ //switch stat. with bit operatations? don't quite work ...
						default: ///All stages except shadow, including:
						case DISPLAY_STAGE: 
						case POSTFX_STAGE:
						case REFLECTION_STAGE:
						case ENVIRONMENT_MAPPING_STAGE: 
							sn->prepareMaterial(sgn);
							break;
						case DEPTH_STAGE:
                        case SHADOW_STAGE:
                        case SSAO_STAGE:
                        case DOF_STAGE:
							sn->prepareDepthMaterial(sgn);
							break;
					}
					///Call stage exclusion mask should do the rest
					sn->render(sgn); 
					///Unbind current material properties
					switch(currentStage){
						default: ///All stages except shadow, including:
						case DISPLAY_STAGE:
						case POSTFX_STAGE:
						case REFLECTION_STAGE:
						case ENVIRONMENT_MAPPING_STAGE:
							sn->releaseMaterial();
							break;
						case DEPTH_STAGE:
                        case SHADOW_STAGE:
                        case SSAO_STAGE:
                        case DOF_STAGE:
							sn->releaseDepthMaterial();
							break;
					}
					///Drop all applied transformations so that they do not affect the next node
					if(!GFX_DEVICE.excludeFromStateChange(sn->getType())){
						GFX_DEVICE.releaseObjectState(t,s);
					}
					///Apply shadows only from the most important 2 lights
					if(GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE | REFLECTION_STAGE)){
						U8 offset = (lightCount - 1) + 9;
						for(I32 n = lightCount - 1; n >= 0; n--,offset--){
							Light* l = LightManager::getInstance().getLightForCurrentNode(n);
							LightManager::getInstance().unbindDepthMaps(l, offset);
						}
					}
				}
				/// Perform any post draw operations regardless of the draw state
				sn->postDraw();
			}
		}
	}
	///Unbind all shaders after every render pass
	ShaderManager::getInstance().unbind();

	if(GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE)){
		for(U16 i = 0; i < renderBinCount; i++){
			RenderBin* currentBin = RenderQueue::getInstance().getBinSorted(i);
			for(U16 j = 0; j < currentBin->getBinSize(); j++){
				//Get the current scene node
				sgn = currentBin->getItem(j)._node;
				///Perform any last updates before the ...
				sgn->getNode<SceneNode>()->preFrameDrawEnd(sgn);
			}
		}
	}
	RenderQueue::getInstance().refresh();
	_updateMaterials = false;
}
