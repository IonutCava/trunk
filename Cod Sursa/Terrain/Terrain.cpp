#include "Terrain.h"
#include "Managers/TextureManager.h"
#include "Importer/OBJ.h"
#include "Managers/ResourceManager.h"
#include "Managers/TerrainManager.h"
#include "Utility/Headers/Quadtree.h"
#include "Utility/Headers/QuadtreeNode.h"
#include "TerrainChunk.h"
#include "Hardware/Video/VertexBufferObject.h"
#include "Rendering/Camera.h"
#include "Sky.h"
#include <omp.h>
#include "Hardware/Video/GFXDevice.h"

Terrain::Terrain()
{
	Terrain(vec3(0,0,0), vec2(1,1));
}

Terrain::Terrain(vec3 pos, vec2 scale)
{
	terrainWidth = 0;
	terrainHeight = 0;
	
	m_pGroundVBO = NULL;
	terrainShader = GFXDevice::getInstance().newShader();
	terrainSetParameters(pos,scale);
	terrain_Quadtree = New Quadtree();
	m_fboDepthMapFromLight[0] = GFXDevice::getInstance().newFBO();
	m_fboDepthMapFromLight[1] = GFXDevice::getInstance().newFBO();
	_loaded = false;
}

void Terrain::destroy()
{
	terrainWidth = 0;
	terrainHeight = 0;

	if(terrain_Quadtree) {
		terrain_Quadtree->Destroy();
		delete terrain_Quadtree;
		terrain_Quadtree = NULL;
	}
	if(m_pGroundVBO) {
		m_pGroundVBO->Destroy();
		delete m_pGroundVBO;
		m_pGroundVBO = NULL;		
	}
}

void Terrain::terrainSetParameters(vec3& pos, vec2& scale)
{
	if(pos.y == 0) pos.y = 0.01f;
	terrainHeightScaleFactor = scale.y;
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

	terrain_BBox.min = vec3(-pos.x-300, pos.y, -pos.z-300);
	terrain_BBox.max = vec3( pos.x+300, pos.y+40, pos.z+300);

	terrain_BBox.Multiply(vec3(terrainScaleFactor,1,terrainScaleFactor));
	terrain_BBox.MultiplyMax(vec3(1,terrainHeightScaleFactor,1));
}

bool Terrain::load(const string& heightmap)
{
	U32 chunkSize = 16;
	std::cout << "Loading Terrain..." << std::endl;

	m_pGroundVBO = GFXDevice::getInstance().newVBO();

	std::vector<vec3>&		tPosition	= m_pGroundVBO->getPosition();
	std::vector<vec3>&		tNormal		= m_pGroundVBO->getNormal();
	std::vector<vec3>&		tTangent	= m_pGroundVBO->getTangent();

	U32 d;
	GLubyte* data = ImageTools::OpenImage(heightmap, terrainWidth, terrainHeight, d,true);
	assert(data!=NULL);

	U32 nIMGWidth  = terrainWidth;
	U32 nIMGHeight = terrainHeight;

	if(terrainWidth%2==0)	terrainWidth++;
	if(terrainHeight%2==0)	terrainHeight++;

	tPosition.resize(terrainWidth*terrainHeight);
	tNormal.resize(terrainWidth*terrainHeight);
	tTangent.resize(terrainWidth*terrainHeight);

	for(U32 j=0; j < terrainHeight; j++) {
		for(U32 i=0; i < terrainWidth; i++) {

			U32 idxHM = COORD(i,j,terrainWidth);
			tPosition[idxHM].x = terrain_BBox.min.x + ((F32)i) * 
				                (terrain_BBox.max.x - terrain_BBox.min.x)/(terrainWidth-1);
			tPosition[idxHM].z = terrain_BBox.min.z + ((F32)j) * 
				                (terrain_BBox.max.z - terrain_BBox.min.z)/(terrainHeight-1);
			U32 idxIMG = COORD(	i<nIMGWidth? i : i-1,
								j<nIMGHeight? j : j-1,
								nIMGWidth);

			F32 h = (F32)(data[idxIMG*d + 0] + data[idxIMG*d + 1] + data[idxIMG*d + 2])/3;

			tPosition[idxHM].y = (terrain_BBox.min.y + ((F32)h) * 
				                 (terrain_BBox.max.y - terrain_BBox.min.y)/(255)) * terrainHeightScaleFactor;

		}
	}
	delete[] data;
	U32 offset = 2;
	

	for(U32 j=offset; j < terrainHeight-offset; j++) {
		for(U32 i=offset; i < terrainWidth-offset; i++) {

			U32 idx = COORD(i,j,terrainWidth);

			vec3 vU = tPosition[COORD(i+offset, j+0, terrainWidth)] - tPosition[COORD(i-offset, j+0, terrainWidth)];
			vec3 vV = tPosition[COORD(i+0, j+offset, terrainWidth)] - tPosition[COORD(i+0, j-offset, terrainWidth)];

			tNormal[idx].cross(vV, vU);
			tNormal[idx].normalize();
			tTangent[idx] = -vU;
			tTangent[idx].normalize();
		}
	}

	for(U32 j=0; j < offset; j++) {
		for(U32 i=0; i < terrainWidth; i++) {
			U32 idx0 = COORD(i,	j,		terrainWidth);
			U32 idx1 = COORD(i,	offset,	terrainWidth);

			tNormal[idx0] = tNormal[idx1];
			tTangent[idx0] = tTangent[idx1];

			idx0 = COORD(i,	terrainHeight-1-j,		terrainWidth);
			idx1 = COORD(i,	terrainHeight-1-offset,	terrainWidth);

			tNormal[idx0] = tNormal[idx1];
			tTangent[idx0] = tTangent[idx1];
		}

	}

	for(U32 i=0; i < offset; i++) {
		for(U32 j=0; j < terrainHeight; j++) {
			U32 idx0 = COORD(i,		    j,	terrainWidth);
			U32 idx1 = COORD(offset,	j,	terrainWidth);

			tNormal[idx0] = tNormal[idx1];
			tTangent[idx0] = tTangent[idx1];

			idx0 = COORD(terrainWidth-1-i,		j,	terrainWidth);
			idx1 = COORD(terrainWidth-1-offset,	j,	terrainWidth);

			tNormal[idx0] = tNormal[idx1];
			tTangent[idx0] = tTangent[idx1];
		}
	}
	//m_pGroundVBO->Create(GL_STATIC_DRAW);

	terrain_Quadtree->Build(&terrain_BBox, ivec2(terrainWidth, terrainHeight), chunkSize);

	return true;
	
}

bool Terrain::postLoad()
{
	_postLoaded = m_pGroundVBO->Create(GL_STATIC_DRAW);
	cout << "Generating lightmap!" << endl;
	_postLoaded = genLightMap();
	cout << "Loading Terrain OK" << endl;
	if(!_postLoaded) _loaded = false;
	return _postLoaded;
}

bool Terrain::computeBoundingBox()
{
	terrain_Quadtree->ComputeBoundingBox( &(m_pGroundVBO->getPosition()[0]) );
	vec3 fogColor(0.7f, 0.7f, 0.9f);
	terrainShader->bind();
		terrainShader->Uniform("bbox_min", terrain_BBox.min);
		terrainShader->Uniform("bbox_max", terrain_BBox.max);
		terrainShader->Uniform("fog_color", fogColor);
	terrainShader->unbind();
	m_fboDepthMapFromLight[0]->Create(FrameBufferObject::FBO_2D_DEPTH, 2048, 2048);
	m_fboDepthMapFromLight[1]->Create(FrameBufferObject::FBO_2D_DEPTH, 2048, 2048);

	return true;
}


vec3 Terrain::getPosition(F32 x_clampf, F32 z_clampf) const
{
	if(x_clampf<.0f || z_clampf<.0f || x_clampf>1.0f || z_clampf>1.0f) return vec3(0.0f, 0.0f, 0.0f);

	vec2  posF(	x_clampf * terrainWidth, z_clampf * terrainHeight );
	ivec2 posI(	(int)(posF.x), (int)(posF.y) );
	vec2  posD(	posF.x - posI.x, posF.y - posI.y );

	if(posI.x >= (int)terrainWidth-1)		posI.x = terrainWidth-2;
	if(posI.y >= (int)terrainHeight-1)	posI.y = terrainHeight-2;
	assert(posI.x>=0 && posI.x<(int)terrainWidth-1 && posI.y>=0 && posI.y<(int)terrainHeight-1);

	vec3 pos(terrain_BBox.min.x + x_clampf * (terrain_BBox.max.x - terrain_BBox.min.x), 0.0f, terrain_BBox.min.z + z_clampf * (terrain_BBox.max.z - terrain_BBox.min.z));
	pos.y =   (m_pGroundVBO->getPosition()[ COORD(posI.x,  posI.y,  terrainWidth) ].y)  * (1.0f-posD.x) * (1.0f-posD.y)
			+ (m_pGroundVBO->getPosition()[ COORD(posI.x+1,posI.y,  terrainWidth) ].y)  *       posD.x  * (1.0f-posD.y)
			+ (m_pGroundVBO->getPosition()[ COORD(posI.x,  posI.y+1,terrainWidth) ].y)  * (1.0f-posD.x) *       posD.y
			+ (m_pGroundVBO->getPosition()[ COORD(posI.x+1,posI.y+1,terrainWidth) ].y)  *       posD.x  *       posD.y;
	return pos;
}

vec3 Terrain::getNormal(F32 x_clampf, F32 z_clampf) const
{
	if(x_clampf<.0f || z_clampf<.0f || x_clampf>1.0f || z_clampf>1.0f) return vec3(0.0f, 1.0f, 0.0f);

	vec2  posF(	x_clampf * terrainWidth, z_clampf * terrainHeight );
	ivec2 posI(	(int)(x_clampf * terrainWidth), (int)(z_clampf * terrainHeight) );
	vec2  posD(	posF.x - posI.x, posF.y - posI.y );

	if(posI.x >= (int)terrainWidth-1)		posI.x = terrainWidth-2;
	if(posI.y >= (int)terrainHeight-1)	posI.y = terrainHeight-2;
	assert(posI.x>=0 && posI.x<(int)terrainWidth-1 && posI.y>=0 && posI.y<(int)terrainHeight-1);

	return    (m_pGroundVBO->getNormal()[ COORD(posI.x,  posI.y,  terrainWidth) ])  * (1.0f-posD.x) * (1.0f-posD.y)
			+ (m_pGroundVBO->getNormal()[ COORD(posI.x+1,posI.y,  terrainWidth) ])  *       posD.x  * (1.0f-posD.y)
			+ (m_pGroundVBO->getNormal()[ COORD(posI.x,  posI.y+1,terrainWidth) ])  * (1.0f-posD.x) *       posD.y
			+ (m_pGroundVBO->getNormal()[ COORD(posI.x+1,posI.y+1,terrainWidth) ])  *       posD.x  *       posD.y;
}

vec3 Terrain::getTangent(F32 x_clampf, F32 z_clampf) const
{
	if(x_clampf<.0f || z_clampf<.0f || x_clampf>1.0f || z_clampf>1.0f) return vec3(1.0f, 0.0f, 0.0f);

	vec2  posF(	x_clampf * terrainWidth, z_clampf * terrainHeight );
	ivec2 posI(	(int)(x_clampf * terrainWidth), (int)(z_clampf * terrainHeight) );
	vec2  posD(	posF.x - posI.x, posF.y - posI.y );

	if(posI.x >= (int)terrainWidth-1)		posI.x = terrainWidth-2;
	if(posI.y >= (int)terrainHeight-1)	posI.y = terrainHeight-2;
	assert(posI.x>=0 && posI.x<(int)terrainWidth-1 && posI.y>=0 && posI.y<(int)terrainHeight-1);

	return    (m_pGroundVBO->getTangent()[ COORD(posI.x,  posI.y,  terrainWidth) ])  * (1.0f-posD.x) * (1.0f-posD.y)
			+ (m_pGroundVBO->getTangent()[ COORD(posI.x+1,posI.y,  terrainWidth) ])  *       posD.x  * (1.0f-posD.y)
			+ (m_pGroundVBO->getTangent()[ COORD(posI.x,  posI.y+1,terrainWidth) ])  * (1.0f-posD.x) *       posD.y
			+ (m_pGroundVBO->getTangent()[ COORD(posI.x+1,posI.y+1,terrainWidth) ])  *       posD.x  *       posD.y;
}



int Terrain::drawObjects() const
{
	glPushAttrib(GL_POLYGON_BIT);
	glDisable(GL_CULL_FACE);
		int ret = terrain_Quadtree->DrawObjects(_drawInReflexion);
	glPopAttrib();

	return ret;
}

void Terrain::draw() const
{
	//if(currentTime - lastTime > diffTime)
	//	_lightmap = genLightMap();

	if(!_loaded) return;
	GFXDevice::getInstance().enable_TEXTURE(0);
	GFXDevice::getInstance().enable_MODELVIEW();
		GLuint idx=0;
		for(GLuint i=0; i<m_tTextures.size(); i++)
			m_tTextures[i]->Bind(idx++);
		m_pTerrainDiffuseMap->Bind(idx++);
		//_lightmap->Bind(idx++);

		terrainShader->bind();
		{
			terrainShader->Uniform("detail_scale", 120.0f);
			terrainShader->Uniform("diffuse_scale", 70.0f);

			terrainShader->Uniform("water_height", 1);
			terrainShader->Uniform("water_reflection_rendering", false);

			terrainShader->Uniform("time", GETTIME());

			terrainShader->UniformTexture("texNormalHeightMap", 0);
			terrainShader->UniformTexture("texDiffuse0", 1);
			terrainShader->UniformTexture("texDiffuse1", 2);
			terrainShader->UniformTexture("texDiffuse2", 3);
			terrainShader->UniformTexture("texWaterCaustics", 4);
			terrainShader->UniformTexture("texDiffuseMap", 5);
			//terrainShader->UniformTexture("shadowMap",6);

			terrainShader->Uniform("depth_map_size", 512);
			drawGround(_drawInReflexion);
		}
		terrainShader->unbind();

		//_lightmap->Unbind(--idx);
		m_pTerrainDiffuseMap->Unbind(--idx);
		for(int i=(GLint)m_tTextures.size()-1; i>=0; i--)
			m_tTextures[i]->Unbind(--idx);

		_veg->draw(_drawInReflexion);

}



int Terrain::drawGround(bool drawInReflexion) const
{
	assert(m_pGroundVBO);

	m_pGroundVBO->Enable();
		int ret = terrain_Quadtree->DrawGround(drawInReflexion);
	m_pGroundVBO->Disable();

	return ret;
}



bool Terrain::genLightMap()
{
	_lightmap = new Texture2D();
	int size = terrainWidth;
	UBYTE *lightmap = (UBYTE*)malloc(size*size*sizeof(UBYTE));
	
	std::vector<vec3>&		tPosition	= m_pGroundVBO->getPosition();
	F32 lightDir[3];
	lightDir[0] = Sky::getInstance().getSunVector().x;
	lightDir[1] = Sky::getInstance().getSunVector().y;
	lightDir[2] = Sky::getInstance().getSunVector().z;
	// create flag buffer to indicate where we've been
	F32 *flagMap = new F32[size*size];
	for(int i = 0; i < size*size; i++) 
		flagMap[i] = 0;
 
 
	// calculate absolute values for light direction
	F32 lightDirXMagnitude = lightDir[0];
	F32 lightDirZMagnitude = lightDir[2];
	if(lightDirXMagnitude < 0) lightDirXMagnitude *= -1;
	if(lightDirZMagnitude < 0) lightDirZMagnitude *= -1;
 
	F32 distanceStep = sqrtf(lightDir[0] * lightDir[0] + lightDir[1] * lightDir[1]);
 
	// decide which loop will come first, the y loop or x loop
	// based on direction of light, makes calculations faster
 
	// outer loop
	//

	//#pragma omp parallel for firstprivate(distanceStep, lightDirXMagnitude, \
	//      lightDirZMagnitude, lightmap, flagMap, size, lightDir) schedule(static,4)
		int *X, *Y;
		int iX, iY;
		int dirX, dirY;
 
		// this might seem like a waste, why calculate it here? you can calculate it before... 
		// that's because threading is really picky about sharing variables. the less you share, 
		// the faster it goes.
		if(lightDirXMagnitude > lightDirZMagnitude)
		{
			Y = &iX;
			X = &iY;
 
			if(lightDir[0] < 0)
				dirY = -1;
			else
				dirY = 1;
 
			if(lightDir[2] < 0)
				dirX = -1;
			else
				dirX = 1;
		}
		else
		{
			Y = &iY;
			X = &iX;
 
			if(lightDir[0] < 0)
				dirX = -1;
			else
				dirX = 1;
 
			if(lightDir[2] < 0)
				dirY = -1;
			else
				dirY = 1;
		}
	for(int y=0; y<size; y++)
	{

        // if you decide to just do it single-threaded, 
		// just copy the previous block back just above the for loop 
 
		if(dirY < 0)
			iY = size - y - 1;
		else
			iY = y;
 
		// inner loop
		for(int x=0; x<size; x++)
		{
			continue;
			if(dirX < 0)
				iX = size - x - 1;
			else
				iX = x;
 
			F32 px, py, height, distance, origX, origY;
			int index;
 
			// travel along the terrain until we:
			// (1) intersect another point
			// (2) find another point with previous collision data
			// (3) or reach the edge of the map
			px = (F32)*X;
			py = (F32)*Y;
			origX = px;
			origY = py;
			index = (*Y) * size + (*X);
			distance = 0.0f;
 
			// travel along ray
			while(1)
			{
				px -= lightDir[0];
				py -= lightDir[2];
 
				// check if we've reached the boundary
				if(px < 0 || px >= size || py < 0 || py >= size)
				{  
					flagMap[index] = -1;
					break;
				}
 
				// calculate interpolated values
				int x0, x1, y0, y1;
				F32 du, dv;
				F32 interpolatedHeight, interpolatedFlagMap;
				//F32 heights[4];
				//F32 pixels[4];
				F32 invdu, invdv;
				F32 w0, w1, w2, w3;
 
				x0 = int(px);
				//x1 = ceilf(px);
				y0 = int(py);
				//y1 = ceilf(py);
 
				x1 = x0 + 1;
				y1 = y0 + 1;
 
				du = px - x0;
				dv = py - y0;
 
				invdu = 1.0f - du;
				invdv = 1.0f - dv;
				w0 = invdu * invdv;
				w1 = invdu * dv;
				w2 = du * invdv;
				w3 = du * dv;
 
				// compute interpolated height value from the heightmap direction below ray
				interpolatedHeight = w0*tPosition[COORD(x0,	y0,	size)].y + w1*tPosition[COORD(x0,	y1,	size)].y + 
				    w2*tPosition[COORD(x1,	y0,	size)].y + w3*tPosition[COORD(x1,	y1,	size)].y;
 
				// compute interpolated flagmap value from point directly below ray
				interpolatedFlagMap = w0*flagMap[y0*size+x0] + w1*flagMap[y1*size+x0] + 
				    w2*flagMap[y0*size+x1] + w3*flagMap[y1*size+x1];
 
				// get distance from original point to current point
				//distance = sqrtf( (px-origX)*(px-origX) + (py-origY)*(py-origY) );
				distance += distanceStep;
 
				// get height at current point while traveling along light ray
				height = tPosition[index].y + lightDir[1]*distance;
 
				// check intersection with either terrain or flagMap
				// if interpolatedHeight is less than interpolatedFlagMap that means 
				// we need to use the flagMap value instead
				// else use the height value
				F32 val;
				if(interpolatedHeight < interpolatedFlagMap) val = interpolatedFlagMap;
				else val = interpolatedHeight;
 
				if(height < val)
				{
					flagMap[index] = val - height;
 
					lightmap[index] = 0;
 
					break;
				}
 
				// check if pixel we've moved to is unshadowed
				// since the flagMap value we're using is interpolated, we will be in 
				// between shadowed and unshadowed areas
				// to compensate for this, simply define some epsilon value and use 
				// this as an offset from -1 to decide
				// if current point under the ray is unshadowed
				static F32 epsilon = 0.5f;
				if(interpolatedFlagMap < -1.0f+epsilon && interpolatedFlagMap > -1.0f-epsilon)
				{
					flagMap[index] = -1.0f;
					break;
				}   
			}
		}
	}
 
	delete [] flagMap;
	//_lightmap->load(lightmap,terrainWidth,terrainHeight,3);
	return true;
}

