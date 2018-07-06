#include "Terrain/Terrain.h"
#include "Vegetation.h"

#include "Utility/Headers/Guardian.h"
#include "Rendering/Frustum.h"
#include "Utility/Headers/Quadtree.h"
#include "Utility/Headers/QuadtreeNode.h"
#include "Terrain/TerrainChunk.h"
#include "Hardware/Video/VertexBufferObject.h"
#include "PhysX/PhysX.h"



void Vegetation::initialize(string grassShader, string treeShader)
{
	_grassShader  = ResourceManager::getInstance().LoadResource<Shader>(grassShader);
    _treeShader   = ResourceManager::getInstance().LoadResource<Shader>(treeShader+".vert");
	_grassDensity = _grassDensity/_billboardCount;

	
	for(U32 i = 0 ; i < _billboardCount; i++) _success = generateGrass(i);
	if(_success) _success = generateTrees();

	//_map.Destroy();
	_render = true;

}

void Vegetation::draw(bool drawInReflexion)
{
	if(!_render || !_success) return;

	glEnable(GL_BLEND);
	_grassShader->bind();
	for(int index = 0; index < _billboardCount; index++)
	{
			_grassBillboards[index]->Bind(0);
			_grassShader->UniformTexture("texDiffuse", 0);
			_grassShader->Uniform("depth_map_size", 1024);
			_grassShader->Uniform("time", GETTIME());
			_grassShader->Uniform("windDirectionX", TerrainManager::getInstance().getWindDirX());
			_grassShader->Uniform("windDirectionZ", TerrainManager::getInstance().getWindDirZ());
			_grassShader->Uniform("windSpeed", TerrainManager::getInstance().getWindSpeed());
			_grassShader->Uniform("lod_metric", 1000.0f);
			_grassShader->Uniform("scale", _grassScale);
				DrawGrass(index,drawInReflexion);
			_grassBillboards[index]->Unbind(0);
		
		
	}
	_grassShader->unbind();
	glDisable(GL_BLEND);

	_treeShader->bind();
	_treeShader->Uniform("time", GETTIME());
	_treeShader->Uniform("windDirectionX", TerrainManager::getInstance().getWindDirX());
	_treeShader->Uniform("windDirectionZ",TerrainManager::getInstance().getWindDirZ());
	_treeShader->Uniform("windSpeed", TerrainManager::getInstance().getWindSpeed());
	glPushAttrib(GL_ALL_ATTRIB_BITS);
		DrawTrees(drawInReflexion);
	glPopAttrib();
	_treeShader->unbind();
}



bool Vegetation::generateGrass(int index)
{
	std::cout << "Generating Grass...[" << _grassDensity << "]" << std::endl;
	assert(_map.data);
	vec2 pos0(cosf(RADIANS(0.0f)), sinf(RADIANS(0.0f)));
	vec2 pos120(cosf(RADIANS(120.0f)), sinf(RADIANS(120.0f)));
	vec2 pos240(cosf(RADIANS(240.0f)), sinf(RADIANS(240.0f)));

	vec3 tVertices[] = {
		vec3(-pos0.x,   -pos0.y,   0.0f),	vec3(-pos0.x,   -pos0.y,   1.0f),	vec3(pos0.x,   pos0.y,   1.0f),	vec3(pos0.x,   pos0.y,   0.0f),
		vec3(-pos120.x, -pos120.y, 0.0f),	vec3(-pos120.x, -pos120.y, 1.0f),	vec3(pos120.x, pos120.y, 1.0f),	vec3(pos120.x, pos120.y, 0.0f),
		vec3(-pos240.x, -pos240.y, 0.0f),	vec3(-pos240.x, -pos240.y, 1.0f),	vec3(pos240.x, pos240.y, 1.0f),	vec3(pos240.x, pos240.y, 0.0f)
	};

	vec2 tTexcoords[] = {
		vec2(0.0f, 0.49f), vec2(0.0f, 0.01f), vec2(1.0f, 0.01f), vec2(1.0f, 0.49f),
		vec2(0.0f, 0.49f), vec2(0.0f, 0.01f), vec2(1.0f, 0.01f), vec2(1.0f, 0.49f),
		vec2(0.0f, 0.49f), vec2(0.0f, 0.01f), vec2(1.0f, 0.01f), vec2(1.0f, 0.49f)
	};

	_grassVBO.push_back(GFXDevice::getInstance().newVBO());

	U32 size = (U32) ceil(_grassDensity) * 3 * 4;
	_grassVBO[index]->getPosition().reserve( size );
	_grassVBO[index]->getNormal().reserve( size );
	_grassVBO[index]->getTexcoord().reserve( size );
	for(U32 k=0; k<(U32)_grassDensity; k++) {
		F32 x = random(1.0f);
		F32 y = random(1.0f);
		int map_x = (int)(x * _map.w);
		int map_y = (int)(y * _map.h);
		vec2 uv_offset = vec2(0.0f, random(3)==0 ? 0.0f : 0.5f);
		F32 size = random(0.5f);
		ivec3 map_color = _map.getColor(map_x, map_y);
		if(map_color.green < 150) {
			k--;
			continue;
		}

		size = (F32)(map_color.green+1) / (256 / _grassScale);
		vec3 P = _terrain.getPosition(x, y);
		vec3 N = _terrain.getNormal(x, y);
		vec3 T = _terrain.getTangent(x, y);
		vec3 B = Cross(N, T);

		if(N.y < 0.8f) {
			k--;
			continue;
		} else {
			mat3 matRot;
			matRot.rotate_z(random(360.0f));

			U32 idx = (GLuint)_grassVBO[index]->getPosition().size();

			QuadtreeNode* node = _terrain.getQuadtree().FindLeaf(vec2(P.x, P.z));
			assert(node);
			TerrainChunk* chunk = node->getChunk();
			assert(chunk);

			for(int i=0; i<3*4; i++)
			{
				vec3 data = matRot*(tVertices[i]*size);
				vec3 vertex = P;
				vertex.x += Dot(data, T);
				vertex.y += Dot(data, B);
				vertex.z += Dot(data, N);
				
				//vertex.y -= 1/_grassScale;
				_grassVBO[index]->getPosition().push_back( vertex );
				_grassVBO[index]->getNormal().push_back( tTexcoords[i].t < 0.2f ? -N : N );
				_grassVBO[index]->getTexcoord().push_back( uv_offset + tTexcoords[i] );
				chunk->getGrassIndiceArray().push_back(idx+i);
			}

		}
	}

	bool ret = _grassVBO[index]->Create(GL_STATIC_DRAW);

	std::cout << "Generating Grass OK" << std::endl;
	return ret;
}

bool Vegetation::generateTrees()
{
	std::cout << "Generating Vegetation... [" << _treeDensity << "]" << std::endl;
	
	for(int k=0; k<(int)_treeDensity; k++) {
		F32 x = random(1.0f);
		F32 y = random(1.0f);
		int map_x = (int)(x * _map.w);
		int map_y = (int)(y * _map.h);
		ivec3 map_color = _map.getColor(map_x, map_y);
		if(map_color.green < 25) {
			k--;
			continue;
		}

		vec3 P = _terrain.getPosition(x, y);

		QuadtreeNode* node = _terrain.getQuadtree().FindLeaf(vec2(P.x, P.z));
		assert(node);
		TerrainChunk* chunk = node->getChunk();
		assert(chunk);
		P.y -= 0.2f;
		chunk->addTree(P, random(360.0f),random(_treeScale - 0.5f , _treeScale));


	}
	std::cout << "Generating Vegetation OK" << std::endl;
	return true;
}

void Vegetation::DrawTrees(bool drawInReflexion)
{
	glPushAttrib(GL_POLYGON_BIT);
	glDisable(GL_CULL_FACE);
		_terrain.getQuadtree().DrawTrees(drawInReflexion);
	glPopAttrib();
}

void Vegetation::DrawGrass(int index,bool drawInReflexion)
{
	if(_grassVBO[index])
	{
		glPushAttrib(GL_POLYGON_BIT);
		glDisable(GL_CULL_FACE);
		_grassVBO[index]->Enable();
			_terrain.getQuadtree().DrawGrass(drawInReflexion);
		_grassVBO[index]->Disable();
		glPopAttrib();
	}
}

