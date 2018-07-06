#include "Terrain.h"
#include "Managers/TextureManager.h"
#include "Managers/ResourceManager.h"
#include "Utility/Headers/ParamHandler.h"
#include "Managers/TerrainManager.h"
#include "Quadtree.h"
#include "QuadtreeNode.h"
#include "TerrainChunk.h"
#include "Hardware/Video/VertexBufferObject.h"
#include "Managers/CameraManager.h"
#include "Sky.h"
#include "Hardware/Video/GFXDevice.h"

#define COORD(x,y,w)	((y)*(w)+(x))

Terrain::Terrain()
{
	Terrain(vec3(0,0,0), vec2(1,1));
}

Terrain::Terrain(vec3 pos, vec2 scale)
{
	_terrainWidth = 0;
	_terrainHeight = 0;
	
	_groundVBO = NULL;
	terrainShader = GFXDevice::getInstance().newShader();
	terrainSetParameters(pos,scale);
	_terrainQuadtree = New Quadtree();
	_depthMapFBO[0] = GFXDevice::getInstance().newFBO();
	_depthMapFBO[1] = GFXDevice::getInstance().newFBO();
	_loaded = false;
}

void Terrain::Destroy()
{
	_terrainWidth = 0;
	_terrainHeight = 0;

	if(_terrainQuadtree) {
		delete _terrainQuadtree;
		_terrainQuadtree = NULL;
	}

	if(_groundVBO) {
		delete _groundVBO;
		_groundVBO = NULL;		
	}
}

void Terrain::terrainSetParameters(const vec3& pos,const vec2& scale)
{
	_terrainHeightScaleFactor = scale.y;
	terrainScaleFactor = scale.x;
	//Terrain dimensions:
	//    |----------------------|        /\                        /\
	//    |          /\          |         |                       /  \
 	//    |          |           |         |                  /\__/    \
	//    |          |           |     40*scale         /\   /          \__/\___
	//    |< ---- 600 * scale -->|         |           |  --/                   \
	//    |          |           |         |          /                          \
	//    |          |           |         |        |-                            \
	//    |          |           |         \/      /_______________________________\
	//    |_________\/___________|

	_terrainBBox.setMin(vec3(-pos.x-300, pos.y, -pos.z-300));
	_terrainBBox.setMax(vec3( pos.x+300, pos.y+40, pos.z+300));

	_terrainBBox.Multiply(vec3(terrainScaleFactor,1,terrainScaleFactor));
	_terrainBBox.MultiplyMax(vec3(1,_terrainHeightScaleFactor,1));
}

bool Terrain::load(const string& heightmap)
{
	U32 chunkSize = 16;
	Con::getInstance().printfn("Loading Terrain...");

	_groundVBO = GFXDevice::getInstance().newVBO();

	std::vector<vec3>&		tPosition	= _groundVBO->getPosition();
	std::vector<vec3>&		tNormal		= _groundVBO->getNormal();
	std::vector<vec3>&		tTangent	= _groundVBO->getTangent();

	U8 d;
	U32 t;
	U8* data = ImageTools::OpenImage(heightmap, _terrainWidth, _terrainHeight, d, t);
	Con::getInstance().printfn("Terrain width: %d and height: %d",_terrainWidth, _terrainHeight);
	assert(data!=NULL);

	U32 nIMGWidth  = _terrainWidth;
	U32 nIMGHeight = _terrainHeight;

	if(_terrainWidth%2==0)	_terrainWidth++;
	if(_terrainHeight%2==0)	_terrainHeight++;

	tPosition.resize(_terrainWidth*_terrainHeight);
	tNormal.resize(_terrainWidth*_terrainHeight);
	tTangent.resize(_terrainWidth*_terrainHeight);

	for(U32 j=0; j < _terrainHeight; j++) {
		for(U32 i=0; i < _terrainWidth; i++) {

			U32 idxHM = COORD(i,j,_terrainWidth);
			tPosition[idxHM].x = _terrainBBox.getMin().x + ((F32)i) * 
				                (_terrainBBox.getMax().x - _terrainBBox.getMin().x)/(_terrainWidth-1);
			tPosition[idxHM].z = _terrainBBox.getMin().z + ((F32)j) * 
				                (_terrainBBox.getMax().z - _terrainBBox.getMin().z)/(_terrainHeight-1);
			U32 idxIMG = COORD(	i<nIMGWidth? i : i-1,
								j<nIMGHeight? j : j-1,
								nIMGWidth);

			F32 h = (F32)(data[idxIMG*d + 0] + data[idxIMG*d + 1] + data[idxIMG*d + 2])/3;

			tPosition[idxHM].y = (_terrainBBox.getMin().y + ((F32)h) * 
				                 (_terrainBBox.getMax().y - _terrainBBox.getMin().y)/(255)) * _terrainHeightScaleFactor;

		}
	}
	delete[] data;
	U32 offset = 2;

	for(U32 j=offset; j < _terrainHeight-offset; j++) {
		for(U32 i=offset; i < _terrainWidth-offset; i++) {

			U32 idx = COORD(i,j,_terrainWidth);

			vec3 vU = tPosition[COORD(i+offset, j+0, _terrainWidth)] - tPosition[COORD(i-offset, j+0, _terrainWidth)];
			vec3 vV = tPosition[COORD(i+0, j+offset, _terrainWidth)] - tPosition[COORD(i+0, j-offset, _terrainWidth)];

			tNormal[idx].cross(vV, vU);
			tNormal[idx].normalize();
			tTangent[idx] = -vU;
			tTangent[idx].normalize();
		}
	}

	for(U32 j=0; j < offset; j++) {
		for(U32 i=0; i < _terrainWidth; i++) {
			U32 idx0 = COORD(i,	j,		_terrainWidth);
			U32 idx1 = COORD(i,	offset,	_terrainWidth);

			tNormal[idx0] = tNormal[idx1];
			tTangent[idx0] = tTangent[idx1];

			idx0 = COORD(i,	_terrainHeight-1-j,		_terrainWidth);
			idx1 = COORD(i,	_terrainHeight-1-offset,	_terrainWidth);

			tNormal[idx0] = tNormal[idx1];
			tTangent[idx0] = tTangent[idx1];
		}

	}

	for(U32 i=0; i < offset; i++) {
		for(U32 j=0; j < _terrainHeight; j++) {
			U32 idx0 = COORD(i,		    j,	_terrainWidth);
			U32 idx1 = COORD(offset,	j,	_terrainWidth);

			tNormal[idx0] = tNormal[idx1];
			tTangent[idx0] = tTangent[idx1];

			idx0 = COORD(_terrainWidth-1-i,		j,	_terrainWidth);
			idx1 = COORD(_terrainWidth-1-offset,	j,	_terrainWidth);

			tNormal[idx0] = tNormal[idx1];
			tTangent[idx0] = tTangent[idx1];
		}
	}

	_terrainQuadtree->Build(&_terrainBBox, ivec2(_terrainWidth, _terrainHeight), chunkSize);

	return true;
	
}

bool Terrain::postLoad()
{
	_postLoaded = _groundVBO->Create();
	Con::getInstance().printfn("Loading Terrain OK");
	if(!_postLoaded) _loaded = false;
	return _postLoaded;
}

bool Terrain::computeBoundingBox()
{
	_terrainQuadtree->ComputeBoundingBox(_groundVBO->getPosition());
	return true;
}


vec3 Terrain::getPosition(F32 x_clampf, F32 z_clampf) const
{
	if(x_clampf<.0f || z_clampf<.0f || x_clampf>1.0f || z_clampf>1.0f) return vec3(0.0f, 0.0f, 0.0f);

	vec2  posF(	x_clampf * _terrainWidth, z_clampf * _terrainHeight );
	ivec2 posI(	(I32)(posF.x), (I32)(posF.y) );
	vec2  posD(	posF.x - posI.x, posF.y - posI.y );

	if(posI.x >= (I32)_terrainWidth-1)		posI.x = _terrainWidth-2;
	if(posI.y >= (I32)_terrainHeight-1)	posI.y = _terrainHeight-2;
	assert(posI.x>=0 && posI.x<(I32)_terrainWidth-1 && posI.y>=0 && posI.y<(I32)_terrainHeight-1);

	vec3 pos(_terrainBBox.getMin().x + x_clampf * (_terrainBBox.getMax().x - _terrainBBox.getMin().x), 0.0f, _terrainBBox.getMin().z + z_clampf * (_terrainBBox.getMax().z - _terrainBBox.getMin().z));
	pos.y =   (_groundVBO->getPosition()[ COORD(posI.x,  posI.y,  _terrainWidth) ].y)  * (1.0f-posD.x) * (1.0f-posD.y)
			+ (_groundVBO->getPosition()[ COORD(posI.x+1,posI.y,  _terrainWidth) ].y)  *       posD.x  * (1.0f-posD.y)
			+ (_groundVBO->getPosition()[ COORD(posI.x,  posI.y+1,_terrainWidth) ].y)  * (1.0f-posD.x) *       posD.y
			+ (_groundVBO->getPosition()[ COORD(posI.x+1,posI.y+1,_terrainWidth) ].y)  *       posD.x  *       posD.y;
	return pos;
}

vec3 Terrain::getNormal(F32 x_clampf, F32 z_clampf) const
{
	if(x_clampf<.0f || z_clampf<.0f || x_clampf>1.0f || z_clampf>1.0f) return vec3(0.0f, 1.0f, 0.0f);

	vec2  posF(	x_clampf * _terrainWidth, z_clampf * _terrainHeight );
	ivec2 posI(	(I32)(x_clampf * _terrainWidth), (I32)(z_clampf * _terrainHeight) );
	vec2  posD(	posF.x - posI.x, posF.y - posI.y );

	if(posI.x >= (I32)_terrainWidth-1)		posI.x = _terrainWidth-2;
	if(posI.y >= (I32)_terrainHeight-1)	posI.y = _terrainHeight-2;
	assert(posI.x>=0 && posI.x<(I32)_terrainWidth-1 && posI.y>=0 && posI.y<(I32)_terrainHeight-1);

	return    (_groundVBO->getNormal()[ COORD(posI.x,  posI.y,  _terrainWidth) ])  * (1.0f-posD.x) * (1.0f-posD.y)
			+ (_groundVBO->getNormal()[ COORD(posI.x+1,posI.y,  _terrainWidth) ])  *       posD.x  * (1.0f-posD.y)
			+ (_groundVBO->getNormal()[ COORD(posI.x,  posI.y+1,_terrainWidth) ])  * (1.0f-posD.x) *       posD.y
			+ (_groundVBO->getNormal()[ COORD(posI.x+1,posI.y+1,_terrainWidth) ])  *       posD.x  *       posD.y;
}

vec3 Terrain::getTangent(F32 x_clampf, F32 z_clampf) const
{
	if(x_clampf<.0f || z_clampf<.0f || x_clampf>1.0f || z_clampf>1.0f) return vec3(1.0f, 0.0f, 0.0f);

	vec2  posF(	x_clampf * _terrainWidth, z_clampf * _terrainHeight );
	ivec2 posI(	(I32)(x_clampf * _terrainWidth), (I32)(z_clampf * _terrainHeight) );
	vec2  posD(	posF.x - posI.x, posF.y - posI.y );

	if(posI.x >= (I32)_terrainWidth-1)		posI.x = _terrainWidth-2;
	if(posI.y >= (I32)_terrainHeight-1)	posI.y = _terrainHeight-2;
	assert(posI.x>=0 && posI.x<(I32)_terrainWidth-1 && posI.y>=0 && posI.y<(I32)_terrainHeight-1);

	return    (_groundVBO->getTangent()[ COORD(posI.x,  posI.y,  _terrainWidth) ])  * (1.0f-posD.x) * (1.0f-posD.y)
			+ (_groundVBO->getTangent()[ COORD(posI.x+1,posI.y,  _terrainWidth) ])  *       posD.x  * (1.0f-posD.y)
			+ (_groundVBO->getTangent()[ COORD(posI.x,  posI.y+1,_terrainWidth) ])  * (1.0f-posD.x) *       posD.y
			+ (_groundVBO->getTangent()[ COORD(posI.x+1,posI.y+1,_terrainWidth) ])  *       posD.x  *       posD.y;
}



void Terrain::draw() const
{
	if(!_loaded) return;

	_veg->draw(_drawInReflexion);

    if(GFXDevice::getInstance().getDepthMapRendering()) return;

	Material mat;
	mat.name = "TerrainMaterial";
	mat.ambient = _ambientColor;
	mat.diffuse = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	mat.specular = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	mat.shininess = 50.0f;
	GFXDevice::getInstance().setMaterial(mat);

	RenderState old = GFXDevice::getInstance().getActiveRenderState();
	RenderState s(true,false,true,true);
	

	_terrainTextures[0]->SetMatrix(0,_sunModelviewProj); //Add sun projection matrix to the normal map for parallax rendering

		U8 idx=0;
		for(U8 i=0; i<_terrainTextures.size(); i++)
			_terrainTextures[i]->Bind(idx++);
		_terrainDiffuseMap->Bind(idx++);
		if(!GFXDevice::getInstance().getDepthMapRendering()) {
			for(U8 i=0; i<2; i++)
				_depthMapFBO[i]->Bind(idx++);
		}
		terrainShader->bind();
		{
			terrainShader->Uniform("detail_scale", 200.0f);
			terrainShader->Uniform("diffuse_scale", 200.0f);
			terrainShader->Uniform("parallax_factor",0.2f);
			terrainShader->Uniform("water_height", ParamHandler::getInstance().getParam<F32>("minHeight"));
			terrainShader->Uniform("water_reflection_rendering", _drawInReflexion);
			terrainShader->Uniform("time", GETTIME());
			terrainShader->UniformTexture("texNormalHeightMap", 0);
			terrainShader->UniformTexture("texDiffuse0", 1);
			terrainShader->UniformTexture("texDiffuse1", 2);
			terrainShader->UniformTexture("texDiffuse2", 3);
			terrainShader->UniformTexture("texWaterCaustics", 4);
			terrainShader->UniformTexture("texDiffuseMap", 5);
			terrainShader->UniformTexture("texDepthMapFromLight0", 6);
			terrainShader->UniformTexture("texDepthMapFromLight1", 7);
			terrainShader->Uniform("bbox_min", _terrainBBox.getMin());
			terrainShader->Uniform("bbox_max", _terrainBBox.getMax());
			terrainShader->Uniform("fog_color", vec3(0.7f, 0.7f, 0.9f));
			terrainShader->Uniform("depth_map_size", 512);
			GFXDevice::getInstance().setRenderState(s);
			drawGround(_drawInReflexion);
			GFXDevice::getInstance().setRenderState(old);
		}
		terrainShader->unbind();

		if(!GFXDevice::getInstance().getDepthMapRendering()) {
			for(U8 i=0; i < 2; i++)
				_depthMapFBO[i]->Unbind(--idx);
		}
		_terrainDiffuseMap->Unbind(--idx);
		for(U8 i= 0; i < _terrainTextures.size()-1; i++)
			_terrainTextures[_terrainTextures.size() -1 - i]->Unbind(--idx);

		_terrainTextures[0]->RestoreMatrix(0);
}



void Terrain::drawGround(bool drawInReflexion) const
{
	assert(_groundVBO);

	_groundVBO->Enable();
		_terrainQuadtree->DrawGround(drawInReflexion);
	_groundVBO->Disable();
}

