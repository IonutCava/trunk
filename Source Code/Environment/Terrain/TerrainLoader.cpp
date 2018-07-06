#include "Headers/Terrain.h"
#include "Headers/TerrainDescriptor.h"

#include "Core/Headers/ParamHandler.h"
#include "Quadtree/Headers/Quadtree.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Hardware/Video/Buffers/VertexBufferObject/Headers/VertexBufferObject.h"

#define COORD(x,y,w)	((y)*(w)+(x))

bool Terrain::unload(){
    SAFE_DELETE(_terrainQuadtree);
    SAFE_DELETE(_groundVBO);
    SAFE_DELETE(_terrainRenderState);
    SAFE_DELETE(_terrainDepthRenderState);
    SAFE_DELETE(_terrainReflectionRenderState);

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
    _alphaTexturePresent = (_terrainTextures[TERRAIN_TEXTURE_ALPHA] != NULL);

    //Generate a render state
    RenderStateBlockDescriptor terrainDesc;
    terrainDesc.setCullMode(CULL_MODE_CW);
    _terrainRenderState = GFX_DEVICE.createStateBlock(terrainDesc);
    _terrainReflectionRenderState = GFX_DEVICE.createStateBlock(terrainDesc);
    //Generate a shadow render state
    terrainDesc.setCullMode(CULL_MODE_CCW);
    _terrainDepthRenderState = GFX_DEVICE.createStateBlock(terrainDesc);
    //For now, terrain doesn't cast shadows
    //getSceneNodeRenderState().addToDrawExclusionMask(SHADOW_STAGE);
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

    _groundVBO = GFX_DEVICE.newVBO(TRIANGLE_STRIP);
    _groundVBO->useLargeIndices(true);//<32bit indices
    const vectorImpl<vec3<F32> >& normalData	= _groundVBO->getNormal();
    const vectorImpl<vec3<F32> >& tangentData	= _groundVBO->getTangent();

    ImageTools::ImageData img;
    img.create(terrain->getVariable("heightmap"));
    ///data will be destroyed when img gets out of scope
    const U8* data = img.data();

    _terrainWidth = img.dimensions().width;
    _terrainHeight = img.dimensions().height;

    D_PRINT_FN(Locale::get("TERRAIN_INFO"),_terrainWidth, _terrainHeight);
    assert(data);
    U32 heightmapWidth  = _terrainWidth;
    U32 heightmapHeight = _terrainHeight;

    if(_terrainWidth%2==0)	_terrainWidth++;
    if(_terrainHeight%2==0)	_terrainHeight++;

    _groundVBO->resizePositionCount(_terrainWidth*_terrainHeight);
    _groundVBO->resizeNormalCount(_terrainWidth*_terrainHeight);
    _groundVBO->resizeTangentCount(_terrainWidth*_terrainHeight);

    _boundingBox.set(vec3<F32>(-_terrainWidth/2,  0.0f, -_terrainHeight/2),
                     vec3<F32>( _terrainWidth/2, 40.0f,  _terrainHeight/2));
    _boundingBox.Translate(terrain->getPosition());
    _boundingBox.Multiply(vec3<F32>(terrain->getScale().x,1,terrain->getScale().x));
    _boundingBox.MultiplyMax(vec3<F32>(1,_terrainHeightScaleFactor,1));

    const vec3<F32> bMin = _boundingBox.getMin();
    const vec3<F32> bMax = _boundingBox.getMax();

    vec3<F32> vertexData;
    for(U32 j=0; j < _terrainHeight; j++) {
        for(U32 i=0; i < _terrainWidth; i++) {
            U32 idxHM = COORD(i,j,_terrainWidth);

            vertexData.x = bMin.x + ((F32)i) * (bMax.x - bMin.x)/(_terrainWidth-1);
            vertexData.z = bMin.z + ((F32)j) * (bMax.z - bMin.z)/(_terrainHeight-1);

            U32 idxIMG = COORD(	i<heightmapWidth? i : i-1, j<heightmapHeight? j : j-1,	heightmapWidth);

            F32 h = (F32)(data[idxIMG*img.bpp() + 0] + data[idxIMG*img.bpp() + 1] + data[idxIMG*img.bpp() + 2])/3.0f;

            vertexData.y = (bMin.y + ((F32)h) * (bMax.y - bMin.y)/(255)) * _terrainHeightScaleFactor;
            _groundVBO->modifyPositionValue(idxHM,vertexData);
        }
    }

    U32 offset = 2;
    vec3<F32> vU, vV, vUV;
    for(U32 j=offset; j < _terrainHeight-offset; j++) {
        for(U32 i=offset; i < _terrainWidth-offset; i++) {
            U32 idx = COORD(i,j,_terrainWidth);

            vU.set(_groundVBO->getPosition(COORD(i+offset, j+0, _terrainWidth)) -  _groundVBO->getPosition(COORD(i-offset, j+0, _terrainWidth)));
            vV.set(_groundVBO->getPosition(COORD(i+0, j+offset, _terrainWidth)) -  _groundVBO->getPosition(COORD(i+0, j-offset, _terrainWidth)));
            vUV.cross(vV,vU); vUV.normalize();
            _groundVBO->modifyNormalValue(idx,vUV);
            vU = -vU; vU.normalize();
            _groundVBO->modifyTangentValue(idx,vU);
        }
    }

    for(U32 j=0; j < offset; j++) {
        for(U32 i=0; i < _terrainWidth; i++) {
            U32 idx0 = COORD(i,	j,		_terrainWidth);
            U32 idx1 = COORD(i,	offset,	_terrainWidth);

            _groundVBO->modifyNormalValue(idx0, normalData[idx1]);
            _groundVBO->modifyTangentValue(idx0,tangentData[idx1]);

            idx0 = COORD(i,	_terrainHeight-1-j,		 _terrainWidth);
            idx1 = COORD(i,	_terrainHeight-1-offset, _terrainWidth);

            _groundVBO->modifyNormalValue(idx0,normalData[idx1]);
            _groundVBO->modifyTangentValue(idx0,tangentData[idx1]);
        }
    }

    for(U32 i=0; i < offset; i++) {
        for(U32 j=0; j < _terrainHeight; j++) {
            U32 idx0 = COORD(i,		    j,	_terrainWidth);
            U32 idx1 = COORD(offset,	j,	_terrainWidth);

            _groundVBO->modifyNormalValue(idx0,normalData[idx1]);
            _groundVBO->modifyTangentValue(idx0,normalData[idx1]);

            idx0 = COORD(_terrainWidth-1-i,		 j,	_terrainWidth);
            idx1 = COORD(_terrainWidth-1-offset, j,	_terrainWidth);

            _groundVBO->modifyNormalValue(idx0,normalData[idx1]);
            _groundVBO->modifyTangentValue(idx0,normalData[idx1]);
        }
    }
    //Terrain dimensions
    vec2<U32> terrainDim(_terrainWidth, _terrainHeight);
    //Use primitive restart for each strip
    _groundVBO->setIndicesDelimiter(TERRAIN_STRIP_RESTART_INDEX);
    _terrainQuadtree->setParentShaderProgram(getMaterial()->getShaderProgram());
    _terrainQuadtree->setParentVBO(_groundVBO);
    _terrainQuadtree->Build(_boundingBox, terrainDim, terrain->getChunkSize(), _groundVBO);

    ResourceDescriptor infinitePlane("infinitePlane");
    infinitePlane.setFlag(true); //No default material
    _plane = CreateResource<Quad3D>(infinitePlane);
    F32 depth = GET_ACTIVE_SCENE()->state().getWaterDepth();
    F32 height = GET_ACTIVE_SCENE()->state().getWaterLevel()- depth;
    _farPlane = 2.0f * ParamHandler::getInstance().getParam<F32>("runtime.zFar");
    _plane->setCorner(Quad3D::TOP_LEFT, vec3<F32>(   -_farPlane, height, -_farPlane));
    _plane->setCorner(Quad3D::TOP_RIGHT, vec3<F32>(   _farPlane, height, -_farPlane));
    _plane->setCorner(Quad3D::BOTTOM_LEFT, vec3<F32>(-_farPlane, height,  _farPlane));
    _plane->setCorner(Quad3D::BOTTOM_RIGHT, vec3<F32>(_farPlane, height,  _farPlane));

    _plane->getSceneNodeRenderState().setDrawState(false);
    _diffuseUVScale = terrain->getDiffuseScale();
    _normalMapUVScale = terrain->getNormalMapScale();
    return true;
}

void Terrain::postLoad(SceneGraphNode* const sgn){
    if(getState() != RES_LOADED) {
        sgn->setActive(false);
    }
    _node = sgn;

    _groundVBO->Create();
    _planeSGN = _node->addNode(_plane);
    _plane->computeBoundingBox(_planeSGN);
    _plane->renderInstance()->preDraw(true);
    _planeSGN->setActive(false);
    _planeTransform = _planeSGN->getTransform();
    _plane->renderInstance()->transform(_planeTransform);
    computeBoundingBox(_node);
    ShaderProgram* s = getMaterial()->getShaderProgram();

    s->Uniform("detail_scale",  _normalMapUVScale);
    s->Uniform("diffuse_scale", _diffuseUVScale);
    s->Uniform("bbox_min", _boundingBox.getMin());
    s->Uniform("bbox_diff", _boundingBox.getMax() - _boundingBox.getMin());
    s->Uniform("_waterHeight", GET_ACTIVE_SCENE()->state().getWaterLevel());
    s->Uniform("dvd_lightCount", 1);
    s->UniformTexture("texDiffuseMap", 0);
    s->UniformTexture("texNormalHeightMap", 1);
    s->UniformTexture("texWaterCaustics", 2);
    s->UniformTexture("texBlend0", 3);
    s->UniformTexture("texBlend1", 4);
    s->UniformTexture("texBlend2", 5);
    if(_alphaTexturePresent) s->UniformTexture("texBlend3",6);

    _groundVBO->setShaderProgram(s);
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
    Vegetation* veg = New Vegetation(3, terrain->getGrassDensity(),
                                    terrain->getGrassScale(),
                                    terrain->getTreeDensity(),
                                    terrain->getTreeScale(),
                                    terrain->getVariable("grassMap"),
                                    grass);
    addVegetation(terrainSGN, veg, "grass");
    toggleVegetation(true);
    veg->initialize(_grassShader,this,terrainSGN);
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