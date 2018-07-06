#include "Headers/Terrain.h"
#include "Headers/TerrainDescriptor.h"

#include "Core/Headers/ParamHandler.h"
#include "Quadtree/Headers/Quadtree.h"
#include "Managers/Headers/SceneManager.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Hardware/Video/Buffers/VertexBufferObject/Headers/VertexBufferObject.h"

#define COORD(x,y,w)	((y)*(w)+(x))

bool Terrain::unload(){

	SAFE_DELETE(_terrainQuadtree);
	SAFE_DELETE(_groundVBO);
	SAFE_DELETE(_terrainRenderState);
	SAFE_DELETE(_veg);

	assert(!_terrainTextures.empty());
	for_each(TerrainTextureMap::value_type& it, _terrainTextures){
		if(it.second != NULL){///Remember kids, alpha channel is optional ;) -Ionut
			RemoveResource(it.second);
		}
	}
	_terrainTextures.clear();

	return SceneNode::unload(); 
}

/// Visual resources must be loaded in the rendering thread to gain acces to the current graphic context
void Terrain::loadVisualResources(){


	_terrainTextures[TERRAIN_TEXTURE_DIFFUSE]->setTextureWrap(TEXTURE_REPEAT,
															  TEXTURE_REPEAT,
															  TEXTURE_REPEAT);


	if(_terrainTextures[TERRAIN_TEXTURE_ALPHA] != NULL){
		_alphaTexturePresent = true;
	}

	///Generate a render state
	RenderStateBlockDescriptor terrainDesc;
	terrainDesc.setCullMode(CULL_MODE_CW);
	terrainDesc._fixedLighting = true;
	_terrainRenderState = GFX_DEVICE.createStateBlock(terrainDesc);

	///For now, terrain doesn't cast shadows
	getSceneNodeRenderState().addToDrawExclusionMask(SHADOW_STAGE);
}

bool Terrain::loadThreadedResources(TerrainDescriptor* const terrain){
	///Terrain dimensions:
	///    |-----------------------|        /\						      /\
	///    |          /\           |         |					         /  \
 	///    |          |            |         |					    /\__/    \
	///    |          |            | _terrainHeightScaleFactor  /\   /          \__/\___
	///    |<-_terrainScaleFactor->|         |				 |  --/                   \
	///    |          |            |         |                /                          \
	///    |          |            |         |              |-                            \
	///    |          |            |         \/            /_______________________________\
	///    |_________\/____________|

	_terrainHeightScaleFactor = terrain->getScale().y;

	_boundingBox.set(vec3<F32>(-300,  0.0f, -300),
					 vec3<F32>( 300, 40.0f,  300));
	_boundingBox.Translate(terrain->getPosition());
	_boundingBox.Multiply(vec3<F32>(terrain->getScale().x,1,terrain->getScale().x));
	_boundingBox.MultiplyMax(vec3<F32>(1,_terrainHeightScaleFactor,1));

	_groundVBO = GFX_DEVICE.newVBO();
	vectorImpl<vec3<F32> >&		normalData	= _groundVBO->getNormal();
	vectorImpl<vec3<F32> >&		tangentData	= _groundVBO->getTangent();

	U8 d; U32 t, id;
	bool alpha = false;
	U8* data = ImageTools::OpenImage(terrain->getVariable("heightmap"), 
									_terrainWidth,
									_terrainHeight,
									 d, t, id,alpha);

	D_PRINT_FN(Locale::get("TERRAIN_INFO"),_terrainWidth, _terrainHeight);
	assert(data);
	U32 heightmapWidth  = _terrainWidth;
	U32 heightmapHeight = _terrainHeight;

	if(_terrainWidth%2==0)	_terrainWidth++;
	if(_terrainHeight%2==0)	_terrainHeight++;

	_groundVBO->resizePositionCount(_terrainWidth*_terrainHeight);
	normalData.resize(_terrainWidth*_terrainHeight);
	tangentData.resize(_terrainWidth*_terrainHeight);
	vec3<F32> vertexData;
	for(U32 j=0; j < _terrainHeight; j++) {
		for(U32 i=0; i < _terrainWidth; i++) {

			U32 idxHM = COORD(i,j,_terrainWidth);
			vertexData.x = _boundingBox.getMin().x + ((F32)i) * 
				                (_boundingBox.getMax().x - _boundingBox.getMin().x)/(_terrainWidth-1);
			vertexData.z = _boundingBox.getMin().z + ((F32)j) * 
				                (_boundingBox.getMax().z - _boundingBox.getMin().z)/(_terrainHeight-1);
			U32 idxIMG = COORD(	i<heightmapWidth? i : i-1,
								j<heightmapHeight? j : j-1,
								heightmapWidth);

			F32 h = (F32)(data[idxIMG*d + 0] + data[idxIMG*d + 1] + data[idxIMG*d + 2])/3.0f;

			vertexData.y = (_boundingBox.getMin().y + ((F32)h) * 
				                 (_boundingBox.getMax().y - _boundingBox.getMin().y)/(255)) * _terrainHeightScaleFactor;
			_groundVBO->modifyPositionValue(idxHM,vertexData);

		}
	}
	SAFE_DELETE_ARRAY(data);

	U32 offset = 2;


			
	for(U32 j=offset; j < _terrainHeight-offset; j++) {
		for(U32 i=offset; i < _terrainWidth-offset; i++) {

			U32 idx = COORD(i,j,_terrainWidth);
			
			vec3<F32> vU = _groundVBO->getPosition(COORD(i+offset, j+0, _terrainWidth)) -  _groundVBO->getPosition(COORD(i-offset, j+0, _terrainWidth));
			vec3<F32> vV = _groundVBO->getPosition(COORD(i+0, j+offset, _terrainWidth)) -  _groundVBO->getPosition(COORD(i+0, j-offset, _terrainWidth));

			normalData[idx].cross(vV, vU);
			normalData[idx].normalize();
			tangentData[idx] = -vU;
			tangentData[idx].normalize();
		}
	}

	for(U32 j=0; j < offset; j++) {
		for(U32 i=0; i < _terrainWidth; i++) {
			U32 idx0 = COORD(i,	j,		_terrainWidth);
			U32 idx1 = COORD(i,	offset,	_terrainWidth);

			normalData[idx0] = normalData[idx1];
		    tangentData[idx0] = tangentData[idx1];

			idx0 = COORD(i,	_terrainHeight-1-j,		 _terrainWidth);
			idx1 = COORD(i,	_terrainHeight-1-offset, _terrainWidth);
	
			normalData[idx0] = normalData[idx1];
			tangentData[idx0] = tangentData[idx1];
		}

	}

	for(U32 i=0; i < offset; i++) {
		for(U32 j=0; j < _terrainHeight; j++) {
			U32 idx0 = COORD(i,		    j,	_terrainWidth);
			U32 idx1 = COORD(offset,	j,	_terrainWidth);

			normalData[idx0] = normalData[idx1];
			tangentData[idx0] = tangentData[idx1];

			idx0 = COORD(_terrainWidth-1-i,		j,	_terrainWidth);
			idx1 = COORD(_terrainWidth-1-offset,	j,	_terrainWidth);

			normalData[idx0] = normalData[idx1];
			tangentData[idx0] = tangentData[idx1];
		}
	}

	U32 chunkSize = 16;
	_terrainQuadtree->setParentShaderProgram(getMaterial()->getShaderProgram());
	_terrainQuadtree->Build(_boundingBox, vec2<U32>(_terrainWidth, _terrainHeight), chunkSize);

	
	ResourceDescriptor infinitePlane("infinitePlane");
	infinitePlane.setFlag(true); //No default material
	_plane = CreateResource<Quad3D>(infinitePlane);

	F32 depth = GET_ACTIVE_SCENE()->state()->getWaterDepth();
	F32 height = GET_ACTIVE_SCENE()->state()->getWaterLevel()- depth;
	_farPlane = 2.0f * ParamHandler::getInstance().getParam<F32>("runtime.zFar");
	_plane->setCorner(Quad3D::TOP_LEFT, vec3<F32>(   -_farPlane, height, -_farPlane));
	_plane->setCorner(Quad3D::TOP_RIGHT, vec3<F32>(   _farPlane, height, -_farPlane));
	_plane->setCorner(Quad3D::BOTTOM_LEFT, vec3<F32>(-_farPlane, height,  _farPlane));
	_plane->setCorner(Quad3D::BOTTOM_RIGHT, vec3<F32>(_farPlane, height,  _farPlane));
	_plane->getSceneNodeRenderState().setDrawState(false);

	return true;
}

void Terrain::postLoad(SceneGraphNode* const sgn){
	
	if(!isLoaded()) {
		sgn->setActive(false);
	}
	_node = sgn;

	_groundVBO->Create();
	_planeSGN = _node->addNode(_plane);
	_plane->computeBoundingBox(_planeSGN);
	_planeSGN->setActive(false);
	_planeTransform = _planeSGN->getTransform();
	computeBoundingBox(_node);
	ShaderProgram* s = getMaterial()->getShaderProgram();
	s->bind();
		s->Uniform("alphaTexture", _alphaTexturePresent);
		s->Uniform("detail_scale", 1000.0f);
		s->Uniform("diffuse_scale", 250.0f);
		s->Uniform("bbox_min", _boundingBox.getMin());
		s->Uniform("bbox_max", _boundingBox.getMax());
		s->Uniform("water_height", GET_ACTIVE_SCENE()->state()->getWaterLevel());
	s->unbind();
	
}

bool Terrain::computeBoundingBox(SceneGraphNode* const sgn){
	///The terrain's final bounding box is the QuadTree's root bounding box
	///Compute the QuadTree boundingboxes and get the root BB
	_boundingBox = _terrainQuadtree->computeBoundingBox(_groundVBO->getPosition());
	///Inform the scenegraph of our new BB
	sgn->getBoundingBox() = _boundingBox;
	return  SceneNode::computeBoundingBox(sgn);
}

void Terrain::initializeVegetation(TerrainDescriptor* const terrain,SceneGraphNode* const terrainSGN) {	
	vectorImpl<Texture2D*> grass;
	ResourceDescriptor grassBillboard1("Grass Billboard 1");
	grassBillboard1.setResourceLocation(terrain->getVariable("grassBillboard1"));
	ResourceDescriptor grassBillboard2("Grass Billboard 2");
	grassBillboard2.setResourceLocation(terrain->getVariable("grassBillboard2"));
	ResourceDescriptor grassBillboard3("Grass Billboard 3");
	grassBillboard3.setResourceLocation(terrain->getVariable("grassBillboard3"));
	//ResourceDescriptor grassBillboard4("Grass Billboard 4");
	//grassBillboard4.setResourceLocation(terrain->getVariable("grassBillboard4"));
	grass.push_back(CreateResource<Texture2D>(grassBillboard1));
	grass.push_back(CreateResource<Texture2D>(grassBillboard2));
	grass.push_back(CreateResource<Texture2D>(grassBillboard3));
	//grass.push_back(CreateResource<Texture2D>(grassBillboard4));
	addVegetation(New Vegetation(3, terrain->getGrassDensity(),
			                        terrain->getGrassScale(),
								    terrain->getTreeDensity(),
									terrain->getTreeScale(),
									terrain->getVariable("grassMap"),
									grass),
				  "grass");
	toggleVegetation(true);
	 _veg->initialize(_grassShader,this,terrainSGN);
}


void Terrain::addTexture(TerrainTextureUsage channel, Texture2D* const texture) {
	assert(texture != NULL);
	std::pair<TerrainTextureMap::iterator, bool > result;
	//Try and add it to the map
	result = _terrainTextures.insert(std::make_pair(channel,texture));
	//If we had a collision (same name?)
	if(!result.second){
		if((result.first)->second){
			UNREGISTER_TRACKED_DEPENDENCY(dynamic_cast<TrackedObject*>((result.first)->second));
			RemoveResource((result.first)->second); 
		}
		//And add this one instead
		(result.first)->second = texture;
	}
	REGISTER_TRACKED_DEPENDENCY(dynamic_cast<TrackedObject*>(texture));
}